import pickle
import re
from pprint import pprint

print("\n\n**TABLE 2\n\n")

with open("../../frozenState/ground_truth/table2/gsl_signatures_GT.pckl", 'rb') as f:
    gsl_signatures_GT = pickle.load(f)

with open("../../frozenState/ground_truth/table2/scipy_signatures_GT.pckl", 'rb') as f:
    scipy_signatures_GT = pickle.load(f)
    
with open("../../frozenState/ground_truth/table2/jmat_signatures_GT.pckl", 'rb') as f:
    jmat_signatures_GT = pickle.load(f)
    
with open("../../frozenState/ground_truth/table2/mpmath_signatures_GT.pckl", 'rb') as f:
    mpmath_signatures_GT = pickle.load(f)

with open("../../frozenState/full_logs/__extractedSignatures.txt", "r") as f:
    lines = f.readlines()

gsl_found = [x.strip().split(":")[0] for x in lines if "gsl_" in x]
gsl_found = [x[x.find("gsl_"):-1] for x in gsl_found]

scipy_found = [x.strip().split(":")[0] for x in lines if "special." in x]
scipy_found = [x[x.find("special."):-1] for x in scipy_found]

mpmath_found = []
for line in lines:
    if "{'Arg_arg2'" in line:
        break
    elif ":" in line:
        mpmath_found.append(line)
mpmath_found = [x.strip().split(":")[0] for x in mpmath_found]
mpmath_found = [x[x.find("'") + 1 : -1] for x in mpmath_found]

jmat_found = []

lines.reverse()
for line in lines:
    if "gsl" in line:
        break
    elif ":" in line:
        jmat_found.append(line)

lines.reverse()
jmat_found = [x.strip().split(":")[0] for x in jmat_found]
jmat_found = [x[x.find("'") + 1 : -1] for x in jmat_found]

print("\n============= GSL =============")
not_found=[]
total=len(gsl_signatures_GT)
for sig in gsl_signatures_GT:
    if sig not in gsl_found:
        not_found.append(sig)
        
found = total - len(not_found)
print("Found {}/{} = {}".format(found, total, found/total * 100))
print("Missing signatures: ")
pprint(not_found)

print("\n============= SciPy =============")

not_found=[]
total=len(scipy_signatures_GT)
for sig in scipy_signatures_GT:
    if sig not in scipy_found:
        not_found.append(sig)
        
found = total - len(not_found)
print("Found {}/{} = {}".format(found, total, found/total * 100))
print("Missing signatures: ")
pprint(not_found)


print("\n============= mpmath =============")

not_found=[]
total=len(mpmath_signatures_GT)
for sig in mpmath_signatures_GT:
    if sig not in mpmath_found:
        not_found.append(sig)
        
found = total - len(not_found)
print("Found {}/{} = {}".format(found, total, found/total * 100))
print("Missing signatures: ")
pprint(not_found)


print("\n============= jmat =============")

not_found=[]
total=len(jmat_signatures_GT)
for sig in jmat_signatures_GT:
    if sig not in jmat_found:
        not_found.append(sig)
        
found = total - len(not_found)
print("Found {}/{} = {}".format(found, total, found/total * 100))
print("Missing signatures: ")
pprint(not_found)


print("\n\n**TABLE 3\n\n")




with open("../../frozenState/ground_truth/table3/mappings_GT.pckl", 'rb') as f:
    all_mappings_GT = pickle.load(f)

with open("../../frozenState/full_logs/__driverMappings.txt", "r") as f:
    lines = f.readlines()

for i in range(len(lines)):
    if ": [" in lines[i]:
        lines[i] = lines[i].split(": [")[1]
    elif ":" in lines[i]:
        lines[i] = lines[i].split(":")[1]
    elif "]"in lines[i]:
        lines[i] = lines[i].split("]")[0]
    lines[i] = lines[i].strip().replace(",","").replace("'","")
    lines[i] = re.sub(r'_DRIVER\d?', '', lines[i])
    
all_found_mappings = lines

missing_mappings = list(set(all_mappings_GT).difference(set(all_found_mappings)))
missing_number = len(missing_mappings)
total_number = len(all_mappings_GT)
found_number = total_number - missing_number

print("\n============= TOTAL =============")
print("Recall: {}/{} = {}".format(found_number, total_number, found_number/total_number))
print("Missing mappings:")
pprint(missing_mappings)

found_mappings = {
    "gsl_gsl" : [],
    "gsl_jmat" : [],
    "gsl_mpmath" : [],
    "gsl_scipy" : [],
    "jmat_jmat" : [],
    "jmat_mpmath" : [],
    "jmat_scipy" : [],
    "mpmath_mpmath" : [],
    "mpmath_scipy" : [],
    "scipy_scipy" : []
}

GT_mappings = {
    "gsl_gsl" : [],
    "gsl_jmat" : [],
    "gsl_mpmath" : [],
    "gsl_scipy" : [],
    "jmat_jmat" : [],
    "jmat_mpmath" : [],
    "jmat_scipy" : [],
    "mpmath_mpmath" : [],
    "mpmath_scipy" : [],
    "scipy_scipy" : []
}

for x in all_found_mappings:
    lib1 = x[:x.find("_")]
    lib2 = x[x.find("/")+1:]
    lib2 = lib2[:lib2.find("_")]
    libs = sorted([lib1, lib2])
    libs = "{}_{}".format(libs[0],libs[1])
    found_mappings[libs].append(x)
    
for x in all_mappings_GT:
    lib1 = x[:x.find("_")]
    lib2 = x[x.find("/")+1:]
    lib2 = lib2[:lib2.find("_")]
    libs = sorted([lib1, lib2])
    libs = "{}_{}".format(libs[0],libs[1])
    GT_mappings[libs].append(x)  

stats = {}
for library_pair in list(found_mappings.keys()):
    missing_mappings = list(set(GT_mappings[library_pair]).difference(set(found_mappings[library_pair])))
    missing_number = len(missing_mappings)
    total_number = len(GT_mappings[library_pair])
    total_found = total_number-missing_number
    stats[library_pair] = ("Recall: {}/{} = {}".format(total_found, total_number, total_found/total_number), missing_mappings)
    
pprint(stats)

extra_mappings = list(set(all_found_mappings).difference(set(all_mappings_GT)))
extra_mappings = [x for x in extra_mappings if "mpmath_fp" not in x]
print("\n============= UNVERIFIED MAPPINGS =============")
pprint(extra_mappings)