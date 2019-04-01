TARGET = smr
TARGET_MPI = $(TARGET)_mpi
LIBS = -lm
CXX = g++
MPICXX = mpicxx
CXXFLAGS = -O3 -march=native -std=c++11 -Wall -Wextra -lpthread
MPICXXFLAGS = $(CXXFLAGS) -DENABLE_MPI -Wno-literal-suffix

default: $(TARGET)
mpi: $(TARGET_MPI)

$(TARGET): *.hpp *.cpp
	$(CXX) $(CXXFLAGS) *.cpp -o $(TARGET)
$(TARGET_MPI): *.hpp *.cpp
	$(MPICXX) $(MPICXXFLAGS) *.cpp -o $(TARGET_MPI)

clean:
	-rm -f *.o
	-rm -f $(TARGET)
	-rm -f $(TARGET_MPI)
