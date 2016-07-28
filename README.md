`sharp`
-------

This repository contains code for some of the libraries that I have
implemented in my free time.

### Building

All the components included in this library will work right out of the box
with no external dependencies like boost.  Only the C++ standard library is
needed.  Only 2 things need to be kept in mind while compiling

* Compilation with C++14 or above is a strict requirement.

* There are some header only files in this library.  To work with them
  simply include the `.hpp` file in your code.  For others there may be `.cpp`
  as a part of the library.  These may have to be compiled into `.a` files
  that should be linked with the rest of your code.  To do so, go into the
  root `sharp` folder and type in the following to build all the components of
  the library

    `make all`

  This will place a `.a` file in the root `sharp` directory that should then
  be linked with your code like so

    `g++ -std=c++14 yourmain.cpp sharp/sharplib.a`

* Any library within the `sharp` library should be included by providing its
  fully qualified path relative from the folder that contains `sharp/`
  directory.  So for example

    `#include "sharp/Tags/Tags.hpp"`

  And any compilation command should include the folder that contains the
  `sharp` directory in the included directories to be searched for headers
  with the `-I` compiler flag like so

    `g++ -std=c++14 -I ./`

  Assuming that the `sharp` folder is in the current working directory.

If any issues are found please open an issue on Github.
