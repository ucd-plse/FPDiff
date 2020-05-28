import os
import subprocess
import pickle
import sys
import itertools
import pdb
import traceback
import tempfile

# insert parent directory into sys.path
sys.path.insert(1, os.path.join(sys.path[0], '..'))

from header import *

# set working directory
WD = os.path.dirname(os.path.abspath(__file__)) + "/../"
os.chdir(WD)

def generatePrograms(CLASSES):

    # for each equivalence class, we're going to generate a set of drivers compatible with s3fp
    for classNo, equivalenceClass in enumerate(CLASSES.values()):

        # make directory to hold all drivers in the equivalence class and copy in settings file
        subprocess.call(["mkdir", "spFunDrivers/s3fpDrivers/{:03d}_class".format(classNo)])
        subprocess.call(["cp", "spFunDrivers/s3fpDrivers/s3fp_setting", "spFunDrivers/s3fpDrivers/{:03d}_class/".format(classNo)])


        for driver in equivalenceClass:

            if driver.get_lang() == "c":

                # copy Makefile
                subprocess.call(["cp", "spFunDrivers/s3fpDrivers/Makefile", "spFunDrivers/s3fpDrivers/{:03d}_class/".format(classNo)])

                with open("spFunDrivers/s3fpDrivers/{:03d}_class/".format(classNo) + driver.get_driverName() + ".c", 'w') as outFile:
                    outFile.write("#include <gsl/gsl_sf.h>\n")
                    outFile.write("#include <gsl/gsl_errno.h>\n")
                    outFile.write("#include <stdio.h>\n\n")
                    outFile.write("void my_handler(const char * reason, const char * file, int line, int gsl_errno)\n")
                    outFile.write("{\n")
                    outFile.write("\tfprintf(stderr, \"%s\\n\", reason);\n")
                    outFile.write("}\n\n")
                    outFile.write(driver.get_driverText())

                    outFile.write("\nint main(int argc, char * argv[]){\n")
                    outFile.write("\tdouble doubles[10];\n")
                    outFile.write("\tdouble temp;\n")
                    outFile.write("\tint ints[] = {1, 2, 3, 1, 2, 3};\n")
                    outFile.write('\tFILE * doubleFile = fopen(argv[1], "r");\n')
                    outFile.write('\tfor(int i = 0; i < 10; i++){\n')
                    outFile.write('\t\tfread(&temp, sizeof(double), 1, doubleFile);\n')
                    outFile.write("\t\tdoubles[i] = (double)temp;\n")
                    outFile.write('\t}\n')
                    outFile.write('\tfclose(doubleFile);\n')
                    outFile.write('\ttemp = {}(doubles, ints);\n'.format(driver.get_driverName()))
                    outFile.write('\tFILE * outFile = fopen(argv[2], "w");\n')
                    outFile.write('\tfwrite(&temp, sizeof(double), 1, outFile);\n')
                    outFile.write('\tfclose(outFile);\n')
                    outFile.write('\treturn 0;\n')
                    outFile.write('}')
    
                with open(os.devnull, 'w') as FNULL:
                    subprocess.call(["make"], cwd="spFunDrivers/s3fpDrivers/{:03d}_class/".format(classNo), stdout=FNULL)


            if driver.get_lang() == "python":

                # gather imports
                with open("__temp/__{}_imports".format(driver.get_libraryName()), 'rb') as fp:
                    imports = pickle.load(fp)
                with open("__temp/__{}_fromImports".format(driver.get_libraryName()), 'rb') as fp:
                    fromImports = pickle.load(fp)

                with open("spFunDrivers/s3fpDrivers/{:03d}_class/".format(classNo) + driver.get_driverName() + ".py", 'w') as outFile:
                    
                    # write all imports
                    for x in imports:
                        if len(x) == 1:
                            outFile.write("import {}\n".format(x[0]))
                        if len(x) == 2:
                            outFile.write("import {} as {}\n".format(x[0], x[1]))
                    for x in fromImports:
                        outFile.write("from {} import {}\n".format(x[0], x[1]))

                    outFile.write("import sys\n")

                    outFile.write(driver.get_driverText())

                    outFile.write("if __name__ == '__main__':\n")
                    outFile.write("\timport struct\n")
                    outFile.write("\tdoubles = []\n")
                    outFile.write("\twith open(sys.argv[1], 'rb') as doubleFile:\n")
                    outFile.write("\t\ttemp = doubleFile.read(8)\n")
                    outFile.write("\t\twhile temp:\n")
                    outFile.write("\t\t\ttemp = struct.unpack('d', temp)[0]\n")
                    outFile.write("\t\t\tdoubles.append(temp)\n")
                    outFile.write("\t\t\ttemp = doubleFile.read(8)\n")
                    outFile.write("\tints = [1, 2, 3, 1, 2, 3]\n")
                    outFile.write("\tresult = {}(doubles, ints)\n".format(driver.get_driverName()))
                    outFile.write("\twith open(sys.argv[2], 'wb') as outFile:\n")
                    outFile.write("\t\toutFile.write(struct.pack('d', result))")


def runS3FP(DRIVERS):

    jobParams = []
    
    subprocess.call(["rm", "logs/s3fp_log.txt"], cwd=WD)
    classDirs = [f for f in os.listdir(WD + "spFunDrivers/s3fpDrivers/") if "_class" in f]
    classNo = 0

    for dir in classDirs:

        tempCWD = WD + "spFunDrivers/s3fpDrivers/" + dir
        allFiles = [f for f in os.listdir(tempCWD) if ".py" in f]
        allFiles.extend([f[:f.index('.')] for f in os.listdir(tempCWD) if ".c" in f])
        
        # prepare each s3fp execution's workspace and parameters
        for pair in itertools.combinations(allFiles, 2):

            driver1_callable = sorted(pair)[0]
            driver2_callable = sorted(pair)[1]

            if "." in driver1_callable:
                driver1_name = driver1_callable[:driver1_callable.index('.')]
            else:
                driver1_name = driver1_callable
            if "." in driver2_callable:
                driver2_name = driver2_callable[:driver2_callable.index('.')]
            else:
                driver2_name = driver2_callable

            if DRIVERS[driver1_name].get_lang() == "python":
                driver1_callable = "python3 ../" + driver1_callable
            else:
                driver1_callable = "../" + driver1_callable
            if DRIVERS[driver2_name].get_lang() == "python":
                driver2_callable = "python3 ../" + driver2_callable
            else:
                driver2_callable = "../" + driver2_callable

            testDirectory = driver1_name + "_" + driver2_name
            subprocess.call(["mkdir", testDirectory], cwd=tempCWD)
            subprocess.call(["cp", "s3fp_setting", "./" + testDirectory], cwd=tempCWD)

            jobParams.append({  
                                "CWD" : tempCWD + "/" + testDirectory,
                                "driver1_name": driver1_name,
                                "driver2_name": driver2_name,
                                "driver1_callable" : driver1_callable,
                                "driver2_callable" : driver2_callable
                            })

            # with subprocess.Popen(["../../../utils/s3fp/s3fp",
            #                     "64",
            #                     str(DRIVERS[driver1_name].get_numberOfDoubles()),
            #                     testDirectory + "/test_input",
            #                     driver1_callable,
            #                     testDirectory + "/output1",
            #                     driver2_callable,
            #                     testDirectory + "/output2",
            #                     ], cwd=tempCWD,
            #                     stdout=subprocess.PIPE,
            #                     bufsize=1) as p, \
            #                         open("logs/s3fp_log.txt", 'a') as f:
            #                         for line in p.stdout:
            #                             if str(line).find("Best") != -1:
            #                                 relErr = str(line).split(' ')[3]
            #                                 if relErr.find("N/A") != -1:
            #                                     relErr = "N/A"
                                            
            #                             f.write(str(line)[2:-3]+"\n")
            #                         f.write("\n")

            # sortedDrivername = sorted([driver1_name, driver2_name])

            # subprocess.call(["mv", "best_input", "../../../savedInputs/{}_{}~s3fpInput".format(sortedDrivername[0], sortedDrivername[1])], cwd=tempCWD)

    processes = []
    for batch in chunks(jobParams, 10):
        for job in batch:
            f = tempfile.TemporaryFile(mode='w+')
            p = subprocess.Popen(
                                    [
                                        "../../../../utils/s3fp/s3fp",
                                        "64",
                                        str(DRIVERS[job["driver1_name"]].get_numberOfDoubles()),
                                        "test_input",
                                        job["driver1_callable"],
                                        "output1",
                                        job["driver2_callable"],
                                        "output2",
                                    ],

                                    cwd=job["CWD"],
                                    stdout=f,
                                    stderr=f
                                )
            processes.append((p,f))
        for p, _ in processes:
            p.wait()

    for _, f in processes:
        f.seek(0)
        with open("logs/s3fp_log.txt", 'a') as outfile:
            outfile.write(f.read())
        f.close()

    for job in jobParams:
        subprocess.call(["mv", "best_input", "../../../../savedInputs/{}_{}~s3fpInput".format(job["driver1_name"], job["driver2_name"])], cwd=job["CWD"])

def chunks(jobList, n):
    for i in range(0, len(jobList), n):
        yield jobList[i:i+n]

if __name__ == "__main__":

    try:
        # load equivalence classes
        with open("__temp/__equivalenceClasses", 'rb') as fp:
            CLASSES = pickle.load(fp)

        # load drivers
        with open("__temp/__driverCollection", 'rb') as fp:
            DRIVERS = pickle.load(fp)

        # clean out s3fp class directories and compile s3fp tool
        with open(os.devnull, 'w') as FNULL:
            subprocess.call(["make", "classClean"], cwd=WD + "spFunDrivers/s3fpDrivers/", stdout=FNULL)
            subprocess.call(["make"], cwd=WD + "utils/s3fp/", stdout=FNULL)

        generatePrograms(CLASSES)
        runS3FP(DRIVERS)

    except:
        extype, value, tb = sys.exc_info()
        traceback.print_exc()
        pdb.post_mortem(tb)