`sharp`
-------

This repository contains code for some C++ libraries that I have implemented
in my free time.

## Building

The libraries in this project support building with
[Buck](https://buckbuild.com).  To install buck from the latest stable commit
on master, run the following

```
brew install facebook/fb/buck
```

This will install buck with homebrew using [Facebook's homebrew
tap](https://github.com/facebook/homebrew-fb).

This repository is arranged in the form of a buck module.  The recommended
way of installing this library into your project is by using  git submodules.

```
git submodule add https://github.com/aary/sharp
git submodule update --init --recursive
```

I would recommend cloning the example project I have
[here](https://github.com/aary/sharp-example) to see how a project would use
`sharp` and build with buck.

To use this with buck simply add a `[repositories]` section in your top level
`.buckconfig` as follows

```
[repositories]
    sharp = ./sharp
```

Then build with buck by adding a dependency of the following form
`sharp//:sharp`.  For example the `deps` section in the `BUCK` file for your
project can look like so

```
deps = [
    "sharp//:sharp",
],
```

and the corresponding include statement in your C++ file would be

```
#include <sharp/Concurrent/Concurrent.hpp>
```

If any bugs are found please open an issue on
[Github](https://github.com/aary/sharp).

## Running unit tests

[Google test](https://github.com/google/googletest) needs to be present in the
sharp folder to run the unit tests.  It has been included as a git submodule
in this repository.  Run the following to install `gtest` within this project
```
git submodule update --init --recursive
```

Once `gtest` has been installed `cd` into the `sharp` folder and run all unit
tests with `buck`
```
buck test ...
```

To run the unit tests individually for a module run `buck test
<module_name>/...` from the `sharp` folder.  For example to run unit tests for
the `Concurrent` module run `buck test Concurrent/...`
