#!/usr/bin/env bash

require () {
    hash "$1" 2>/dev/null || { echo >&2 "command $1 is not found"; exit 1; }
}

require f1x

cd "$( dirname "${BASH_SOURCE[0]}" )"

if [[ -z "$TESTS" ]]; then
    TESTS=`ls -d */`
fi

for test in $TESTS; do
    test=${test%%/} # removing slash in the end
    echo -n "* testing $test... "
    case "$test" in
        if-condition)
            args="--files program.c --tests 1 2 3 --test-timeout 1000"
            ;;
    esac

    dir=`mktemp -d`
    cp -r "$test" "$dir"
    (
        cd $dir
        f1x "$test" $args --output output.patch &> log.txt
        status=$?
        if [[ ($status != 0) || (! -f output.patch) || (! -s output.patch) ]]; then
            echo 'FAIL'
            echo "----------------------------------------"
            echo "failed test: $test"
            echo "directory: $dir"
            echo "command: f1x $test $args --output output.patch"
            echo "----------------------------------------"
            exit 1
        fi
    )
    subshell=$?
    if [[ ($subshell != 0) ]]; then
        exit 1
    fi
    rm -rf "$dir"
    echo 'PASS'
done

echo "----------------------------------------"
echo "f1x passed the tests"
echo "----------------------------------------"
