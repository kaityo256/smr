#include "combiner.hpp"
#include "searchworker.hpp"
#include "message.hpp"
#include "canonicalizer.hpp"
#include "solver.hpp"
#include "mpiworkqueue.hpp"
#include "mpiresponcequeue.hpp"
#include "mpisupport.hpp"
#include <vector>
#include <string>
#include <fstream>
#include <unordered_set>
#include <thread>
#include <memory>
#include <sstream>
using namespace std;

void Combiner::combineAllMultithreaded(const vector<Mask81>& hintMasks, const vector<string>& solutions, const vector<string>& initKnownProblems) {
	_infoLogger.reset(new Logger(true));
	_mtWorkQueue.reset(new MultithreadWorkQueue(solutions.size() + 1 + _numWorkers));
	_mtResponceQueue.reset(new MultithreadResponceQueue(_numWorkers * 10));
	_workQueue = _mtWorkQueue.get();
	_responceQueue = _mtResponceQueue.get();

	MaskSymmetry::RowPermutationDiagram rowPermutationDiagram;
	_infoLogger->log("building row permutation diagram...");
	rowPermutationDiagram.buildDiagramForMasks(hintMasks, _infoLogger.get());

	if(_numWorkers == 1) {
		thread main([&]() {
			combineAllMain(hintMasks, solutions, initKnownProblems);
		});
		SearchWorker worker(0, hintMasks, &rowPermutationDiagram, _config, *_workQueue, *_responceQueue, *_infoLogger);
		worker.run();
		main.join();
		return;
	}

	vector<unique_ptr<SearchWorker>> workers(_numWorkers);
	vector<thread> workerThreads(_numWorkers);
	rep(i, _numWorkers) {
		workers[i].reset(new SearchWorker(i + 1, hintMasks, &rowPermutationDiagram, _config, *_workQueue, *_responceQueue, *_infoLogger));
		workerThreads[i] = thread([&workers, i]() {
			return workers[i]->run();
		});
	}

	combineAllMain(hintMasks, solutions, initKnownProblems);

	rep(i, _numWorkers)
		workerThreads[i].detach();
}

void Combiner::combineAllMPI(const vector<Mask81>& hintMasks, const vector<string>& solutions, const vector<string>& initKnownProblems) {
#ifndef ENABLE_MPI
	VALUE_IS_UNUSED(hintMasks);
	VALUE_IS_UNUSED(solutions);
	VALUE_IS_UNUSED(initKnownProblems);
	abort();
#else
	_infoLogger.reset(new Logger(true));
	_workQueue = &MPIWorkQueue::getInstance();
	_responceQueue = &MPIResponceQueue::getInstance();
	combineAllMain(hintMasks, solutions, initKnownProblems);
#endif
}

void Combiner::combineAllMain(const vector<Mask81> &hintMasks, const vector<string>& solutions, const vector<string>& initKnownProblems) {
	_infoLogger->log("number of workers: ", _numWorkers);

	unordered_set<string> knownProblemSet(initKnownProblems.begin(), initKnownProblems.end());

	vector<long long> numberOfValidProblems(hintMasks.size(), 0);
	long long numberOfValidProblemsAddCount = 0;

	Logger resultLogger(_outputFileName.c_str(), false);

	size_t solutionIndex = 0;
	int endedWorkers = 0;
	while(endedWorkers < _numWorkers) {
		ResponceMessage message = _responceQueue->dequeue();
		switch(message.type) {
		case ResponceMessage::Type::WorkerIsIdle:
			if(solutionIndex < solutions.size()) {
				WorkMessage message;
				message.type = WorkMessage::Type::SearchForSolution;
				message.solution = solutions[solutionIndex ++];

				_workQueue->enqueue(message);

				if(solutionIndex == solutions.size()) {
					WorkMessage message;
					message.type = WorkMessage::Type::End;
					rep(i, _numWorkers)
						_workQueue->enqueue(message);
				}
			}
			break;
		case ResponceMessage::Type::WorkerEnded:
			++ endedWorkers;
			break;
		case ResponceMessage::Type::FoundProblem:
			if(knownProblemSet.emplace(message.problem).second) {
				if(_config.verboseness >= 1)
					resultLogger.log(message.problem, " ", message.rate);
				_infoLogger->log(message.problem, ": ", Rater::squash(message.rate), "s");
			} else {
				if(_config.verboseness >= 2)
					_infoLogger->log(message.problem, " is already known");
			}
			break;
		case ResponceMessage::Type::AddNumberOfValidProblems:
			rep(i, hintMasks.size())
				numberOfValidProblems[i] += message.validProblemCounts[i];

			++ numberOfValidProblemsAddCount;
			if(numberOfValidProblemsAddCount % _numWorkers == 0 || (size_t)numberOfValidProblemsAddCount == solutions.size()) {
				stringstream ss;
				ss << "number of valid problems (" << numberOfValidProblemsAddCount << "):";
				rep(i, hintMasks.size())
					ss << '\n' << hintMasks[i].toBitString() << ": " << numberOfValidProblems[i];
				_infoLogger->log(ss.str());
			}
			break;
		case ResponceMessage::Type::LogInfo:
			_infoLogger->logString(message.info);
			break;
		}
	}
}
