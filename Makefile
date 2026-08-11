
# declare variables
CXX = g++
#CXX = clang++-9
LD = $(CXX)
# CXXFLAGS = -g -pg -O1 -Wall -std=c++17
# LDFLAGS = -g -pg -lgsl -lgslcblas -lm
CXXFLAGS = -O3 -Wall -std=c++17
LDFLAGS = -lgsl -lgslcblas -lm
PYTHON ?= python3

OBJS = main.o utilities.o setup.o annealing.o simulation_parameters.o
TEST_BIN = tests/grasshopper_tests
TEST_SRCS = tests/test_main.cpp \
		tests/test_globals.cpp \
		tests/test_utilities.cpp \
		tests/test_annealing.cpp \
		tests/test_setup.cpp \
		tests/test_simulation_parameters.cpp
TEST_OBJS = $(TEST_SRCS:.cpp=.o)
TEST_PRODUCTION_OBJS = utilities.o setup.o annealing.o simulation_parameters.o

# link .o-files to program
grasshopper:  $(OBJS)
	$(LD) $(OBJS) -o grasshopper $(LDFLAGS)

$(TEST_BIN): $(TEST_OBJS) $(TEST_PRODUCTION_OBJS)
	$(LD) $(TEST_OBJS) $(TEST_PRODUCTION_OBJS) -o $(TEST_BIN) $(LDFLAGS)

# create .o-files from .cpp-files using g++
main.o: main.cpp utilities.hpp setup.hpp annealing.hpp simulation_parameters.hpp
	$(CXX) $(CXXFLAGS) -c main.cpp
utilities.o: utilities.cpp utilities.hpp
	$(CXX) $(CXXFLAGS) -c utilities.cpp
setup.o: setup.cpp setup.hpp utilities.hpp
	$(CXX) $(CXXFLAGS) -c setup.cpp
annealing.o: annealing.cpp annealing.hpp utilities.hpp
	$(CXX) $(CXXFLAGS) -c annealing.cpp
simulation_parameters.o: simulation_parameters.cpp simulation_parameters.hpp
	$(CXX) $(CXXFLAGS) -c simulation_parameters.cpp

tests/%.o: tests/%.cpp tests/doctest/doctest.h utilities.hpp setup.hpp annealing.hpp simulation_parameters.hpp
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
