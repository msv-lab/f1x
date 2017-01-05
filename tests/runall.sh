#!/usr/bin/env bash

#  This file is part of f1x.
#  Copyright (C) 2016  Sergey Mechtaev, Gao Xiang, Abhik Roychoudhury
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
            args="--files program.c --driver $test/test.sh --tests n1 p1 p2 --test-timeout 1000"
            ;;
        assign-in-condition)
            args="--files program.c --driver $test/test.sh --tests n1 p1 p2 --test-timeout 1000"
            ;;
        guarded-assignment)
            args="--files program.c --driver $test/test.sh --tests n1 p1 p2 --test-timeout 1000"
            ;;
        int-assignment)
            args="--files program.c --driver $test/test.sh --tests n1 p1 p2 --test-timeout 1000"
            ;;
        dangling-else)
            args="--files program.c --driver $test/test.sh --tests n1 p1 p2 --test-timeout 1000"
            ;;
        break)
            args="--files program.c --driver $test/test.sh --tests n1 p1 p2 --test-timeout 1000"
            ;;
        array-subscripts)
            args="--files program.c --driver $test/test.sh --tests n1 p1 p2 --test-timeout 1000"
            ;;
        cast-expr)
            args="--files program.c --driver $test/test.sh --tests n1 p1 p2 --test-timeout 1000"
            ;;
        return)
            args="--files program.c --driver $test/test.sh --tests n1 --test-timeout 1000"
            ;;
        deletion)
            args="--files program.c:11 --driver $test/test.sh --tests n1 n2 n3 --test-timeout 1000"
            ;;
        *)
            echo "command for test $test is not defined"
            exit 1
            ;;
    esac

    repair_cmd="f1x $test $args --output output.patch"

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

    rm -rf "$repair_dir"

    echo 'PASS'
done

echo "----------------------------------------"
echo "f1x passed the tests"
echo "----------------------------------------"
