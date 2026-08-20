
# declare variables
CXX ?= g++
CXXFLAGS ?= -O3 -Wall -Wextra -std=c++17
LDFLAGS ?=
LDLIBS ?= -lgsl -lgslcblas -lm
PYTHON ?= python3

OBJS = main.o interactions.o output.o setup.o annealing.o parameters.o
CORRELATION_BIN = tools/correlations
CORRELATION_OBJ = tools/correlation_functions.o
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
	$(CXX) $(LDFLAGS) $(OBJS) -o grasshopper $(LDLIBS)

$(TEST_BIN): $(TEST_OBJS) $(TEST_PRODUCTION_OBJS)
	$(CXX) $(LDFLAGS) $(TEST_OBJS) $(TEST_PRODUCTION_OBJS) -o $(TEST_BIN) $(LDLIBS)

$(CORRELATION_BIN): $(CORRELATION_OBJ) interactions.o output.o
	$(CXX) $(LDFLAGS) $(CORRELATION_OBJ) interactions.o output.o -o $(CORRELATION_BIN) $(LDLIBS)

# create .o-files from .cpp-files
main.o: main.cpp utilities.hpp interactions.hpp output.hpp setup.hpp annealing.hpp parameters.hpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c main.cpp
interactions.o: interactions.cpp interactions.hpp utilities.hpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c interactions.cpp
output.o: output.cpp output.hpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c output.cpp
setup.o: setup.cpp setup.hpp utilities.hpp interactions.hpp output.hpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c setup.cpp
annealing.o: annealing.cpp annealing.hpp utilities.hpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c annealing.cpp
parameters.o: parameters.cpp parameters.hpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c parameters.cpp
$(CORRELATION_OBJ): tools/correlation_functions.cpp interactions.hpp output.hpp utilities.hpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c tools/correlation_functions.cpp -o $(CORRELATION_OBJ)

tests/%.o: tests/%.cpp tests/doctest/doctest.h utilities.hpp interactions.hpp output.hpp setup.hpp annealing.hpp parameters.hpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -I. -Itests -c $< -o $@

test: $(TEST_BIN)
	./$(TEST_BIN)

correlation-tool: $(CORRELATION_BIN)

integration-test: grasshopper $(CORRELATION_BIN)
	$(PYTHON) tests/run_integration_tests.py ./grasshopper ./$(CORRELATION_BIN)

check: test integration-test

# clean up
.PHONY: clean tidy test correlation-tool integration-test check
clean:
	rm -f *~ *.o $(TEST_OBJS) $(TEST_BIN) $(CORRELATION_OBJ) $(CORRELATION_BIN)
tidy:	clean
	rm -f grasshopper
