#!/bin/bash

generated_logs=$(dirname $(realpath "${0}"))/workspace/logs

if [ $# -eq 0 ]; then
    echo "Checking generated statistics.txt against expected results..."

    if test -f "${generated_logs}"/__extractedSignatures.txt; then
        expected_statistics=$(dirname $(realpath "${0}"))/frozenState/full_logs/statistics.txt
    else
        expected_statistics=$(dirname $(realpath "${0}"))/frozenState/subset_logs/statistics.txt
    fi

    out=$(diff -x '.gitignore' "${expected_statistics}" "${generated_logs}/statistics.txt")

    if [[ "${out}" ]]; then
        echo -e "\t[!] The following differences were observed:"
        echo "${out}" | while read line; do
            if [[ "${line}" == *"<"* ]]; then
                read -r -a temp <<< ${line}
                echo -e "\tEXPECTED  : ${temp[@]:1}"
            elif [[ "${line}" == ">"* ]]; then
                read -r -a temp <<< ${line}
                echo -e "\tGENERATED : ${temp[@]:1}"
            elif [[ "${line}" =~ [0-9]+(c)[0-9]+ ]]; then
                echo -e ""
            fi
        done
    else
        echo -e "\t[PASS] statistics.txt is identical to the expected statistics.txt"
    fi
fi

if [[ "$*" == *verbose* ]]; then
    echo "Checking generated logs against expected results..."

    if test -f "${generated_logs}"/__extractedSignatures.txt; then
        expected_logs=$(dirname $(realpath "${0}"))/frozenState/full_logs/
    else
        expected_logs=$(dirname $(realpath "${0}"))/frozenState/subset_logs/
    fi

    out=$(diff -x '.gitignore' "${expected_logs}" "${generated_logs}")

    if [[ "${out}" ]]; then
        echo -e "\t[!] The following differences were observed:"
        
        this_log=""

        echo "${out}" | while read line; do

            if [[ "${line}" == "diff"* ]]; then

                IFS='/' read -r -a file1 <<< ${line}
                this_log="${file1[-1]}"
                echo -e "\n\tIn ${this_log}:\n"

            elif [[ "${this_log}" == *".csv" ]]; then

                if [[ "${line}" =~ ^([0-9,])+[a-z]([0-9,]+)$ ]]; then
                    echo -e "\tEXPECTED: "
                elif [[ "${line}" == "---" ]]; then
                    echo -e "\tGENERATED:"
                else
                    echo -e "\t\t${line}"
                fi

            else
                if [[ "${line}" == *"<"* ]]; then
                    read -r -a temp <<< ${line}
                    echo -e "\tEXPECTED  : ${temp[@]:1}"
                elif [[ "${line}" == ">"* ]]; then
                    read -r -a temp <<< ${line}
                    echo -e "\tGENERATED : ${temp[@]:1}"
                elif [[ "${line}" =~ [0-9]+(c)[0-9]+ ]]; then
                    echo -e ""
                fi
            fi
        done
    else
        echo -e "\t[PASS] All logs are identical to those expected"
    fi
fi

echo
echo "Checking for paper examples in generated logs..."

examples=(
    discrepancyNo,84158476b467087661551d71e0601aae,reducedDiffTestingResults.csv
    discrepancyNo,64185faeeed48d4ead067d29d2c29343,reducedDiffTestingResults.csv
    discrepancyNo,253319312b81faac256c91080b348dce,reducedDiffTestingResults.csv
    discrepancyNo,2a4001efcc956759a8c8aa671adf8660,reducedDiffTestingResults.csv
    discrepancyNo,2c0bf95be8f8338ac9765024546c0749,reducedDiffTestingResults.csv
    discrepancyNo,dd258915e38973f68f699a46179c2559,__diffTestingResults.csv
    classKey,24.44048792293956,equivalenceClasses.csv
)

for i in "${examples[@]}"; do
    IFS=","
    set -- ${i}
    out=$(awk "/${2}/ {print NR; exit;}" "${generated_logs}"/${3})
    if [[ "${out}" ]]; then
        echo -e "\t[PASS] ${1} ${2} is present in ${3}, lineNo ${out}"
    else
        echo -e "\t[FAIL] ${1} ${2} is NOT present in ${3}"
    fi
done