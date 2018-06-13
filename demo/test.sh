#!/bin/bash

assert-equal () {
    diff -q <(echo $1 | ./sort) <(echo -ne "$2") > /dev/null
}

case "$1" in
    long)
        assert-equal '5 4 7 2 1 9 3 6 10 8' '1 2 3 4 5 6 7 8 9 10 '
        ;;
    short)
        assert-equal '3 2 1' '1 2 3 '
        ;;
    random)
        assert-equal '1 2 3' '4 5 6'
        ;;
    *)
        exit 1
        ;;
esac
