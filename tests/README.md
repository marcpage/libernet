# Testing strategy

We use `pytest` test to the code. 
In every module we try to maximize test coverage (using `coverage`).
Generally `test_{module}.py` will try to test the file by the `{module}` name in `libernet`.
Often tests will cover other modules as well.

Top-level files will not have coverage as testing command-line parameters and servers can be difficult.
Platform-specific code may not be covered on all platforms.
Extereme error-handling may not be easily covered.
Other than these cases, we should attempt to get 100% coverage in each of the files in `libernet`.
