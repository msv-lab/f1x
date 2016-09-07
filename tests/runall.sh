#!/usr/bin/env bash

cd "$( dirname "${BASH_SOURCE[0]}" )"

rm -rf minifix-patches-*

ALL_TESTS=`ls -d */`

for test in $ALL_TESTS; do
    echo $test
    minifix $test --files program.c --tests p1 p2 n1 --generic-runner $test/test.sh
    rm -rf minifix-patches-*
done


