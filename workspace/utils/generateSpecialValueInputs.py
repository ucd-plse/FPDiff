import random
import struct
import numpy as np
import subprocess
import pickle
import copy

NUMBER_OF_INPUTS = 8



def generate(TEST_INPUTS, ELEMENTARY_INPUTS):

    specialVals = {'inf': np.inf, 'negInf': np.NINF, 'negZero' : np.NZERO, 'nan' : np.nan, 'zero' : np.PZERO}
    ints = copy.deepcopy(ELEMENTARY_INPUTS[list(ELEMENTARY_INPUTS.keys())[0]])[1]

    for name, val in specialVals.items():
        for pos in range(NUMBER_OF_INPUTS):
            doubles = copy.deepcopy(ELEMENTARY_INPUTS[list(ELEMENTARY_INPUTS.keys())[pos]])[0]
            doubles[pos] = val

            for i in range(len(set(ints))):
                TEST_INPUTS["all_{}Input_pos{}_{}".format(name, pos, i)] = (doubles, ints[i:] + ints[:i])


if __name__ == "__main__":
    with open("__temp/__testInputs", 'rb') as f:
        TEST_INPUTS = pickle.load(f)

    with open("__temp/__elementaryInputs", 'rb') as f:
        ELEMENTARY_INPUTS = pickle.load(f)

    generate(TEST_INPUTS, ELEMENTARY_INPUTS)

    with open("__temp/__testInputs", 'wb') as f:
        pickle.dump(TEST_INPUTS, f)