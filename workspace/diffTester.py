import os
import pickle
import sys
import subprocess
import ctypes
import signal
import time
import struct
import csv
import itertools
import re
import hashlib
from header import *

# set working directory
WD = os.path.dirname(os.path.abspath(__file__))
os.chdir(WD)

'''
takes in an equivalence class, gathers all test inputs to be used 
with that equivalence class, and creates/saves discrepancy objects
'''
def diffTester(classNo, classKey, totalClassCount, equivalenceClass, TEST_INPUTS, UNIQUE_DISCREPANCIES, ALL_DISCREPANCIES, USED_INPUTS):

    print("Testing Class {}/{}".format(classNo, totalClassCount - 1))

    # grab all inputs tagged with "all_"
    inputNames = [x for x in TEST_INPUTS.keys() if "all_" in x]

    # add all inputs tagged with the name of a driver in the equivalence class
    for driver in equivalenceClass:
        inputNames.extend([x for x in TEST_INPUTS.keys() if (driver.get_driverName() in x) and len(TEST_INPUTS[x][0]) == driver.get_numberOfDoubles()])
        
    # remove any duplicates (s3fp inputs are gathered twice above)
    inputNames = list(set(inputNames))

    # test each of those inputs
    for inputName in sorted(inputNames):
        
        # store the name of input used for the purpose of gathering statistics
        if inputName not in USED_INPUTS.keys():
            USED_INPUTS[inputName] = 1
        else:
            USED_INPUTS[inputName] += 1

        inputTuple = TEST_INPUTS[inputName]
        outputDriverPairs = []

        # collect tuples of the form (driver, output)
        for driver in equivalenceClass:
            outputDriverPairs.append((driver, driver.run_driver(inputTuple[0], inputTuple[1])))

        # if the outputs are not consistent...
        if not isConsistent([f[1] for f in outputDriverPairs]):

            # create and save a discrepancy object
            x = Discrepancy(classKey, inputName, inputTuple, outputDriverPairs)
            if x:            
                if x.get_id() not in UNIQUE_DISCREPANCIES.keys():
                    UNIQUE_DISCREPANCIES[x.get_id()] = x
                    UNIQUE_DISCREPANCIES[x.get_id()].set_discrepancyNo(hashlib.md5(str(x.get_id()).encode()).hexdigest())

                # if we get a better relative error for an identical discrepancy id, take the better relative error
                elif x.get_discrepancyCategory() == INACCURACY and UNIQUE_DISCREPANCIES[x.get_id()].get_maxRelErr() < x.get_maxRelErr():
                    UNIQUE_DISCREPANCIES[x.get_id()] = x
                    UNIQUE_DISCREPANCIES[x.get_id()].set_discrepancyNo(hashlib.md5(str(x.get_id()).encode()).hexdigest())
                
                ALL_DISCREPANCIES.append(x)
        

''' function to write out a csv file of the results '''
def writeOutCSVfile(DISCREPANCIES, csvName):

    with open("logs/{}".format(csvName), 'w') as f:

        fieldnames = ['discrepancyNo', 'classKey', 'discrepancyCategory', 'inputName', 'inputList', 'libraryName', 'driverName', 'funcName', 'output', 'absErrs', 'relErrs', 'maxDeltaDiff']
        writer = csv.DictWriter(f, fieldnames=fieldnames)

        writer.writeheader()

        for discrepancy in DISCREPANCIES.values():

            if "_pos" in discrepancy.get_inputName():
                inputName = discrepancy.get_inputName()[:-2]
            else:
                inputName = discrepancy.get_inputName()

            for pair in discrepancy.get_driverOutputPairs():
                    writer.writerow({'discrepancyNo' : discrepancy.get_discrepancyNo(),
                                'classKey': "{:0>3}".format(discrepancy.get_classKey()),
                                'discrepancyCategory': discrepancy.get_discrepancyCategory(),
                                'inputName': inputName,
                                'inputList': discrepancy.get_inputList(),
                                'libraryName': pair[0].get_libraryName(),
                                'driverName': pair[0].get_driverName(),
                                'funcName': pair[0].get_funcName(),
                                'output': pair[1],
                                'absErrs' : discrepancy.get_absErrs(),
                                'relErrs' : discrepancy.get_relErrs(),
                                'maxDeltaDiff' : discrepancy.get_maxDeltaDiff()})

    sort_csv(csvName)


def getStats(UNIQUE_DISCREPANCIES, ALL_DISCREPANCIES, USED_INPUTS):

    total_discrepancyTally = {1:0, 2:0, 3:0, 4:0, 5:0, 6:0}

    specialValue_discrepancyTally = {1:0, 2:0, 3:0, 4:0, 5:0, 6:0}
    specialValUniqueDiscrepancies = {}
    specialValueInputTotal = 0
    specialValueEvals = 0

    testMigration_discrepancyTally = {1:0, 2:0, 3:0, 4:0, 5:0, 6:0}
    testMigrateUniqueDiscrepancies = {}
    testMigrationInputTotal = 0
    testMigrationEvals = 0

    s3fp_discrepancyTally = {1:0, 2:0, 3:0, 4:0, 5:0, 6:0}
    s3fpUniqueDiscrepancies = {}
    s3fpInputTotal = 0
    s3fpEvals = 0

    for inputName, tally in USED_INPUTS.items():
        if "_pos" in inputName:
            specialValueInputTotal += 1
            specialValueEvals += tally
        elif "_num" in inputName:
            testMigrationInputTotal += 1
            testMigrationEvals += tally
        elif "s3fpInput" in inputName:
            s3fpInputTotal += 1
            s3fpEvals += tally
    
    ALL_DISCREPANCIES.sort(key=lambda x : str(x.get_discrepancyNo()))

    for discrepancy in ALL_DISCREPANCIES:
        if "_pos" in discrepancy.get_inputName():
            if discrepancy.get_discrepancyNo() not in specialValUniqueDiscrepancies.keys():
                specialValUniqueDiscrepancies[discrepancy.get_discrepancyNo()] = discrepancy
                specialValue_discrepancyTally[discrepancy.get_discrepancyCategory()] += 1
        elif "_num" in discrepancy.get_inputName():
            if discrepancy.get_discrepancyNo() not in testMigrateUniqueDiscrepancies.keys():
                testMigrateUniqueDiscrepancies[discrepancy.get_discrepancyNo()] = discrepancy
                testMigration_discrepancyTally[discrepancy.get_discrepancyCategory()] += 1
        elif "s3fpInput" in discrepancy.get_inputName():
            if discrepancy.get_discrepancyNo() not in s3fpUniqueDiscrepancies.keys():
                s3fpUniqueDiscrepancies[discrepancy.get_discrepancyNo()] = discrepancy
                s3fp_discrepancyTally[discrepancy.get_discrepancyCategory()] += 1
            
    for discrepancy in UNIQUE_DISCREPANCIES.values():
        total_discrepancyTally[discrepancy.get_discrepancyCategory()] += 1

    with open("logs/statistics.txt", 'a') as f:
        tempx = list(specialValue_discrepancyTally.values())
        tempy = list(testMigration_discrepancyTally.values())
        tempz = list(s3fp_discrepancyTally.values())

        tempx.reverse()
        tempy.reverse()
        tempz.reverse()

        f.write("\nspecialValue_unique_discrepancyTally: {}\n".format(tempx))
        #f.write("\t# of Unique Discrepancies/# of Evals (# of Unique Inputs): {}/{} ({})\n".format(sum(list(specialValue_discrepancyTally.values())), specialValueEvals, specialValueInputTotal))
        f.write("testMigration_unique_discrepancyTally: {}\n".format(tempy))
        #f.write("\t# of Unique Discrepancies/# of Evals (# of Unique Inputs): {}/{} ({})\n".format(sum(list(testMigration_discrepancyTally.values())), testMigrationEvals, testMigrationInputTotal))
        f.write("s3fp_unique_discrepancyTally: {}\n".format(tempz))
        #f.write("\t# of Unique Discrepancies/# of Evals (# of Unique Inputs): {}/{} ({})\n".format(sum(list(s3fp_discrepancyTally.values())), s3fpEvals, s3fpInputTotal))

        f.write("\nUNIQUE DISCREPANCY TOTALS:\n")
        f.write("\tTIMEOUTS: {}\n".format(total_discrepancyTally[6]))
        f.write("\tDOUBLES FROM NAN: {}\n".format(total_discrepancyTally[5]))
        f.write("\tDOUBLES FROM INF: {}\n".format(total_discrepancyTally[4]))
        f.write("\tINACCURACIES: {}\n".format(total_discrepancyTally[3]))
        f.write("\tMIX OF DOUBLES, EXCEPTIONS, SPECIAL VALUES: {}\n".format(total_discrepancyTally[2]))
        f.write("\tMIX OF EXCEPTIONS AND SPECIAL VALUES: {}\n".format(total_discrepancyTally[1]))
        f.write("\n\t\tTOTAL # OF DISCREPANCIES: {}\n".format(sum(list(total_discrepancyTally.values()))))

                    
def inspect(UNIQUE_DISCREPANCIES):
    inspected_discrepancies = {}

    for discrepancyNo, discrepancy in UNIQUE_DISCREPANCIES.items():

        # remove type 6 discrepancies
        if discrepancy.get_discrepancyCategory() != MIX_OF_EXCEPTIONS_SPECIAL_VALUES:

            # remove type 4 discrepancies between high and low
            # precision mpmath functions
            if discrepancy.get_discrepancyCategory() == INACCURACY:
                x = [x for x in discrepancy.relErrs.items() if x[1] == discrepancy.get_maxRelErr()]
                if re.search('mpmath.*/mpmath', x[0][0]):
                    continue

            inspected_discrepancies[discrepancyNo] = discrepancy

    writeOutCSVfile(inspected_discrepancies, "reducedDiffTestingResults.csv")

if __name__ == "__main__":

    UNIQUE_DISCREPANCIES = {}
    ALL_DISCREPANCIES = []
    USED_INPUTS = {}

    with open("__temp/__equivalenceClasses", 'rb') as fp:
        CLASSES = pickle.load(fp)

    with open("__temp/__testInputs", "rb") as fp:
        TEST_INPUTS = pickle.load(fp)

    # load s3fpInputs if there are any
    elementaryInts = []
    with open("__temp/__elementaryInputs", "rb") as fp:
        ELEMENTARY_INPUTS = pickle.load(fp)
        for outputTuple in ELEMENTARY_INPUTS.values():
            if outputTuple[1] not in elementaryInts:
                elementaryInts.append(outputTuple[1])

    savedInputs = [f for f in os.listdir("savedInputs") if "s3fpInput" in f]
    for x in savedInputs:
        with open("savedInputs/" + x, 'rb') as f:
            doubles = []
            temp = f.read(8)
            while temp:
                temp = struct.unpack('d', temp)[0]
                doubles.append(temp)
                temp = f.read(8)
            for i, ints in enumerate(elementaryInts):
                TEST_INPUTS[x + "_{:0>3}".format(i)] = (doubles, ints)

    for classNo, classKey in enumerate(sorted(list(CLASSES.keys()))):
        diffTester(classNo, classKey, len(list(CLASSES.keys())), CLASSES[classKey], TEST_INPUTS, UNIQUE_DISCREPANCIES, ALL_DISCREPANCIES, USED_INPUTS)

    writeOutCSVfile(UNIQUE_DISCREPANCIES, "__diffTestingResults.csv")
    getStats(UNIQUE_DISCREPANCIES, ALL_DISCREPANCIES, USED_INPUTS)

    inspect(UNIQUE_DISCREPANCIES)

    with open("__temp/__uniqueDiscrepancies", "wb") as fp:
        pickle.dump(UNIQUE_DISCREPANCIES, fp)

    with open("__temp/__allDiscrepancies", 'wb') as f:
        pickle.dump(ALL_DISCREPANCIES, f)