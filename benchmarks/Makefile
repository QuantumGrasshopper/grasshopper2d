
# declare variables
CXX = g++
#CXX = clang++-9
LD = $(CXX)
# CXXFLAGS = -g -pg -O1 -Wall -std=c++17
# LDFLAGS = -g -pg -lgsl -lgslcblas -lm
CXXFLAGS = -O3 -Wall -std=c++17
LDFLAGS = -lgsl -lgslcblas -lm

OBJS = grasshopper.o common.o

STORAGE_BENCH_SOURCE = grasshopper_storage_benchmark.cpp
STORAGE_BENCHMARKS = grasshopper_storage_vla \
	grasshopper_storage_unique \
	grasshopper_storage_vector

# link .o-files to program
grasshopper:  $(OBJS)
	$(LD) $(OBJS) -o grasshopper $(LDFLAGS)

# create .o-files from .cpp-files using g++
grasshopper.o: grasshopper.cpp common.hpp
	$(CXX) $(CXXFLAGS) -c grasshopper.cpp
common.o: common.cpp common.hpp
	$(CXX) $(CXXFLAGS) -c common.cpp

.PHONY: storage-benchmarks
storage-benchmarks: $(STORAGE_BENCHMARKS)

grasshopper_storage_vla: $(STORAGE_BENCH_SOURCE) common.o common.hpp
	$(CXX) $(CXXFLAGS) -DSTORAGE_VLA=1 $(STORAGE_BENCH_SOURCE) common.o -o $@ $(LDFLAGS)

grasshopper_storage_unique: $(STORAGE_BENCH_SOURCE) common.o common.hpp
	$(CXX) $(CXXFLAGS) -DSTORAGE_UNIQUE_PTR=1 $(STORAGE_BENCH_SOURCE) common.o -o $@ $(LDFLAGS)

grasshopper_storage_vector: $(STORAGE_BENCH_SOURCE) common.o common.hpp
	$(CXX) $(CXXFLAGS) -DSTORAGE_VECTOR=1 $(STORAGE_BENCH_SOURCE) common.o -o $@ $(LDFLAGS)


# clean up
.PHONY: clean tidy
clean:
	rm -f *~ *.o $(STORAGE_BENCHMARKS)
tidy:	clean
	rm -f grasshopper
