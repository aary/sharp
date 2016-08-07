`sharp`
-------

This repository contains code for some of the libraries that I have
implemented in my free time.

### Building

The libraries in this project support building with Buck.  To install buck run
the following

```
brew update
brew tap facebook/fb
brew install buck
```

This repository is arranged in the form of a buck project for ease of
demonstration.  To compile and run an example project run the following

```
buck run example
```

This should compile and run the example `.cpp` file included in the `example/`
folder.

To use this library in another buck project just move the `sharp` folder into
the root of the other project write a `BUCK` file similar to that in the
`example` folder and build with `buck build`

If any issues are found please open an issue on Github.
