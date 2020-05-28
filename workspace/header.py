import ctypes
import os
import sys
import threading
import time
import csv
import subprocess
import signal
import warnings
import multiprocessing
import itertools
import importlib
import numpy as np
from abc import ABC, abstractmethod


# set working directory
WD = os.path.dirname(os.path.abspath(__file__))
os.chdir(WD)

# put drivers directory in path
sys.path.insert(0, './spFunDrivers')

'''
The following constants are used throughout the pipeline
'''

# input category macros
POSITIVE_INPUT = 0
NEGATIVE_INPUT = 1
ZERO_INPUT = 2
INF_INPUT = 3
NAN_INPUT = 4

# discrepancy category macros
TIMEOUT = 6
DOUBLES_FROM_NAN = 5    
DOUBLES_FROM_INF = 4
INACCURACY = 3
MIX_OF_DOUBLES_EXCEPTIONS_SPECIAL_VALUES = 2
MIX_OF_EXCEPTIONS_SPECIAL_VALUES = 1

# Threshold for determining when a function execution should be
# determined a "TIMEOUT"
TIMELIMIT = 20

# Respective relative tolerance values for classifier and diffTester
CLASS_TOLERANCE = 0.00000001
ERROR_TOLERANCE = 0.001


class TemplateDriver(ABC):

    def __init__(self, driverName, funcName, libraryName, language, numberOfParameters, callName, numberOfDoubles, numberOfInts):
        self.driverName = driverName
        self.funcName = funcName
        self.libraryName = libraryName
        self.language = language
        self.driverText = ""
        self.classificationOutputs = []
        self.exceptionMessages = []
        self.callableDriver = None
        self.numberOfDoubles = numberOfDoubles
        self.numberOfInts = numberOfInts
        self.numberOfParameters = numberOfParameters
        self.callName = callName

    @abstractmethod
    def run_driver(self, doubles, ints):
        pass     

    # to be implemented for drivers that import test harnesses and run
    # them natively from Python code.
    def set_callableDriver(self):
        pass

    def reset_callableDriver(self):
        self.callableDriver = None

    def get_driverName(self):
        return self.driverName

    def get_callName(self):
        return self.callName

    def get_funcName(self):
        return self.funcName

    def get_numberOfParameters(self):
        return self.numberOfParameters

    def get_numberOfInts(self):
        return self.numberOfInts

    def get_numberOfDoubles(self):
        return self.numberOfDoubles

    def get_numericalArgNo(self):
        return self.numberOfDoubles + self.numberOfInts

    def add_classificationOutput(self, output):
        self.classificationOutputs.append(output)

    def get_classificationOutputs(self):
        return self.classificationOutputs

    def get_id(self):
        return sum(self.get_classificationOutputs()) * self.get_numericalArgNo()

    def add_exceptionMessage(self, message):
        self.exceptionMessages.append(message)

    def get_exceptionMessages(self):
        return self.exceptionMessages

    def get_libraryName(self):
        return self.libraryName


class GenericCDriver(TemplateDriver):

    def __init__(self, driverName, funcName, libraryName, language, numberOfParameters, callName, numberOfDoubles, numberOfInts):
        super().__init__(driverName, funcName, libraryName, language, numberOfParameters, callName, numberOfDoubles, numberOfInts)


    def run_driver(self, doubles, ints):

        self.set_callableDriver()

        # convert list of inputs to c types
        c_doubles = (ctypes.c_double * len(doubles))(*doubles)
        c_ints = (ctypes.c_int * len(ints))(*ints)

        # redirect stderr to out
        out = OutputGrabber(sys.stderr)
        out.start()

        # run driver, keep an eye out for timeouts
        result = 0
        recv_end, send_end = multiprocessing.Pipe(False)
        p = multiprocessing.Process(target=lambda send_end, arg1, arg2: send_end.send(self.callableDriver(arg1, arg2)), args=(send_end, c_doubles, c_ints))
        p.start()
        if recv_end.poll(TIMELIMIT):
            result = recv_end.recv()
        else:
            result = "EXCEPTION: TIMEOUT"
            p.terminate()
        out.stop()

        # if out captured an error, log an exception
        if out.capturedtext != '':
            dic = []
            errText = out.capturedtext.split("\n")
            errInfo = ""
            for text in errText:
                if text not in dic:
                    dic.append(text)
            for text in dic:
                if text != '':# and text.find("gsl_sf_") == -1:
                    errInfo += text + " "
            result = "EXCEPTION: " + errInfo

        self.reset_callableDriver()

        return result


    def set_callableDriver(self):
        c_module = ctypes.CDLL('./spFunDrivers/{}_drivers.so'.format(self.get_libraryName()))
        self.callableDriver = getattr(locals()["c_module"], self.driverName)
        self.callableDriver.restype = ctypes.c_double


class GenericPythonDriver(TemplateDriver):

    def __init__(self, driverName, funcName, libraryName, language, numberOfParameters, callName, numberOfDoubles, numberOfInts):
        super().__init__(driverName, funcName, libraryName, language, numberOfParameters, callName, numberOfDoubles, numberOfInts)


    def run_driver(self, doubles, ints):

        self.set_callableDriver()

        # catch warnings and throw exceptions instead
        warnings.simplefilter('error')
        
        try:
            # run the test
            signal.alarm(TIMELIMIT)
            result = self.callableDriver(doubles, ints)
            signal.alarm(0)
        except Exception as e:
            signal.alarm(0)
            if str(e) == '':
                result = "EXCEPTION: " + str(type(e).__name__)
            else:
                result = "EXCEPTION: " + str(e)       

        self.reset_callableDriver()
        return result


    def set_callableDriver(self):
        
        python_module = importlib.import_module("{}_drivers".format(self.get_libraryName()))
        self.callableDriver = getattr(locals()["python_module"], self.get_driverName())


class JmatDriver(TemplateDriver):

    def __init__(self, driverName, funcName, libraryName, language, numberOfParameters, callName, numberOfDoubles, numberOfInts):
        super().__init__(driverName, funcName, libraryName, language, numberOfParameters, callName, numberOfDoubles, numberOfInts)


    def run_driver(self, doubles, ints):
        try:
            warnings.simplefilter('error')

            # generate input string
            doubleString = ""
            intString = ""
            for num in doubles:
                if np.isnan(num):
                    doubleString+="NaN"
                elif num == np.inf:
                    doubleString+="Infinity"
                elif num == -np.inf:
                    doubleString+="-Infinity"
                else:
                    doubleString+="{}".format(num, '.64f')
                doubleString+=","
            for num in ints:
                intString+="{}".format(num, '.64f')
                intString+=","
            doubleString = doubleString[:-1]
            intString = intString[:-1]

            # call node.js to run javascript code
            signal.signal(signal.SIGALRM, self.timeoutHandler)
            signal.alarm(TIMELIMIT)
            cmd = subprocess.Popen(["node", "jmat_driver.js", self.funcName, doubleString, intString, 'i'*self.numberOfInts+'d'*self.numberOfDoubles], cwd="spFunDrivers/", stdout=subprocess.PIPE)
            signal.alarm(0)

            # convert output to float
            cmd_out, cmd_err = cmd.communicate()
            outputString = cmd_out.decode("utf-8")
            try:
                result = np.float(outputString)
            except:
                result = outputString

            return result


        except Exception as e:
            signal.alarm(0)
            if str(e) == '':
                return "EXCEPTION: " + str(type(e).__name__)
            else:
                return "EXCEPTION: " + str(e)

    '''
    called when a call to a driver exceeds the TIMELIMIT
    '''
    def timeoutHandler(self, num, stack):
        raise Exception("TIMEOUT")


class Discrepancy:

    def __init__(self, classKey, inputName, inputTuple, driverOutputPairs):
        self.classKey = classKey
        
        self.inputName = inputName

        self.inputList = []
        for i in range(driverOutputPairs[0][0].get_numberOfInts()):
            self.inputList.append(inputTuple[1][i])
        for i in range(driverOutputPairs[0][0].get_numberOfDoubles()):
            self.inputList.append(inputTuple[0][i])
        
        self.driverOutputPairs = driverOutputPairs
        self.outputList = [f[1] for f in driverOutputPairs]

        self.abstractedInput = self.categorizeInput(self.inputList)
        self.discrepancyCategory = self.categorizeDiscrepancy(self.abstractedInput, self.outputList)
        self.id = (self.classKey, self.abstractedInput, self.discrepancyCategory)

        self.discrepancyNo = 0

        self.absErrs = {}
        self.relErrs = {}
        self.maxRelErr = None
        self.maxDeltaDiff = self.calculate_maxDeltaDiff()

        if self.discrepancyCategory == INACCURACY:
            for pair in itertools.combinations(self.driverOutputPairs, 2):
                pairName = pair[0][0].get_driverName() + "/" + pair[1][0].get_driverName()
                absErr = abs(pair[0][1] - pair[1][1])
                relErr = 2 * absErr / max((abs(pair[0][1]) + abs(pair[1][1])), CLASS_TOLERANCE)
                
                self.absErrs[pairName] = absErr
                self.relErrs[pairName] = relErr

            self.maxRelErr = max(list(self.relErrs.values()))
            for x,y in self.relErrs.items():
                if y == self.maxRelErr:
                    if "_fp_" in x:
                        return None


    def get_maxDeltaDiff(self):
        return self.maxDeltaDiff


    def calculate_maxDeltaDiff(self):

        # initialize 
        score = {}
        for driverOutputPair in self.get_driverOutputPairs():
            score[driverOutputPair[0].get_driverName()] = 0

        # calculate delta diffs
        for pair in itertools.combinations(self.driverOutputPairs, 2):
            if not isConsistent([pair[0][1], pair[1][1]]):
                score[pair[0][0].get_driverName()] += 1
                score[pair[1][0].get_driverName()] += 1

        return max(list(score.values()))


    def set_discrepancyNo(self, x):
        self.discrepancyNo = x

    def get_discrepancyNo(self):
        return self.discrepancyNo

    def get_maxRelErr(self):
        return self.maxRelErr

    def get_id(self):
        return self.id

    def get_driverOutputPairs(self):
        return self.driverOutputPairs

    def get_inputName(self):
        return self.inputName

    def get_discrepancyCategory(self):
        return self.discrepancyCategory
    
    def get_inputList(self):
        return self.inputList

    def get_classKey(self):
        return self.classKey

    def get_absErrs(self):
        if self.discrepancyCategory == INACCURACY:
            return self.absErrs
        else:
            return None

    def get_relErrs(self):
        if self.discrepancyCategory == INACCURACY:
            return self.relErrs
        else:
            return None

    def categorizeDiscrepancy(self, abstractedInput, outputList):

        doubleTally = 0
        exceptionTally = 0
        specialValueTally = 0

        for output in outputList:
            if isinstance(output, str):
                exceptionTally += 1
                
                if "TIMEOUT" in output:
                    return TIMEOUT

            elif np.isfinite(output):
                doubleTally += 1
            
            else:
                specialValueTally += 1

        if doubleTally > 0:

            if doubleTally == len(outputList) and NAN_INPUT not in abstractedInput and INF_INPUT not in abstractedInput:
                return INACCURACY

            elif NAN_INPUT in abstractedInput:
                return DOUBLES_FROM_NAN
                
            elif INF_INPUT in abstractedInput:
                return DOUBLES_FROM_INF

            elif (exceptionTally > 0 or specialValueTally > 0):
                return MIX_OF_DOUBLES_EXCEPTIONS_SPECIAL_VALUES

        elif doubleTally == 0:
            if (exceptionTally > 0 or specialValueTally > 0):
                return MIX_OF_EXCEPTIONS_SPECIAL_VALUES

        else:
            raise Exception("UNCLASSIFIABLE DISCREPANCY")

    def categorizeInput(self, inputList):

        abstractedInput = []

        for x in inputList:
            if np.isnan(x):
                abstractedInput.append(NAN_INPUT)
            elif np.isinf(x):
                abstractedInput.append(INF_INPUT)
            elif x == 0:
                abstractedInput.append(ZERO_INPUT)
            elif x < 0:
                abstractedInput.append(NEGATIVE_INPUT)
            elif x > 0:
                abstractedInput.append(POSITIVE_INPUT)
            else:
                raise Exception("UNCLASSIFIABLE INPUT")

        return tuple(abstractedInput)


'''
Class used to grab standard output or another stream.
In this context, used by GSL drivers to capture
error messages thrown by our custom GSL error handler 
'''
class OutputGrabber(object):

    escape_char = "\b"

    def __init__(self, stream=None, threaded=False):
        self.origstream = stream
        self.threaded = threaded
        if self.origstream is None:
            self.origstream = sys.stdout
        self.origstreamfd = self.origstream.fileno()
        self.capturedtext = ""
        # Create a pipe so the stream can be captured:
        self.pipe_out, self.pipe_in = os.pipe()

    def __enter__(self):
        self.start()
        return self

    def __exit__(self, type, value, traceback):
        self.stop()

    def start(self):
        """
        Start capturing the stream data.
        """
        self.capturedtext = ""
        # Save a copy of the stream:
        self.streamfd = os.dup(self.origstreamfd)
        # Replace the original stream with our write pipe:
        os.dup2(self.pipe_in, self.origstreamfd)
        if self.threaded:
            # Start thread that will read the stream:
            self.workerThread = threading.Thread(target=self.readOutput)
            self.workerThread.start()
            # Make sure that the thread is running and os.read() has executed:
            time.sleep(0.01)

    def stop(self):
        """
        Stop capturing the stream data and save the text in `capturedtext`.
        """
        # Print the escape character to make the readOutput method stop:
        self.origstream.write(self.escape_char)
        # Flush the stream to make sure all our data goes in before
        # the escape character:
        self.origstream.flush()
        if self.threaded:
            # wait until the thread finishes so we are sure that
            # we have until the last character:
            self.workerThread.join()
        else:
            self.readOutput()
        # Close the pipe:
        os.close(self.pipe_in)
        os.close(self.pipe_out)
        # Restore the original stream:
        os.dup2(self.streamfd, self.origstreamfd)
        # Close the duplicate stream:
        os.close(self.streamfd)

    def readOutput(self):
        """
        Read the stream data (one byte at a time)
        and save the text in `capturedtext`.
        """
        while True:
            char = os.read(self.pipe_out, 1).decode(self.origstream.encoding)
            if not char or self.escape_char in char:
                break
            self.capturedtext += char


'''
function to sort csv file by the value in the first
column; intended to facilitate quick comparisons of
logs from different pipeline runs.
'''
def sort_csv(csvFileName):

    with open("logs/{}".format(csvFileName), newline='') as f:
        reader = csv.DictReader(f)
        sortedlist = sorted(reader, key=lambda row: row[reader.fieldnames[0]], reverse=False)

    subprocess.call(["rm", "-f", "logs/{}".format(csvFileName)])

    with open("logs/{}".format(csvFileName), 'w') as f:
        writer = csv.DictWriter(f, fieldnames=reader.fieldnames)
        writer.writeheader()
        for row in sortedlist:
            writer.writerow(row)


'''
function that checks a set of outputs for a discrepancy;
returns a boolean
'''
def isConsistent(outputs):
    
    exceptionCount = 0

    # count up the number of exceptions
    for output in outputs:
        if isinstance(output, str):
            exceptionCount += 1
            if "TIMEOUT" in output:
                return False
                
    # if all drivers threw exceptions, consider them consistent
    if exceptionCount == len(outputs):
        return True

    # if there weren't any exceptions...
    elif exceptionCount == 0:

        # if they're all nan, return True            
        if np.isnan(outputs).prod():
            return True

        # if they're all positive infinity, return True
        elif np.isposinf(outputs).prod():
            return True

        # if they're all negative infinity, return True
        elif np.isneginf(outputs).prod():
            return True

        # if they're all negative zero, return True
        elif np.array([x is np.NZERO for x in outputs]).prod():
            return True

        # otherwise, if they're all proper doubles...
        elif np.isfinite(outputs).prod():

            # check whether the double outputs are within the
            # specified epsilon neighborhood
            for pair in itertools.combinations(outputs, 2):

                # if they're both zero, move on
                if abs(pair[0]) + abs(pair[1]) == 0:
                    continue

                # otherwise, if the relative error is greater than
                # TOLERANCE, return false
                elif 2 * abs(pair[0] - pair[1]) / (abs(pair[0]) + abs(pair[1])) > ERROR_TOLERANCE:
                    return False
                
                # otherwise, move on
                else:
                    continue

            # if we made it through the accuracy check, return True
            return True

        else:
            return False

    # otherwise, it's a mix of exceptions and other values, so return False
    else:
        return False
