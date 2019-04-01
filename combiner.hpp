#pragma once
#include "searchworker.hpp"
#include "mask81.hpp"
#include "logger.hpp"
#include <vector>
#include <string>
#include <unordered_set>
#include <memory>

class Combiner {
public:
	Combiner(std::string outputFileName, int numWorkers, const WorkerConfig &config) :
		_outputFileName(outputFileName), _numWorkers(numWorkers), _config(config) { }

	void combineAllMultithreaded(const std::vector<Mask81> &hintMasks, const std::vector<std::string> &solutions, const std::vector<std::string> &initKnownProblems);
	void combineAllMPI(const std::vector<Mask81> &hintMasks, const std::vector<std::string> &solutions, const std::vector<std::string> &initKnownProblems);

private:
	void combineAllMain(const std::vector<Mask81> &hintMasks, const std::vector<std::string> &solutions, const std::vector<std::string> &initKnownProblems);

	std::string _outputFileName;
	int _numWorkers;
	WorkerConfig _config;
	std::unique_ptr<MultithreadWorkQueue> _mtWorkQueue;
	std::unique_ptr<MultithreadResponceQueue> _mtResponceQueue;
	std::unique_ptr<Logger> _infoLogger;
	WorkQueueInterface *_workQueue;
	ResponceQueueInterface *_responceQueue;
};
