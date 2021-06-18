#!/bin/bash

echo Running tests...
echo The test succeed if there are no diffs printed.
echo

for filename in tests/*.img; do
    test_num=`echo $filename | cut -d'.' -f1`
    ./sim_main ${filename} > ${test_num}.YoursOut
done

for filename in tests/*_output; do
    test_num=`echo $filename | cut -d'_' -f1`
    diff_result=$(diff ${test_num}_output ${test_num}.YoursOut)
    if [ "$diff_result" != "" ]; then
        echo The test ${test_num} didnt pass
    fi
done

echo
echo Ran all tests.