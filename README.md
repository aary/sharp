`sharp`
-------

This repository contains code for some of the libraries that I have
implemented in my free time that are misssing in the standard library.

### Building

All the components included in this library will work right out of the box
with no external dependencies like boost.  Only the C++ standard library is
needed.  Only 2 things need to be kept in mind while compiling

* Compilation with C++14 or above is a strict requirement.
* The header only libraries (like template modules) might need their
  respective cpp files to be included in the compilation.  So examining the
  folder with the library needed will give a hint, if there is a cpp file then
  it should be compiled with the rest as well.
* Any library within the `sharp` library should be included by providing its
  fully qualified path relative from inside the `sharp/` directory.  So for
  example

    `#include "Tags/Tags.hpp"`

  And any compilation command should include the root `sharp` directory in the
  included directories to be searched for headers with the `-I` compiler flag
  like so

    `g++ -std=c++14 -I ../sharp`

  Assuming that the `sharp` folder is in the directory one level up from the
  current working directory.
