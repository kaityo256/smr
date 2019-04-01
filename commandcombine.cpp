#include "commandcombine.hpp"
#include "mpisupport.hpp"
#include "mask81.hpp"
#include "canonicalizer.hpp"
#include "solver.hpp"
#include "combiner.hpp"
#include "utilforcommands.hpp"
#include "mpiworkqueue.hpp"
#include "mpiresponcequeue.hpp"
#include "searchworker.hpp"
#include "loadlist.hpp"
#include <unordered_set>
#include <fstream>
#include <iostream>
#include <thread>

using namespace utilforcommands;
using namespace std;

static const int RANK_MANAGER = 0;

static void syncData(vector<Mask81> &hintMasks, WorkerConfig &config) {
#ifndef ENABLE_MPI
	VALUE_IS_UNUSED(hintMasks);
	VALUE_IS_UNUSED(config);
	abort();
#else
	auto bcast = [](void *buf, MPI_Datatype type, int count = 1) {
		::MPI_Bcast(buf, count, type, RANK_MANAGER, MPI_COMM_WORLD);
	};

	bool isRoot = getMPIRank() == RANK_MANAGER;
	int size = isRoot ? (int)hintMasks.size() : -1;
	bcast(&size, MPI_INT);
	static_assert(sizeof(Mask81) == sizeof(unsigned) * 3, "sizeof(Mask81)");
	if(!isRoot) hintMasks.resize(size * 3);
	bcast(hintMasks.data(), MPI_UNSIGNED, size * 3);
	bcast(&config.rateThreshold, MPI_INT);
	bcast(&config.uaSize, MPI_INT);
	bcast(&config.memoSize, MPI_INT);
	bcast(&config.digitCountBounds.first, MPI_INT);
	bcast(&config.digitCountBounds.second, MPI_INT);
	bcast(&config.verboseness, MPI_INT);
	bcast(&config.randomSolution, MPI_INT);
#endif
}

static int combineForManagerProcess(const string &maskListFilename, const string &solutionListFilename, const string &outputFilename, WorkerConfig config, int workers) {
	string maskListCacheFilename = "masks_reduced.txt";
	string solutionListCacheFilename = "solutions_reduced.txt";
	string knownProblemListCacheFilename = "problems_reduced.txt";

	vector<Mask81> hintMasks;
	vector<string> solutions;
	unordered_set<string> knownProblemSet;

	loadHintMasks(maskListFilename, maskListCacheFilename, hintMasks);
	if(!config.randomSolution)
		loadSolutions(solutionListFilename, solutionListCacheFilename, knownProblemListCacheFilename, solutions, knownProblemSet);

	Combiner combiner(outputFilename, workers, config);
	vector<string> knownProblems(knownProblemSet.begin(), knownProblemSet.end());
	if(!isMPIEnabled()) {
		combiner.combineAllMultithreaded(hintMasks, solutions, knownProblems);
	} else {
		syncData(hintMasks, config);
		combiner.combineAllMPI(hintMasks, solutions, knownProblems);
	}
	return 0;
}

static int combineForWorkerProcess(int myRank) {
#ifndef ENABLE_MPI
	VALUE_IS_UNUSED(myRank);
	abort();
#else
	vector<Mask81> hintMasks;
	WorkerConfig config;
	syncData(hintMasks, config);

	MaskSymmetry::RowPermutationDiagram rowPermutationDiagram;
	Logger dummyLogger(false);
	rowPermutationDiagram.buildDiagramForMasks(hintMasks, &dummyLogger);

	Logger infoLogger(MPIResponceQueue::getInstance());
	SearchWorker worker(myRank, hintMasks, &rowPermutationDiagram, config, MPIWorkQueue::getInstance(), MPIResponceQueue::getInstance(), infoLogger);

	worker.run();
#endif
	return 0;
}

int commandCombine(const vector<string> &positional, const unordered_map<string, string> &arguments) {
	checkArgument(positional.size() == 1);

	string maskListFilename, solutionListFilename, outputFilename;
	int rateThreshold = 1000;
	int uaSize = 14;
	int memoSize = 100000;
	int workers = 0;
	int digitCountLowerBound = 0;
	int digitCountUpperBound = 81;
	int verboseness = 10;
	for(const auto &p : arguments) {
		if(p.first == "problems") {
			checkArgument(maskListFilename.empty(), "duplicated arguments: --masks and --problems");
			checkArgument(solutionListFilename.empty(), "duplicated arguments: --solutions and --problems");
			maskListFilename = solutionListFilename = p.second;
		} else if(p.first == "masks") {
			maskListFilename = p.second;
		} else if(p.first == "solutions") {
			solutionListFilename = p.second;
		} else if(p.first == "threshold") {
			rateThreshold = parseInt(p.second, 0, 0x7ffffffe);
		} else if(p.first == "uasize") {
			uaSize = parseInt(p.second, 0, 81);
		} else if(p.first == "memosize") {
			memoSize = parseInt(p.second, 0, 10000000);
		} else if(p.first == "output") {
			outputFilename = p.second;
		} else if(p.first == "workers") {
			workers = parseInt(p.second, 0, 100000);
		} else if(p.first == "dclb") {
			digitCountLowerBound = parseInt(p.second, 0, 81);
		} else if(p.first == "dcub") {
			digitCountUpperBound = parseInt(p.second, 0, 81);
		} else if(p.first == "verboseness") {
			verboseness = parseInt(p.second, 0, 99);
		} else {
			checkArgument(false);
		}
	}
	if(workers != 0 && isMPIEnabled())
		workers = -1;
	if(workers == 0) {
		if(!isMPIEnabled()) {
			workers = (int)thread::hardware_concurrency();
		} else {
			workers = getMPINumberOfProcesses() - 1;
		}
	}
	if(workers <= 0) {
		cerr << "use 'mpirun -np <workers+1> ...' where workers > 0)" << endl;
		checkArgument(false);
	}

	checkArgument(!maskListFilename.empty(), "use --masks=filename or --problems=filename");
	checkArgument(!solutionListFilename.empty(), "use --solutions=filename or --problems=filename");
	checkArgument(!outputFilename.empty(), "use --output=filename");

	if(!isMPIEnabled() || getMPIRank() == 0) {
		WorkerConfig config;
		config.rateThreshold = rateThreshold;
		config.uaSize = uaSize;
		config.memoSize = memoSize;
		config.digitCountBounds = { digitCountLowerBound, digitCountUpperBound };
		config.verboseness = verboseness;
		config.randomSolution = solutionListFilename == "RANDOM";
		return combineForManagerProcess(maskListFilename, solutionListFilename, outputFilename, config, workers);
	} else {
		return combineForWorkerProcess(getMPIRank());
	}
}
