
# declare variables
CXX = g++
#CXX = clang++-9
LD = $(CXX)
# CXXFLAGS = -g -pg -O1 -Wall -std=c++17
# LDFLAGS = -g -pg -lgsl -lgslcblas -lm
CXXFLAGS = -O3 -Wall -std=c++17
LDFLAGS = -lgsl -lgslcblas -lm

OBJS = main.o utilities.o setup.o annealing.o

# link .o-files to program
grasshopper:  $(OBJS)
	$(LD) $(OBJS) -o grasshopper $(LDFLAGS)

# create .o-files from .cpp-files using g++
main.o: main.cpp utilities.hpp setup.hpp annealing.hpp
	$(CXX) $(CXXFLAGS) -c main.cpp
utilities.o: utilities.cpp utilities.hpp
	$(CXX) $(CXXFLAGS) -c utilities.cpp
setup.o: setup.cpp setup.hpp utilities.hpp
	$(CXX) $(CXXFLAGS) -c setup.cpp
annealing.o: annealing.cpp annealing.hpp utilities.hpp
	$(CXX) $(CXXFLAGS) -c annealing.cpp

# clean up
.PHONY: clean tidy
clean:
	rm -f *~ *.o
tidy:	clean
	rm -f grasshopper
