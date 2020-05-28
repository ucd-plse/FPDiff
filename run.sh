#!/bin/bash

containerWD=/usr/local/src/fp-diff-testing/workspace/
hostWD=$(dirname $(realpath $0))

if [ $# -eq 0 ]
    then
        docker run --rm -i -v ${hostWD}/workspace:/usr/local/src/fp-diff-testing/workspace ucdavisplse/sp-diff-testing:artifact bash ${containerWD}runExperiment.sh
fi

if [[ "$*" == *subset* ]]
    then
        pushd workspace && sudo make reset && popd
        cp -r frozenState/subset_testing_files workspace/__temp
        cp workspace/__temp/runTest.sh workspace/
        docker run --rm -i -v ${hostWD}/workspace:/usr/local/src/fp-diff-testing/workspace ucdavisplse/sp-diff-testing:artifact bash ${containerWD}runTest.sh
        rm workspace/runTest.sh
fi

if [[ "$*" == *debug* ]]
    then
        pushd workspace && sudo make reset && popd
        sudo cp -r frozenState/__tempDebug workspace/__temp
        cp workspace/__temp/runTest.sh workspace/
        docker run --rm -i -v ${hostWD}/workspace:/usr/local/src/fp-diff-testing/workspace ucdavisplse/sp-diff-testing:blank-3_10 bash ${containerWD}runTest.sh
        rm workspace/runTest.sh
fi