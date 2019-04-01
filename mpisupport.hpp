#pragma once

//#define ENABLE_MPI

#ifdef ENABLE_MPI
#include <mpi.h>
#endif

inline bool isMPIEnabled() {
#ifndef ENABLE_MPI
	return false;
#else
	return true;
#endif
}

inline int getMPIRank() {
	int rank = -1;
#ifdef ENABLE_MPI
	::MPI_Comm_rank(MPI_COMM_WORLD, &rank);
#endif
	return rank;
}

inline int getMPINumberOfProcesses() {
	int size = 1;
#ifdef ENABLE_MPI
	::MPI_Comm_size(MPI_COMM_WORLD, &size);
#endif
	return size;
}
