import pickle
import os
import sys
import ast
import subprocess
import re
from pprint import pprint

# set working directory
WD = os.path.dirname(os.path.abspath(__file__))
os.chdir(WD)

class GenericPythonExtractor(ast.NodeVisitor):
   
    def __init__(self, libraryName):
        self.libraryName = libraryName
        self.signatures = {}
        self.variableDecls = {}
        self.imports = set()
        self.fromImports = set()
        self.FUNC_IGNORE = []
        self.testFileRegEx = ".*test.*\.py$"
        super().__init__()

    def extract(self, rootDirectoryPath):

        # recursively collect all files
        files = [(p, f) for p, _ , f in os.walk(rootDirectoryPath)]

        for path, fileNames in sorted(files, key=lambda x : x[0]):
            for name in sorted(fileNames):
                if re.search(self.testFileRegEx, name):
                    with open(os.path.join(path,name), "r") as source:
                        tree = ast.parse(source.read())
                    self.visit(tree)

        return self.get_signatures(), self.get_imports(), self.get_fromImports()

    def modify_funcName(self, funcName):
        return funcName

    def get_signatures(self):
        return self.signatures

    def get_imports(self):
        return self.imports

    def get_fromImports(self):
        return self.fromImports

    # collect variable assignments
    def visit_Assign(self, node):

        variableNames = [f.id for f in node.targets if isinstance(f, ast.Name)]

        if isinstance(node.value, ast.Num) and not isinstance(node.value.n, complex):
            value = node.value.n
            for name in variableNames:
                self.variableDecls[name] = value
            
        ast.NodeVisitor.generic_visit(self, node)

    # collect function signatures
    def visit_Call(self, node):

        argList = []
        funcName = None

        # for func calls without a namespace...
        if isinstance(node.func, ast.Name):
            funcName = node.func.id

        # for func calls with a namespace...
        elif isinstance(node.func, ast.Attribute) and isinstance(node.func.value, ast.Name):
            funcName = node.func.value.id + '.' + node.func.attr

        if funcName:

            # collect arguments
            for x in node.args:

                # collect non-complex literals:
                if isinstance(x, ast.Num) and not isinstance(x.n, complex):
                    argList.append(x.n)
            
                # collect references to non-complex literals:
                elif isinstance(x, ast.Name):
                    if x.id in self.variableDecls.keys():
                        argList.append(self.variableDecls[x.id])

                # collect negative non-complex literals and references:
                elif isinstance(x, ast.UnaryOp) and isinstance(x.op, ast.USub):
                    if isinstance(x.operand, ast.Num) and not isinstance(x.operand.n, complex):
                        argList.append(x.operand.n * -1)

                    elif isinstance(x.operand, ast.Name):
                        if x.operand.id in self.variableDecls.keys():
                            argList.append(self.variableDecls[x.operand.id] * -1)

                # if there is an argtype we don't handle, reset argList to empty and break
                else:
                    argList = []
                    break

            if argList != [] and funcName not in self.FUNC_IGNORE:

                funcName = self.modify_funcName(funcName)

                # tag signature with argList length
                funcName = funcName + '_arg{}'.format(len(argList))

                # save new func signatures
                if funcName not in self.signatures.keys():
                    self.signatures[funcName] = [argList]

                # save new args for previously found signatures
                else:
                    self.signatures[funcName].append(argList)

        # proceed on depth first search
        ast.NodeVisitor.generic_visit(self, node)

    # collect import statements
    def visit_Import(self, node):
        for alias in node.names:
            if self.libraryName in alias.name:
                if alias.asname:
                    self.imports.add((alias.name, alias.asname))
                else:
                    self.imports.add((alias.name))

        ast.NodeVisitor.generic_visit(self, node)

    # collect fromImport statements
    def visit_ImportFrom(self,node):
        for alias in node.names:
            if self.libraryName in node.module or self.libraryName in alias.name:
                if alias.asname:
                    self.fromImports.add((node.module, alias.name, alias.asname))
                else:
                    self.fromImports.add((node.module, alias.name))

        ast.NodeVisitor.generic_visit(self, node)


class MpmathExtractor(GenericPythonExtractor):
    def __init__(self, libraryName):
        super().__init__(libraryName)

        # stieltjes from mpmath times out on inputs between 0 and 1
        self.FUNC_IGNORE = ["stieltjes", "print"]

    def modify_funcName(self, funcName):
        # remove precision namespace; mp by default
        if "fp." in funcName or "mp." in funcName:
            return funcName[funcName.index(".") + 1:]
        else:
            return funcName

    def get_imports(self):
        return []

    def get_fromImports(self):
        return [("mpmath", "*")]


class ScipyExtractor(GenericPythonExtractor):
    def __init__(self, libraryName):
        super().__init__(libraryName)

        # ellip_harm from scipy causes seg fault due to integer and double 
        # parameters being in strange order
        self.FUNC_IGNORE = ["ellip_harm"]

    def modify_funcName(self, funcName):
        # remove weird namespaces and focus on special functions
        if "." in funcName:
            return "special" + funcName[funcName.index("."):]
        else:
            return "special." + funcName

    def get_imports(self):
        return []

    def get_fromImports(self):
        return [("scipy", "special")]


class GenericCExtractor():

    def __init__(self, libraryName):
        self.libraryName = libraryName
        self.signatures = {}
        self.imports = set()
        self.fromImports = set()
        self.FUNC_IGNORE = []
        self.ALLOWED_TYPES = ["double",
                            "const double",
                            "const int",
                            "int",
                            "unsigned int",
                            "signed int"]
        self.funcDeclRegEx = "^double\s+.+\(.*double.*\)"
        self.funcTestCallRegEx = ".*(.*)"
        self.headerFileRegEx = ".*h$"
        self.testFileRegEx = ".*test.*\.c$"

    def extract(self, rootDirectoryPath):

        # recursively collect all files
        files = [(p, f) for p, _ , f in os.walk(rootDirectoryPath)]

        # extract signatures from header files
        for path, fileNames in files:
            for name in fileNames:
                if re.search(self.headerFileRegEx, name):
                    self.parse_header(path, name)

        # get test inputs for extracted signatures
        for path, fileNames in files:
            for name in fileNames:                    
                if re.search(self.testFileRegEx, name):
                    self.parse_test_file(path, name)

        return self.get_signatures(), self.get_imports(), self.get_fromImports()

    def get_signatures(self):
        return self.signatures

    def get_imports(self):
        return self.imports

    def get_fromImports(self):
        return self.fromImports

    def parse_header(self, path, headerFileName):
        with open(path + headerFileName, "r") as source:
            code = source.readlines()
        for line in code:
            line = line.strip()
            if re.search(self.funcDeclRegEx, line):
                functionName = self.extract_funcNameFromHeaderLine(line)
                if functionName and functionName not in self.FUNC_IGNORE:

                    # construct list of parameter types
                    paramList = line[line.index('(') + 1 : line.index(')')].split(',')
                    for i in range(len(paramList)):
                        paramList[i] = paramList[i].strip()

                        # remove variable name so we're left with only type
                        while paramList[i][-1].isalpha():
                            paramList[i] = paramList[i][:-1]
                        paramList[i] = paramList[i].strip()

                    if any(x not in self.ALLOWED_TYPES for x in paramList):
                        continue
                    else:            
                        self.signatures[functionName] = [paramList]
                        self.imports.add(headerFileName)

    def extract_funcNameFromHeaderLine(self, lineOfCode):
        try:
            funcName = lineOfCode[lineOfCode.index('double ') + 6 : lineOfCode.index('(')].strip()
            if funcName[0].isalpha():
                return funcName
            else:
                return None
        except ValueError:
            return None 

    def parse_test_file(self, path, testFileName):
        with open(path + testFileName, "r") as source:
            code = source.readlines()
        for line in code:
            if re.search(self.funcTestCallRegEx, line):
                funcNames = self.extract_funcNamesFromTestLine(line)

                for name in funcNames:
                    if name in self.get_signatures().keys():
                        argList = self.extract_argListFromTestLine(name, line)
                        reqdArgsTypes = self.get_signatures()[name][0]

                        if argList != []:
                            failConversion = False
                            for i in range(len(argList)):
                                if "int" in reqdArgsTypes[i]:
                                    try:
                                        argList[i] = int(argList[i])
                                    except ValueError:
                                        failConversion = True
                                if "double" in reqdArgsTypes[i]:
                                    try:
                                        argList[i] = float(argList[i])
                                    except ValueError:
                                        failConversion = True

                            if not failConversion:
                                self.signatures[name].append(argList)

    def extract_funcNamesFromTestLine(self, line):
        return [x[:-1] for x in re.findall('\w*\(', line)]

    def extract_argListFromTestLine(self, funcName, line):
        firstParenIndex = line.index(funcName) + len(funcName)
        
        for i in range(firstParenIndex + 1, len(line)):
            if line[i] == ")":
                return line[firstParenIndex: i+1].split(sep=",")


class GSLExtractor(GenericCExtractor):

    def __init__(self, libraryName):
        super().__init__(libraryName)

        self.FUNC_IGNORE = ["gsl_sf_mathieu_a",
                            "gsl_sf_mathieu_b",
                            "gsl_sf_mathieu_ce",
                            "gsl_sf_mathieu_se",
                            "gsl_sf_mathieu_Mc",
                            "gsl_sf_mathieu_Ms"
                            ]
        self.funcDeclRegEx = "^double\s+gsl_.*\(.*double.*\)"
        self.funcTestCallRegEx = "TEST_SF\("
        self.ALLOWED_TYPES.append("gsl_mode_t")

    def get_imports(self):
        return [("gsl/gsl_sf.h"),("gsl/gsl_errno.h")]

    def extract_funcNamesFromTestLine(self, line):
        # ex line: TEST_SF(s, gsl_sf_hzeta_e, (2,  1.0, &r),  1.6449340668482264365, TEST_TOL0, GSL_SUCCESS);

        line = line.split(sep='(')
        if len(line) < 3:
            return []

        funcName = line[1].split(sep=',')[-2].strip()

        # convert error checking version of function to normal version
        if funcName[-2:] == '_e':
            funcName = funcName[:-2]

        return [funcName]

    def extract_argListFromTestLine(self, funcName, line):
        # ex line: TEST_SF(s, gsl_sf_hzeta_e, (2,  1.0, &r),  1.6449340668482264365, TEST_TOL0, GSL_SUCCESS);

        line = line.split(sep='(')
        if len(line) < 3:
            return []

        # get argList and remove the error handler argument
        argList = line[2].split(sep=')')[0].split(sep=',')
        for i in range(len(argList)):
            argList[i] = argList[i].strip()
        if '&r' in argList:
            argList = argList[:-1]

        return argList


def extract_c_signatures(rootDirectoryPath, libraryName):

    # initialize an extractor
    if libraryName == "gsl":
        myExtractor = GSLExtractor(libraryName)
    else:
        myExtractor = GenericCExtractor(libraryName)

    return myExtractor.extract(rootDirectoryPath)


def extract_python_signatures(rootDirectoryPath, libraryName):

    # initialize an extractor
    if libraryName == "mpmath":
        myExtractor = MpmathExtractor(libraryName)
    elif libraryName == "scipy":
        myExtractor = ScipyExtractor(libraryName)
    else:
        myExtractor = GenericPythonExtractor(libraryName)

    return myExtractor.extract(rootDirectoryPath)


def signature_extractor_js(inFile, libraryName):

    # Find the line number of the start and end of the special functions section
    lookup1 = '// Trigonometric functions'
    lookup2 = 'Matrix (NOTE: more are above: add, sub, mul, div, inv, neg, conj, exp, log, sqrt, cos, sin)'
    with open(inFile, encoding='utf8') as myFile:
        for num, line in enumerate(myFile, 1):
            if lookup1 in line:
                start = num
            if lookup2 in line:
                end = num
    
    # Read lines from start to end
    with open(inFile, "r", encoding='utf8') as file:
        lines = file.readlines()
    lines = lines[start:end]

    # Find all the function definition in special functions section
    for line in lines:
        if re.match("^Jmat\...*=.function\(.*\) {.*", line):
            content = line.split(' ')
            parameters = line[line.find("(")+1:line.find(")")].split(', ')
            name = content[0].split('.')
            signatures[name[1]] = parameters


def getStats(signatures, libraryName):

    funcTotal = len(signatures)

    migratedInputsTotal = 0
    for args in signatures.values():
        migratedInputsTotal += len(args)

    with open("logs/statistics.txt", "a") as f:
        f.write("{}: found {} function signatures and {} test inputs.\n".format(libraryName, funcTotal, migratedInputsTotal))

    with open("logs/__extractedSignatures.txt", "a") as f:
        pprint(signatures, stream=f)


if __name__ == "__main__":
    # python3 extractor.py libraryName path language

    # make the temp directory to hold binary files
    if not os.path.isdir("__temp"):
        subprocess.call(["mkdir", "__temp"])

    # grab command line arguments
    libraryName = sys.argv[1]
    path = sys.argv[2]
    language = sys.argv[3]

    # initialize structures to hold extracted information
    signatures = {}
    imports = []
    fromImports = []

    if language == "c":
        signatures, imports, fromImports = extract_c_signatures(path, libraryName)

    if language == "python":
        signatures, imports, fromImports = extract_python_signatures(path, libraryName)

    if language == "javascript":
        file = path + "jmat.js"
        signature_extractor_js(file, libraryName)

    getStats(signatures, libraryName)
    
    with open("__temp/__" + libraryName + "_signatures", 'wb') as fp:
        pickle.dump(signatures, fp)

    with open("__temp/__" + libraryName + "_imports", 'wb') as fp:
        pickle.dump(imports, fp)

    with open("__temp/__" + libraryName + "_fromImports", 'wb') as fp:
        pickle.dump(fromImports, fp)
