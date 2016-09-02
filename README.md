`sharp`
-------

This repository contains code for some C++ libraries that I have implemented
in my free time.

### Building

The libraries in this project support building with
[Buck](https://buckbuild.com).  To install buck from the latest stable commit
on master, run the following

```
brew update
brew install --HEAD facebook/fb/buck
```

This will install buck from the `HEAD` commit from [Facebook's homebrew
tap](https://github.com/facebook/homebrew-fb).

This repository is arranged in the form of a buck project for ease of
demonstration.  The `example/` directory includes a simple `.cpp` file that
uses this library.  To see the output run the following

```
buck run example
```

This should compile and run the `.cpp` file in the `example/` folder.

To use this library in another buck project just move the `sharp` folder into
the root of the other project write a `BUCK` file similar to that in the
`example` folder and build with `buck build`

If any issues are found please open an issue on Github.

### Running unit tests

[Google test](https://github.com/google/googletest) needs to be present in the
sharp folder to run the unit tests.  It has been included as a git submodule
in this repository.  Run the following to install `gtest` within this project
```
git submodule update --init --recursive
```

Once `gtest` has been installed run all unit tests with `buck`
```
buck test
```

The unit tests for each module are in the `test` folder in the each submodule
like so

```
├── LockedData
│   ├── BUCK
│   └── test
│       ├── test.cpp
├── Optional
│   ├── BUCK
│   └── test
│       ├── test.cpp
```

To run the unit tests individually for a module run
`buck test //sharp/<module_name>/test`.  For example to run unit tests for the
`LockedData` module run `buck test //sharp/LockedData/test`
