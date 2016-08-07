# The gtest library to use for buck test
cxx_library(
    name = 'gtest',
    srcs = glob(['googletest/googletest/src/*.cc'],
      excludes=['googletest/googletest/src/gtest-all.cc']),

    # Not all compilers support <tr1/tuple>, so have gtest use it's
    # internal implementation.
    exported_preprocessor_flags = [
      '-DGTEST_USE_OWN_TR1_TUPLE=1',
    ],

    # the subdir_glob() call below evaluates to
    #   {
    #       "src/gtest-internal-inl.h"
    #           : "googletest/googletest/src/gtest-internal-inl.h",
    #   }
    #
    # which means that the source files being compiled in this library can
    # include the gtest-internal-inl.h by doing
    #
    #   #include <src/gtest-internal-inl.h>
    #
    # check https://buckbuild.com/function/subdir_glob.html for more details
    headers = subdir_glob([
        ("googletest/googletest", "**/*.h"),
    ]),

    # the headers to export to the outside world for including
    exported_headers = subdir_glob([
        ('googletest/googletest/src', '*.h'),
        ('googletest/googletest/include', '**/*.h'),
    ]),

    # make the exported_headers publicly available
    visibility = [
      'PUBLIC',
    ],
)
