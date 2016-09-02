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

This repository is arranged in the form of a buck submodule.  The recommended
way of installing this library into your project is in the form of a git
submodule.

```
git submodule add https://github.com/aary/sharp
```

This command will install the library into your git project as a submodule.
To use it with buck simply add a `[repositories]` section in your top level
`.buckconfig` as follows

```
[repositories]
    sharp = ./sharp
```

Then add it as a dependency with the following form `sharp//<library>` and
build with `buck build`.  Very simple examples of using this library are in
another repository [here](https://github.com/aary/sharp-example).

If any bugs are found please open an issue on the
[Github](https://github.com/aary/sharp).

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
`buck test //sharp/<module_name>/test` from the `sharp` folder.  For example
to run unit tests for the `LockedData` module run `buck test
//sharp/LockedData/test`
