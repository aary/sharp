# The compiler to use
CXX	= g++
# Default Flags
CXXFLAGS = -std=c++14 -Wall -Werror -Wextra -pedantic -Wvla -O3

# run tests
tests:
	$(CXX) $(CXXFLAGS) tests/UniqueLockedProxy_tests.cpp -I ./
	@./a.out
	@rm a.out
	$(CXX) $(CXXFLAGS) tests/LockedDataTests.cpp -I ./
	@./a.out
	@rm a.out

# make clean - remove .o files, executable, tarball
clean:
	rm -f *.o a.out

.PHONY: tests
