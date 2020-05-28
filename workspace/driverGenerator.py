import pickle
import os
import sys
import subprocess

from abc import ABC, abstractmethod

import header

# set working directory
WD = os.path.dirname(os.path.abspath(__file__))
os.chdir(WD)


class TemplateGenerator(ABC):

    def __init__(self, libraryName, signatures, imports, fromImports):
        self.libraryName = libraryName
        self.signatures = signatures
        self.imports = imports
        self.fromImports = fromImports

    @abstractmethod
    def generate(self, DRIVER_LIST, TEST_INPUTS):
        pass

    @abstractmethod
    def save_test_migration_inputs(self, extracted_args, driver, TEST_INPUTS):
        pass

    @abstractmethod
    def construct_driver_object(self, funcName, args):
        pass

    @abstractmethod
    def form_names(self, funcName):
        pass

    @abstractmethod
    def write_harness_shared(self):
        pass

    @abstractmethod
    def write_harness_begin(self, driver):
        pass

    @abstractmethod
    def write_harness_pre_call(self, driver):
        pass

    @abstractmethod
    def write_harness_func_call(self, driver):
        pass

    @abstractmethod
    def write_harness_end(self, driver):
        pass


class GenericCGenerator(TemplateGenerator):
    
    def __init__(self, libraryName, signatures, imports, fromImports):
        super().__init__(libraryName, signatures, imports, fromImports)
        self.language = "c"
        self.driverFilePath = "spFunDrivers/{}_drivers.c".format(self.libraryName)


    def save_test_migration_inputs(self, extracted_args, driver, TEST_INPUTS):
        for j in range(1, len(extracted_args)):

            ints = []
            doubles = []

            for i in range(driver.get_numberOfParameters()):
                if "int" in extracted_args[0][i]:
                    ints.append(int(extracted_args[j][i]))
                elif "double" in extracted_args[0][i]:
                    doubles.append(float(extracted_args[j][i]))

            TEST_INPUTS["{}~input_num{:0>3}".format(driver.get_driverName(), j-1)] = (doubles, ints)


    def generate(self, DRIVER_LIST, TEST_INPUTS):

        self.write_harness_shared()

        for funcName, args in self.signatures.items():

            # create new Driver object
            thisDriver = self.construct_driver_object(funcName, args)
        
            # store new driver in master DRIVER_LIST
            DRIVER_LIST[thisDriver.get_driverName()] = thisDriver

            self.save_test_migration_inputs(args, thisDriver, TEST_INPUTS)
            self.write_harness_begin(thisDriver)
            self.write_harness_pre_call(thisDriver)
            self.write_harness_func_call(thisDriver)
            self.write_harness_end(thisDriver)
    
    
    def construct_driver_object(self, funcName, args):
            # the c extractors keep the names of the data types for
            # the arguments as the first list in the list of args 
            argumentTypes = args[0]

            names = self.form_names(funcName)
                        
            return header.GenericCDriver(
                driverName=names['driverName'],
                funcName=names['funcName'],
                libraryName=self.libraryName,
                language=self.language,
                numberOfParameters=len(argumentTypes),
                callName=names['callName'],
                numberOfDoubles=sum("double" in argType for argType in argumentTypes),
                numberOfInts=sum("int" in argType for argType in argumentTypes)
            ) 


    def form_names(self, funcName):
        driverName = "{}_{}_DRIVER".format(self.libraryName, funcName)
        callName = funcName
        return {'driverName' : driverName, 'funcName' : funcName, 'callName' : callName}


    def write_harness_shared(self):
        with open(self.driverFilePath, 'a') as f:
            for importName in self.imports:
                f.write("#include <{}>\n".format(importName))

    
    def write_harness_begin(self, driver):
        with open(self.driverFilePath, 'a') as f:
            f.write("double {}(double * doubleInput, int * intInput)\n".format(driver.get_driverName()))
            f.write("{\n")
            f.write("\tdouble out;\n")

    
    def write_harness_pre_call(self, driver):
        pass


    def write_harness_func_call(self, driver):
        with open(self.driverFilePath, 'a') as f:
            
            f.write("\tout= {}(".format(driver.get_callName()))

            for i in range(driver.get_numberOfInts()):

                # write the read from the array as an argument to function
                f.write("intInput[{}]".format(i))

                # add a comma if there are more arguments to come
                if i + 1 != driver.get_numberOfInts() or driver.get_numberOfDoubles() != 0:
                    f.write(", ")

            # do the same as above but with the double arguments
            for i in range(driver.get_numberOfDoubles()):
                f.write("doubleInput[{}]".format(i))
                if i + 1 != driver.get_numberOfDoubles():
                    f.write(", ")

            f.write(");\n\n")


    def write_harness_end(self, driver):
        with open(self.driverFilePath, 'a') as f:
            f.write("\treturn out;\n")
            f.write("}\n\n")


class GenericPythonGenerator(TemplateGenerator):
    
    def __init__(self, libraryName, signatures, imports, fromImports):
        super().__init__(libraryName, signatures, imports, fromImports)
        self.language = "python"
        self.driverFilePath = "spFunDrivers/{}_drivers.py".format(self.libraryName)


    def save_test_migration_inputs(self, extracted_args, driver, TEST_INPUTS):
        for j in range(len(extracted_args)):

            ints = []
            doubles = []

            for k in range(driver.get_numberOfInts()):
                ints.append(int(extracted_args[j][k]))
            for k in range(driver.get_numberOfDoubles()):
                doubles.append(float(extracted_args[j][k + driver.get_numberOfInts()]))

            TEST_INPUTS["{}~input_num{:0>3}".format(driver.get_driverName(), j)] = (doubles, ints)


    def generate(self, DRIVER_LIST, TEST_INPUTS):

        self.write_harness_shared()

        for funcNameWithArgNo, args in self.signatures.items():
            
            numberOfParameters = len(args[0])

            for numberOfInts in range(numberOfParameters):

                # create new Driver object
                thisDriver = self.construct_driver_object(funcNameWithArgNo, numberOfInts, numberOfParameters)
            
                # store new driver in master DRIVER_LIST
                DRIVER_LIST[thisDriver.get_driverName()] = thisDriver

                self.save_test_migration_inputs(args, thisDriver, TEST_INPUTS)
                self.write_harness_begin(thisDriver)
                self.write_harness_pre_call(thisDriver)
                self.write_harness_func_call(thisDriver)
                self.write_harness_end(thisDriver)
        
    
    def construct_driver_object(self, funcNameWithArgNo, numberOfInts, numberOfParameters):
        numberOfDoubles = numberOfParameters - numberOfInts
        
        names = self.form_names(funcNameWithArgNo, numberOfInts)
        
        return header.GenericPythonDriver(
                driverName=names['driverName'],
                funcName=names['funcName'],
                libraryName=self.libraryName,
                language=self.language,
                numberOfParameters=numberOfParameters,
                callName=names['callName'],
                numberOfDoubles=numberOfDoubles,
                numberOfInts=numberOfInts
            )


    def form_names(self, funcNameWithArgNo, numberOfInts):
        driverName = "{}_{}_DRIVER{}".format(self.libraryName, funcNameWithArgNo.replace('.', '_'), numberOfInts )
        callName = funcNameWithArgNo[:funcNameWithArgNo.index("_arg")]
        if '.' in callName:
            funcName = callName[callName.index(".") + 1:]
        else:
            funcName = callName

        return {'driverName' : driverName, 'funcName' : funcName, 'callName' : callName}


    def write_harness_shared(self):
        with open(self.driverFilePath, 'a') as f:
            for importName in self.imports:
                if len(importName) == 1:
                    f.write("import {}\n".format(importName[0]))
                elif len(importName) == 2:
                    f.write("import {} as {}\n".format(importName[0], importName[1]))
            for importName in self.fromImports:
                if len(importName) == 2:
                    f.write("from {} import {}\n".format(importName[0], importName[1]))
                elif len(importName) == 3:
                    f.write("from {} import {} as {}\n".format(importName[0], importName[1], importName[2]))


    def write_harness_begin(self, driver):
        with open(self.driverFilePath, 'a') as f:
            f.write("def {}(doubleInput, intInput):\n".format(driver.get_driverName()))

    
    def write_harness_pre_call(self, driver):
        pass


    def write_harness_func_call(self, driver):
        with open(self.driverFilePath, 'a') as f:
            
            f.write("\tout= {}(".format(driver.get_callName()))

            for i in range(driver.get_numberOfInts()):

                # write the read from the array as an argument to function
                f.write("intInput[{}]".format(i))

                # add a comma if there are more arguments to come
                if i + 1 != driver.get_numberOfInts() or driver.get_numberOfDoubles() != 0:
                    f.write(", ")

            # do the same as above but with the double arguments
            for i in range(driver.get_numberOfDoubles()):
                f.write("doubleInput[{}]".format(i))
                if i + 1 != driver.get_numberOfDoubles():
                    f.write(", ")

            f.write(");\n\n")


    def write_harness_end(self, driver):
        with open(self.driverFilePath, 'a') as f:
            f.write("\treturn float(out)\n\n")


class GSLGenerator(GenericCGenerator):

    def __init__(self, libraryName, signatures, imports, fromImports):
        super().__init__(libraryName, signatures, imports, fromImports)


    # no need to include library name in driverName as per the default
    # method; gsl functions names are already preprended with "gsl_"
    def form_names(self, funcName):
        driverName = "{}_DRIVER".format(funcName)
        callName = funcName
        return {'driverName' : driverName, 'funcName' : funcName, 'callName' : callName}


    # include code for custom gsl error handler
    def write_harness_shared(self):
        super().write_harness_shared()
        with open(self.driverFilePath, 'a') as f:
            f.write("void my_handler(const char * reason, const char * file, int line, int gsl_errno)\n")
            f.write("{\n")
            f.write("\tfprintf(stderr, \"%s\\n\", reason);\n")
            f.write("}\n\n")


    # include code to set custom gsl error handler
    def write_harness_pre_call(self, driver):
        with open(self.driverFilePath, 'a') as f:
            f.write("\tgsl_error_handler_t * old_handler = gsl_set_error_handler (&my_handler);\n\n")


    # everything identical to base class method save for two lines
    # that deal with gsl_mode_t type parameters
    def write_harness_func_call(self, driver):
        with open(self.driverFilePath, 'a') as f:
            
            f.write("\tout= {}(".format(driver.get_callName()))

            for i in range(driver.get_numberOfInts()):

                f.write("intInput[{}]".format(i))

                if i + 1 != driver.get_numberOfInts() or driver.get_numberOfDoubles() != 0:
                    f.write(", ")

            for i in range(driver.get_numberOfDoubles()):
                f.write("doubleInput[{}]".format(i))
                if i + 1 != driver.get_numberOfDoubles():
                    f.write(", ")

            if driver.get_numberOfDoubles() + driver.get_numberOfInts() < driver.get_numberOfParameters():
                f.write(", GSL_PREC_DOUBLE")

            f.write(");\n\n")


    def save_test_migration_inputs(self, extracted_args, driver, TEST_INPUTS):
        return super().save_test_migration_inputs(extracted_args, driver, TEST_INPUTS)

    def generate(self, DRIVER_LIST, TEST_INPUTS):
        return super().generate(DRIVER_LIST, TEST_INPUTS)

    def construct_driver_object(self, funcName, args):
        return super().construct_driver_object(funcName, args)

    def write_harness_begin(self, driver):
        return super().write_harness_begin(driver)

    def write_harness_end(self, driver):
        return super().write_harness_end(driver)


class MpmathGenerator(GenericPythonGenerator):

    # give each function a floating point precision signatures in
    # addition to the default arbitrary precision 
    def __init__(self, libraryName, signatures, imports, fromImports):
        super().__init__(libraryName, signatures, imports, fromImports)

        fp_sigs={}
        for funcNameWithArgNo, args in self.signatures.items():
            if "." not in funcNameWithArgNo:
                fp_sigs["fp.{}".format(funcNameWithArgNo)] = args

        for funcNameWithArgNo, args in fp_sigs.items():
            self.signatures[funcNameWithArgNo] = args


    def save_test_migration_inputs(self, extracted_args, driver, TEST_INPUTS):
        return super().save_test_migration_inputs(extracted_args, driver, TEST_INPUTS)
        
    def generate(self, DRIVER_LIST, TEST_INPUTS):
        return super().generate(DRIVER_LIST, TEST_INPUTS)

    def construct_driver_object(self, funcNameWithArgNo, numberOfInts, numberOfParameters):
        return super().construct_driver_object(funcNameWithArgNo, numberOfInts, numberOfParameters)
        
    def write_harness_shared(self):
        return super().write_harness_shared()

    def form_names(self, funcNameWithArgNo, numberOfInts):
        return super().form_names(funcNameWithArgNo, numberOfInts)

    def write_harness_begin(self, driver):
        return super().write_harness_begin(driver)

    def write_harness_pre_call(self, driver):
        return super().write_harness_pre_call(driver)

    def write_harness_func_call(self, driver):
        return super().write_harness_func_call(driver)

    def write_harness_end(self, driver):
        return super().write_harness_end(driver)


class ScipyGenerator(GenericPythonGenerator):

    def __init__(self, libraryName, signatures, imports, fromImports):
        super().__init__(libraryName, signatures, imports, fromImports)


    # add code to suppress benign warnings when trying to import scipy
    # special functions; see
    # https://stackoverflow.com/questions/40845304/runtimewarning-numpy-dtype-size-changed-may-indicate-binary-incompatibility
    def write_harness_shared(self):
        with open(self.driverFilePath, 'a') as f:
            f.write("import warnings\n")
            f.write('warnings.filterwarnings("ignore", message="numpy.dtype size changed")\n')
            f.write('warnings.filterwarnings("ignore", message="numpy.ufunc size changed")\n')
        super().write_harness_shared()


    def save_test_migration_inputs(self, extracted_args, driver, TEST_INPUTS):
        return super().save_test_migration_inputs(extracted_args, driver, TEST_INPUTS)
        
    def generate(self, DRIVER_LIST, TEST_INPUTS):
        return super().generate(DRIVER_LIST, TEST_INPUTS)

    def construct_driver_object(self, funcNameWithArgNo, numberOfInts, numberOfParameters):
        return super().construct_driver_object(funcNameWithArgNo, numberOfInts, numberOfParameters)
        
    def form_names(self, funcNameWithArgNo, numberOfInts):
        return super().form_names(funcNameWithArgNo, numberOfInts)

    def write_harness_begin(self, driver):
        return super().write_harness_begin(driver)

    def write_harness_pre_call(self, driver):
        return super().write_harness_pre_call(driver)

    def write_harness_func_call(self, driver):
        return super().write_harness_func_call(driver)

    def write_harness_end(self, driver):
        return super().write_harness_end(driver)


def jmat_generator(libraryName, DRIVER_LIST, signatures, imports, fromImports, TEST_INPUTS):
    import shutil
    # export jmat special functions from jmat.min.js
    shutil.copy("/usr/local/lib/jmat/jmat.min.js", "/usr/local/lib/jmat/jmat.min.fpdiff.js")
    with open("/usr/local/lib/jmat/jmat.min.fpdiff.js", 'a') as minFile:
        for funcName, args in signatures.items():
            exportStmt = "module.exports." + funcName + " = " + "Jmat." + funcName + ";\n"
            minFile.write(exportStmt)
            #for a varying number of integers...
            for numberOfInts in range(len(args)):
                # get the number of doubles
                numberOfDoubles = len(args) - numberOfInts
                # form driverName
                driverName = "{}_{}_DRIVER{}".format(libraryName, funcName.replace('.', '_'), numberOfInts)

                # construct driver object
                thisDriver = header.JmatDriver(
                    driverName=driverName,
                    funcName=funcName,
                    libraryName=libraryName,
                    language="javascript",
                    numberOfParameters=len(args),
                    callName="Jmat."+funcName,
                    numberOfDoubles=numberOfDoubles,
                    numberOfInts=numberOfInts
                )
                DRIVER_LIST[thisDriver.get_driverName()] = thisDriver


if __name__ == "__main__":
    # python3 driverGenerator mpmath python

    libraryName = sys.argv[1]
    language = sys.argv[2]

    try:
        with open("__temp/__driverCollection", 'rb') as fp:
            DRIVER_LIST = pickle.load(fp)
    except:
        DRIVER_LIST = {}

    try:
        with open("__temp/__testInputs", 'rb') as fp:
            TEST_INPUTS = pickle.load(fp)
    except:
        TEST_INPUTS = {}

    # load information from signature extractor
    with open("__temp/__" + libraryName + "_signatures", 'rb') as fp:
        signatures = pickle.load(fp)
    with open("__temp/__" + libraryName + "_imports", 'rb') as fp:
        imports = pickle.load(fp)
    with open("__temp/__" + libraryName + "_fromImports", 'rb') as fp:
        fromImports = pickle.load(fp)

    if language == 'c':

        # initialize a generator
        if libraryName == "gsl":
            my_generator = GSLGenerator(libraryName, signatures, imports, fromImports)
        else:
            my_generator = GenericCGenerator(libraryName, signatures, imports, fromImports)

        my_generator.generate(DRIVER_LIST, TEST_INPUTS)

        # call makefile on generated test harness source code to
        # compile into shared library callable from python
        with open(os.devnull, 'w') as FNULL:
            subprocess.call(['make'], cwd="spFunDrivers/", stdout=FNULL)

    elif language == 'python':

        # initialize a generator
        if libraryName == "mpmath":
            my_generator = MpmathGenerator(libraryName, signatures, imports, fromImports)
        elif libraryName == "scipy":
            my_generator = ScipyGenerator(libraryName, signatures, imports, fromImports)
        else:
            my_generator = GenericPythonGenerator(libraryName, signatures, imports, fromImports)
        
        my_generator.generate(DRIVER_LIST, TEST_INPUTS)

    elif language == 'javascript':
        jmat_generator(libraryName, DRIVER_LIST, signatures, imports, fromImports, TEST_INPUTS)

    with open("__temp/__testInputs", 'wb') as fp:
        pickle.dump(TEST_INPUTS, fp)

    with open("__temp/__driverCollection", 'wb') as fp:
        pickle.dump(DRIVER_LIST, fp)