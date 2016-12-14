#!/usr/bin/env bash

#  This file is part of f1x.
#  Copyright (C) 2016  Sergey Mechtaev, Abhik Roychoudhury
#
#  f1x is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.

require () {
    hash "$1" 2>/dev/null || { echo "command $1 is not found"; exit 1; }
}

require f1x
require make
require patch

cd "$( dirname "${BASH_SOURCE[0]}" )"

if [[ -z "$TESTS" ]]; then
    TESTS=`ls -d */`
fi

for test in $TESTS; do
    test=${test%%/} # removing slash in the end
    echo -n "* testing $test... "
    case "$test" in
        if-condition)
            subject_files="program.c"
            subject_driver="$PWD/$test/test.sh"
            subject_tests="1 2 3"
            subject_args="--test-timeout 1000"
            ;;
        assign-in-condition)
            subject_files="program.c"
            subject_driver="$PWD/$test/test.sh"
            subject_tests="1 2 3"
            subject_args="--test-timeout 1000"
            ;;
        guarded-assignment)
            subject_files="program.c"
            subject_driver="$PWD/$test/test.sh"
            subject_tests="1 2 3"
            subject_args="--test-timeout 1000"
            ;;
        int-assignment)
            subject_files="program.c"
            subject_driver="$PWD/$test/test.sh"
            subject_tests="1 2 3"
            subject_args="--test-timeout 1000"
            ;;
        *)
            echo "command for test $test is not defined"
            exit 1
            ;;
    esac

    repair_cmd="f1x $test --files $subject_files --tests $subject_tests --driver $subject_driver --output output.patch $subject_args"

    repair_dir=`mktemp -d`
    cp -r "$test" "$repair_dir"
    (
        cd $repair_dir
        $repair_cmd &> log.txt
        repair_status=$?
        if [[ ($repair_status != 0) || (! -f output.patch) || (! -s output.patch) ]]; then
            echo 'FAIL'
            echo "----------------------------------------"
            echo "repair directory: $repair_dir"
            echo "failed command: $repair_cmd"
            echo "----------------------------------------"
            exit 1
        fi
    )
    repair_subshell=$?
    if [[ ($repair_subshell != 0) ]]; then
        exit 1
    fi

    validation_dir=`mktemp -d`
    cp -r "$test" "$validation_dir"
    (
        cd $validation_dir
        cd $test

        patch -p1 < $repair_dir/output.patch &> /dev/null
        application_status=$?
        if [[ $application_status != 0 ]]; then
            echo 'FAIL'
            echo "----------------------------------------"
            echo "repair directory: $repair_dir"
            echo "repair command: $repair_cmd"
            echo "validation directory: $validation_dir"
            echo "failed command: patch -p0 < $repair_dir/output.patch"
            echo "----------------------------------------"
            exit 1
        fi

        make &> /dev/null
        compilation_status=$?
        if [[ $compilation_status != 0 ]]; then
            echo 'FAIL'
            echo "----------------------------------------"
            echo "repair directory: $repair_dir"
            echo "repair command: $repair_cmd"
            echo "validation directory: $validation_dir"
            echo "failed command: make"
            echo "----------------------------------------"
            exit 1
        fi

        for current_id in $subject_tests; do
            $subject_driver $current_id &> /dev/null
            validation_status=$?
            if [[ ($validation_status != 0) ]]; then
                echo 'FAIL'
                echo "----------------------------------------"
                echo "repair directory: $repair_dir"
                echo "repair command: $repair_cmd"
                echo "validation directory: $validation_dir"
                echo "failed command: $subject_driver $current_id"
                echo "----------------------------------------"
                exit 1
            fi
        done
    )
    validation_subshell=$?
    if [[ ($validation_subshell != 0) ]]; then
        exit 1
    fi

    rm -rf "$repair_dir"
    rm -rf "$validation_dir"

    echo 'PASS'
done

echo "----------------------------------------"
echo "f1x passed the tests"
echo "----------------------------------------"
