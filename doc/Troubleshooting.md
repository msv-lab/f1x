If you encounter a problem5B, please do the following

1. Reproduce the problem in a clean environment (e.g. fresh copy of your project, removed intermediate test data)
2. Execute f1x with `--verbose` option to obtain more datailed information about execution
3. Save the working directory of the failing execution

## Common problems ##

### f1x produces different results for multiple runs with the same inputs  ###

f1x implementation is deternimistic. The sources of non-determinism can be

- Non-determinism of program tests
- Timeouts (check if the total number of timeouts is different for different runs)
