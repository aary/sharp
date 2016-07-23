# The compiler to use
CXX	= g++
# Default Flags
CXXFLAGS = -std=c++14 -Wall -Werror -Wextra -pedantic -Wvla -O3

# run tests
tests:
	$(CXX) $(CXXFLAGS) tests/UniqueLockedProxy_tests.cpp -I ./

# make clean - remove .o files, executable, tarball
clean:
	rm -f *.o a.out

.PHONY: tests
