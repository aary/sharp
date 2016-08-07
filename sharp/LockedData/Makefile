# The compiler to use
CXX	= g++
# Default Flags
CXXFLAGS = -std=c++14 -Wall -Werror -Wextra -pedantic -Wvla -O3
# Test flags
CXXTESTFLAGS = -DTEST
# Link to libraries
LIBRARIES = -I ../../

# run tests
tests:
	@$(CXX) $(CXXFLAGS) $(CXXTESTFLAGS) $(LIBRARIES) \
		tests/LockedDataTests.cpp -I ./
	@./a.out
	@rm a.out
	@echo "All tests passed!"

# make clean - remove .o files, executable, tarball
clean:
	rm -f *.o a.out

.PHONY: tests
