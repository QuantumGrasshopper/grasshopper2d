
# declare variables
CXX = g++
#CXX = clang++-9
LD = $(CXX)
# CXXFLAGS = -g -pg -O1 -Wall -std=c++17
# LDFLAGS = -g -pg -lgsl -lgslcblas -lm
CXXFLAGS = -O3 -Wall -std=c++17
LDFLAGS = -lgsl -lgslcblas -lm

OBJS = grasshopper.o common.o

# link .o-files to program
grasshopper:  $(OBJS)
	$(LD) $(OBJS) -o grasshopper $(LDFLAGS)

# create .o-files from .cpp-files using g++
grasshopper.o: grasshopper.cpp common.hpp
	$(CXX) $(CXXFLAGS) -c grasshopper.cpp
common.o: common.cpp common.hpp
	$(CXX) $(CXXFLAGS) -c common.cpp


# clean up
.PHONY: clean tidy
clean:
	rm -f *~ *.o
tidy:	clean
	rm -f grasshopper
