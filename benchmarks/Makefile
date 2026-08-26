CXX ?= g++
CXXFLAGS ?= -O3 -std=c++17
LDFLAGS ?=
LDLIBS ?= -lgsl -lgslcblas -lm

BENCHMARK_CPPFLAGS = -I..
NEIGHBOR_BENCHMARK = grasshopper_neighbor_benchmark
NEIGHBOR_BENCHMARK_OBJS = grasshopper_neighbor_benchmark.o interactions_benchmark.o

BENCHMARKS = \
	grasshopper_relative_neighbors \
	grasshopper_neighbor_template

COMMON_OBJ = interactions_benchmark.o

.PHONY: all active-benchmarks clean

all: active-benchmarks

active-benchmarks: $(BENCHMARKS)

$(BENCHMARKS): %: %.o $(COMMON_OBJ)
	$(CXX) $(LDFLAGS) $^ -o $@ $(LDLIBS)

%.o: %.cpp ../interactions.hpp ../utilities.hpp
	$(CXX) $(CPPFLAGS) $(BENCHMARK_CPPFLAGS) $(CXXFLAGS) -c $< -o $@

interactions_benchmark.o: ../interactions.cpp ../interactions.hpp ../utilities.hpp
	$(CXX) $(CPPFLAGS) $(BENCHMARK_CPPFLAGS) $(CXXFLAGS) -c $< -o $@

clean:
	$(RM) *.o $(BENCHMARKS)
