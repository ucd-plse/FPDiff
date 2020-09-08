import ctypes
import os
import sys
import subprocess
import random
import pickle
import numpy as np
from header import *
import csv
import itertools
import math
import multiprocessing
from pprint import pprint
from difflib import SequenceMatcher

SIZE_OF_INPUTS = 8
NUMBER_OF_DIFFERENT_INTEGER_INPUTS = 4
NUMBER_OF_DIFFERENT_DOUBLE_INPUTS = 10

IGNORE =    [

                "gsl_sf_lambert_Wm1_DRIVER",    # diverges from Wm0 for reals in [-1,0] (contributes 2 incorrect mappings)

                "gsl_sf_log_abs_DRIVER",        # diverges from log for negative reals (contributes 3 incorrect mappings)

                "scipy_special_loggamma_arg1_DRIVER0",    # diverges from gammaln for negative reals (contributes 4 incorrect mappings)

                "jmat_rem_DRIVER0",              # diverges from mod for negative reals (contributes 1 incorrect mapping)

                "jmat_lshift_DRIVER1",           # incorrect mapping (contributes 1 incorrect mapping)
                "jmat_rshift_DRIVER1",

                "jmat_fracn_DRIVER0"            # incorrect mapping, diverges (contributes 2 incorrect mappings)
                                                #for negative inputs
            ]

ACCEPTABLE_REPEATS = []
            
            
def generateElementaryInputs():
    ELEMENTARY_INPUTS = {}

    inputNo = 0

    for i in range(NUMBER_OF_DIFFERENT_DOUBLE_INPUTS):
        random.seed(i)

        doubles=[random.uniform(0,3) for j in range(SIZE_OF_INPUTS)]

        for k in range(NUMBER_OF_DIFFERENT_INTEGER_INPUTS):  
            ints = [(x+k)%NUMBER_OF_DIFFERENT_INTEGER_INPUTS for x in range(SIZE_OF_INPUTS)]

            ELEMENTARY_INPUTS["elementaryInput_num{:0>3}".format(inputNo)] = (doubles, ints)
            inputNo += 1

    with open("__temp/__elementaryInputs", 'wb') as f:
        pickle.dump(ELEMENTARY_INPUTS, f)

    return ELEMENTARY_INPUTS


def classify(CLASSES, DRIVER_LIST, ELEMENTARY_INPUTS):

    # i for progress bar
    i = 1

    lengthInput = len(ELEMENTARY_INPUTS.values()) * len(DRIVER_LIST.values())

    # shared dict to store key and result
    manager = multiprocessing.Manager()
    RESULT_LIST_MANAGER = manager.dict()

    # function for parallization
    def runDriver_parallelly(elementaryInput, driver, key):
        result = driver.run_driver(elementaryInput[0], elementaryInput[1])
        
        if isinstance(result, str):
                driver.add_exceptionMessage(result)
                driver.add_classificationOutput(0)

        # weeds out special values, identity funcs, rounding funcs
        elif np.isfinite(result):
            if driver.get_numericalArgNo() == 1:
                if result != elementaryInput[0][0] and result != math.ceil(elementaryInput[0][0]) and result != math.floor(elementaryInput[0][0]):
                    driver.add_classificationOutput(result)
                else:
                    driver.add_exceptionMessage("warning: possible identity or rounding function")
                    driver.add_classificationOutput(0)
            else:
                driver.add_classificationOutput(result)
        else:
            driver.add_exceptionMessage("unrecognized output type")
            driver.add_classificationOutput(0)
        DRIVER_LIST[key] = driver
    
    # for each elementary input
    for elementaryInput in ELEMENTARY_INPUTS.values():
        taskID = 1
        processes = []
        # for each driver in the DRIVER_LIST, run on the elementary input and accumulate result
        for key, driver in DRIVER_LIST.items():
            # create multi-process
            p = multiprocessing.Process(target=runDriver_parallelly, args=[elementaryInput, driver, key])
            p.start()
            processes.append(p)

            # wait when we have enough tasks in parallel
            if taskID % NUM_MULTIPROCESSING is 0:
                # wait for the processes
                for process in processes:
                    process.join()
            
            # printing progress bar
            sys.stdout.write('\r')
            sys.stdout.write("[%-100s] %d%%" % ('#'*int(i/lengthInput*100), int(i/lengthInput*100)))
            sys.stdout.flush()
            i+=1

        # wait for the processes
        for process in processes:
            process.join()
    
    sys.stdout.write('\n')

    executedDriverLists = {}

    # classify each driver based on the accumulated result (also collect executed drivers)            
    for driver in DRIVER_LIST.values():

        # weed out drivers that threw exceptions on every execution
        if len(driver.get_exceptionMessages()) != len(ELEMENTARY_INPUTS.values()):
            
            # accumulate lists of unique functions per library
            if driver.get_libraryName() not in executedDriverLists.keys():
                executedDriverLists[driver.get_libraryName()] = []
            if driver.get_funcName() not in executedDriverLists[driver.get_libraryName()]:
                executedDriverLists[driver.get_libraryName()].append(driver.get_funcName())

            # continue on to classify if the driver didn't return the same value on every execution
            if len(set([f for f in driver.get_classificationOutputs() if f != 0])) > 1:

                # insert first class
                if len(CLASSES) == 0:
                    CLASSES[driver.get_id()] = [driver]

                # otherwise compare against all outputs to find its equivalence class
                else:
                    match = False

                    # go through the existing classKeys
                    for classKey, equivalenceClass in CLASSES.items():

                        # if the current tuple is close enough to the class key, add it to the equivalence class
                        if isSameNumber(driver.get_id(), classKey) and equivalenceClass[0].get_numberOfInts() == driver.get_numberOfInts() and equivalenceClass[0].get_numberOfDoubles() == driver.get_numberOfDoubles():
                            
                            match = True

                            if all(x == True for x in list(map(isSameNumber, driver.get_classificationOutputs(), equivalenceClass[0].get_classificationOutputs()))):
                                
                                # conditional added to remove duplicate functions
                                if driver.get_libraryName() + driver.get_driverName() not in [f.get_libraryName() + f.get_driverName() for f in equivalenceClass]:
                                    CLASSES[classKey].append(driver)
                                
                                break

                    # otherwise, add a new equivalence class        
                    if not match:
                        CLASSES[driver.get_id()] = [driver]

    ''' code to write out the executable driver lists ''' 
    with open("logs/__executableDrivers.txt", 'w') as f:
        for libraryName in executedDriverLists.keys():
            for funcName in sorted(executedDriverLists[libraryName]):
                f.write("{} : {}\n".format(libraryName, funcName))


def isSameNumber(x,y):
    if np.isfinite(np.array([x,y])).prod():
        return 2 * (abs(x - y))/max((abs(x) + abs(y) ), CLASS_TOLERANCE) < CLASS_TOLERANCE
    else:
        return False

def isSameOutput_classifier(x,y):

    # if they are different types, return false
    if type(x) != type(y):
        return False

    # if they are both exceptions, consider them the same
    elif type(x) == type("hello"):
        return True

    # if they are all finite, call isSameNumber
    elif np.isfinite(np.array([x,y])).prod():
        return isSameNumber(x,y)

    # if they are all non-finite, return True
    elif np.isfinite(np.array([x,y])).sum() == 0:
        return True

    return False


'''
functions that consults the hard-coded IGNORE list
above to automatically remove selected drivers
'''
def autoRemoveSelectedDrivers(CLASSES):

    deleteList = []

    for classKey, equivalenceClass in CLASSES.items():

        for driver in equivalenceClass:
            if driver.get_driverName() in IGNORE:
                deleteList.append((classKey, driver))

    for classKey, driver in deleteList:
        CLASSES[classKey].remove(driver)

    pruneClasses(CLASSES)


'''
function to delete equivalence classes that fall into either of two categories:
1) Those that contain only a single driver
2) Those that contain a set of function mappings that are a subset of those in another equivalence class (redundant)
'''
def pruneClasses(CLASSES):

    deleteList = []

    # figure out which classes have only one driver and delete them
    for classKey, equivalenceClass in CLASSES.items():

        if len(equivalenceClass) < 2:
            deleteList.append(classKey)

    for x in deleteList:
        del CLASSES[x]

    temp = {}
    deleteList = []

    for classKey, equivalenceClass in CLASSES.items():

        # gather set of all functions present
        thisFuncSet = *(f.get_libraryName() + f.get_funcName() for f in equivalenceClass),
        
        match = False
        replace = False

        # for each funcSet we've already found...
        for funcSet in temp.keys():
            
            # if the current funcSet is a subset of the funcSet in an existing class, we can delete this class
            if set(thisFuncSet).issubset(set(funcSet)):
                deleteList.append(classKey)
                match = True
                break

            # if the current funcSet is a superset of the funcSet in an existing class, we can delete that class
            elif set(thisFuncSet).issuperset(set(funcSet)):
                deleteList.append(temp[funcSet])
                replace = funcSet
                break

        if replace:
            del temp[replace]
            temp[thisFuncSet] = classKey

        elif not match and not replace:
            temp[thisFuncSet] = classKey

    for x in deleteList:
            del CLASSES[x]
            

def getStats(CLASSES):
    
    functionMappings = {}
    driverMappings = {}
    functionsWithMappings = {}
    nontrivialClassTally = 0
    allClassTally = 0

    # for each equivalence class
    for equivalenceClass in CLASSES.values():

        # for each pair of drivers
        for pair in itertools.combinations(equivalenceClass, 2):

            # concatenate libraries to get mapping type, eg "mpmath/scipy"
            libraries = sorted([pair[0].get_libraryName(), pair[1].get_libraryName()])
            libraries = libraries[0] + '/' + libraries[1]

            # concatenate function names to get mapping name, eg "gsl_sf_poch/rf"
            functionNames = sorted([pair[0].get_funcName(), pair[1].get_funcName()])
            functionNames = functionNames[0] + '/' + functionNames[1]

            # concatenate driver names to get unique mapping name, eg "gsl_sf_poch_DRIVER/mpmath_fp_rf_DRIVER0"
            driverNames = sorted([pair[0].get_driverName(), pair[1].get_driverName()])
            driverNames = driverNames[0] + '/' + driverNames[1]

            if libraries not in functionMappings.keys():
                functionMappings[libraries] = [functionNames]
            elif functionNames not in functionMappings[libraries]:
                functionMappings[libraries].append(functionNames)

            if libraries not in driverMappings.keys():
                driverMappings[libraries] = [driverNames]
            elif driverNames not in driverMappings[libraries]:
                driverMappings[libraries].append(driverNames)
        
        # count up the number of functions with mappings
        for driver in equivalenceClass:

            if driver.get_libraryName() not in functionsWithMappings.keys():
                functionsWithMappings[driver.get_libraryName()] = [driver.get_funcName()]
            elif driver.get_funcName() not in functionsWithMappings[driver.get_libraryName()]:
                functionsWithMappings[driver.get_libraryName()].append(driver.get_funcName())

        # contribute to the class tallies
        funcsPresent = []
        for driver in equivalenceClass:
            uniqueName = driver.get_libraryName() + driver.get_funcName()
            if uniqueName not in funcsPresent:
                funcsPresent.append(uniqueName)
            if len(funcsPresent) > 1:
                nontrivialClassTally += 1
                break
        allClassTally += 1


    # write out the discovered mappings
    with open("logs/__driverMappings.txt", 'wt') as out:
        for x in driverMappings.values():
            x.sort()
        pprint(driverMappings, stream=out)
    with open("logs/__funcMappings.txt", 'wt') as out:
        for x in functionMappings.values():
            x.sort()
        pprint(functionMappings, stream=out)

    # gather some statistics, save the information
    with open("logs/statistics.txt", 'a') as f:
        f.write("\nTOTAL # OF CLASSES: {}\n".format(allClassTally))
        

def prettyPrintClasses(CLASSES):
    print("--------------------------------------------")

    for counter, classKey in enumerate(CLASSES.keys()):
        print("Class {}:\n".format(counter))
        for driver in CLASSES[classKey]:
            print("{}".format(driver.get_driverName()))
        print()
        print("--------------------------------------------")


if __name__ == "__main__":

    with open("__temp/__driverCollection", 'rb') as fp:
        DRIVER_LIST = pickle.load(fp)

    # convert DRIVER_LIST to a shared list
    manager = multiprocessing.Manager()

    DRIVER_LIST_MANAGER = manager.dict(DRIVER_LIST)

    ELEMENTARY_INPUTS = generateElementaryInputs()

    CLASSES = {}

    print("\n** Running Classifier: ")
    classify(CLASSES, DRIVER_LIST_MANAGER, ELEMENTARY_INPUTS)
    DRIVER_LIST = dict(DRIVER_LIST_MANAGER)

    pruneClasses(CLASSES)
    autoRemoveSelectedDrivers(CLASSES)
    getStats(CLASSES)
    
    temp = {}
    for classKey in sorted(CLASSES.keys()):
        temp[classKey] = CLASSES[classKey]

    CLASSES = temp

    prettyPrintClasses(CLASSES)

    with open("__temp/__equivalenceClasses", 'wb') as fp:
        for equivalenceClass in CLASSES.values():
            for driver in equivalenceClass:
                driver.reset_callableDriver()
        pickle.dump(CLASSES, fp)

    with open("logs/equivalenceClasses.csv", 'w') as fp:

        fieldnames = ['classKey', 'driverName', 'funcName']
        writer = csv.DictWriter(fp, fieldnames=fieldnames)

        writer.writeheader()
        
        for classKey, equivalenceClass in CLASSES.items():
            for driver in equivalenceClass:
                writer.writerow({   'classKey':classKey,
                                    'driverName':driver.get_driverName(),
                                    'funcName':driver.get_funcName()})

    sort_csv("equivalenceClasses.csv")