
# declare variables
CXX = g++
#CXX = clang++-9
LD = $(CXX)
# CXXFLAGS = -g -pg -O1 -Wall -std=c++17
# LDFLAGS = -g -pg -lgsl -lgslcblas -lm
CXXFLAGS = -O3 -Wall -std=c++17
LDFLAGS = -lgsl -lgslcblas -lm
PYTHON ?= python3

OBJS = main.o interactions.o output.o setup.o annealing.o parameters.o
TEST_BIN = tests/grasshopper_tests
TEST_SRCS = tests/test_main.cpp \
		tests/test_globals.cpp \
		tests/test_interactions.cpp \
		tests/test_output.cpp \
		tests/test_annealing.cpp \
		tests/test_setup.cpp \
		tests/test_parameters.cpp
TEST_OBJS = $(TEST_SRCS:.cpp=.o)
TEST_PRODUCTION_OBJS = interactions.o output.o setup.o annealing.o parameters.o

# link .o-files to program
grasshopper:  $(OBJS)
	$(LD) $(OBJS) -o grasshopper $(LDFLAGS)

$(TEST_BIN): $(TEST_OBJS) $(TEST_PRODUCTION_OBJS)
	$(LD) $(TEST_OBJS) $(TEST_PRODUCTION_OBJS) -o $(TEST_BIN) $(LDFLAGS)

# create .o-files from .cpp-files using g++
main.o: main.cpp utilities.hpp interactions.hpp output.hpp setup.hpp annealing.hpp parameters.hpp
	$(CXX) $(CXXFLAGS) -c main.cpp
interactions.o: interactions.cpp interactions.hpp utilities.hpp
	$(CXX) $(CXXFLAGS) -c interactions.cpp
output.o: output.cpp output.hpp
	$(CXX) $(CXXFLAGS) -c output.cpp
setup.o: setup.cpp setup.hpp utilities.hpp interactions.hpp output.hpp
	$(CXX) $(CXXFLAGS) -c setup.cpp
annealing.o: annealing.cpp annealing.hpp utilities.hpp
	$(CXX) $(CXXFLAGS) -c annealing.cpp
parameters.o: parameters.cpp parameters.hpp
	$(CXX) $(CXXFLAGS) -c parameters.cpp

tests/%.o: tests/%.cpp tests/doctest/doctest.h utilities.hpp interactions.hpp output.hpp setup.hpp annealing.hpp parameters.hpp
	$(CXX) $(CXXFLAGS) -I. -Itests -c $< -o $@

test: $(TEST_BIN)
	./$(TEST_BIN)

integration-test: grasshopper
	$(PYTHON) tests/run_integration_tests.py ./grasshopper

check: test integration-test

# clean up
.PHONY: clean tidy test integration-test check
clean:
	rm -f *~ *.o $(TEST_OBJS) $(TEST_BIN)
tidy:	clean
	rm -f grasshopper
