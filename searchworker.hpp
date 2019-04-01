#pragma once
#include "queueinterfaces.hpp"
#include "logger.hpp"
#include "mask81.hpp"
#include "symmetrysearcher.hpp"
#include "message.hpp"
#include "masksymmetry.hpp"
#include "rater.hpp"
#include <string>

class SearchWorker {
private:
	int _workerID;
	SymmetrySearcher _searcher;
	Rater _rater;
	Canonicalizer _canonicalizer;
	std::vector<Mask81> _hintMasks;
	const MaskSymmetry::RowPermutationDiagram *_rowPermutationDiagram;
	WorkerConfig _config;
	WorkQueueInterface &_workQueue;
	ResponceQueueInterface &_outputQueue;
	Logger &_infoLogger;

public:
	SearchWorker(
		int workerID,
		const std::vector<Mask81> &hintMasks,
		const MaskSymmetry::RowPermutationDiagram *rowPermutationDiagram,
		const WorkerConfig &config,
		WorkQueueInterface &workQueue,
		ResponceQueueInterface &outputQueue,
		Logger &infoLogger) :
		_workerID(workerID),
		_searcher(config.memoSize, infoLogger),
		_rater(config.memoSize),
		_canonicalizer(),
		_hintMasks(hintMasks),
		_rowPermutationDiagram(rowPermutationDiagram),
		_config(config),
		_workQueue(workQueue), _outputQueue(outputQueue),
		_infoLogger(infoLogger) { }

	void run();

private:
	std::vector<int> _validProblems;

	void searchForSolution(const std::string &solution);

	void sendNumberOfValidProblems();
};
