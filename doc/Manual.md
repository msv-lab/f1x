# Manual #

## Usage ##
    
Before using f1x:

- Create a copy of your project (f1x can corrupt your source files)
- Configure your project using f1x compiler wrapper (e.g. `CC=f1x-cc ./configure`)
- Clean binaries (e.g. `make clean`)

Example:

    $ f1x grep-1.4.5/src/ -d tests/run.sh -t 1 2 3
    run time:     0:12:54
    total execs:  1401
    exec speed:   5.4/sec
    evaluated:    120042
