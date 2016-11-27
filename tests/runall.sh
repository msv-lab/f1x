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
    case "$test" in
        if-condition)
            args="--files program.c --tests 1 2 3"
            ;;
    esac

    dir=`mktemp -d`
    cp -r "$test" "$dir"
    (
        cd $dir
        f1x "$test" $args --output output.patch &> log.txt
        status=$?
        if [[ ($status != 0) || (! -f output.patch) ]]; then
            echo ""
            echo "failed test: $test"
            echo "directory: $dir"
            echo "command: f1x $test $args --output output.patch"
            exit 1
        fi
    )
    subshell=$?
    if [[ ($subshell != 0) ]]; then
        exit 1
    fi
    rm -rf "$dir"
    echo -n '.'
done

echo ""
echo "f1x passed the tests"
