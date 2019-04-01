#include "searchworker.hpp"
#include "mpiworkqueue.hpp"
#include "mpiresponcequeue.hpp"
#include "solver.hpp"
#include <thread>
#include <algorithm>
using namespace std;

void SearchWorker::run() {
	if(_config.verboseness >= 5)
		_infoLogger.log("worker ", _workerID, " started");
	if(_workerID <= 1) {
		_infoLogger.log(
			"config:"
			, "\n  rateThreshold: ", _config.rateThreshold
			, "\n  uaSize: ", _config.uaSize
			, "\n  memoSize: ", _config.memoSize
			, "\n  digitCountBounds: [", _config.digitCountBounds.first, ", ", _config.digitCountBounds.second, "]"
			, "\n  verboseness: ", _config.verboseness
			, "\n  randomizeSolution: ", _config.randomSolution ? "true" : "false"
			);
	}
	_validProblems.assign(_hintMasks.size(), 0);
	_searcher.setMasks(_hintMasks, _rowPermutationDiagram);

	long long searches = 0;
	if(!_config.randomSolution) {
		while(1) {
			{
				ResponceMessage message;
				message.type = ResponceMessage::Type::WorkerIsIdle;
				_outputQueue.enqueue(message);
			}
			WorkMessage message = _workQueue.dequeue();
			//		_infoLogger.log("worker ", _workerID, ": message type = ", (int)message.type);
			switch(message.type) {
			case WorkMessage::Type::End:
				goto end;
			case WorkMessage::Type::SearchForSolution:
				searchForSolution(message.solution);
				++ searches;
				break;
			}
		}
	} else {
		Solver solver;
		solver.setRandomEngine(default_random_engine{ random_device{}() });
		string emptyProblem(81, '0');
		char solution[82];
		solution[81] = 0;
		while(1) {
			solver.findSolution(emptyProblem.c_str(), solution, true);
			searchForSolution(solution);
			++ searches;
		}
	}
end:
	if(searches > 0 && _config.verboseness >= 4)
		sendNumberOfValidProblems();
	ResponceMessage message;
	message.type = ResponceMessage::Type::WorkerEnded;
	_outputQueue.enqueue(message);
	if(_config.verboseness >= 3)
		_infoLogger.log("worker ", _workerID, " ended");
}

void SearchWorker::searchForSolution(const std::string &solution) {
	_searcher.setSolution(solution.c_str(), _config.uaSize);
	vector<SymmetrySearcher::SearchResultEntry> result;
	long long totalValidProblems = 0;

	_searcher.searchForSymmetries(_config.digitCountBounds, result);

	string problem;
	for(const auto &e : result) {
		++ _validProblems[e.originalMaskIndex];
		++ totalValidProblems;

		problem.assign(81, '0');
		for(int pos : e.mask)
			problem[pos] = solution[pos];
		problem = _canonicalizer.canonicalizeProblem(problem.c_str());
		long long rate = _rater.rate(problem.c_str());
		if(_config.rateThreshold <= rate) {
			ResponceMessage message;
			message.type = ResponceMessage::Type::FoundProblem;
			message.problem = problem;
			message.rate = rate;
			_outputQueue.enqueue(message);
		}
	}

	if(_config.verboseness >= 4)
		_infoLogger.log("search for solution ", solution, " ended. ", totalValidProblems, " valid problems found");
}

void SearchWorker::sendNumberOfValidProblems() {
	ResponceMessage message;
	message.type = ResponceMessage::Type::AddNumberOfValidProblems;
	message.validProblemCounts = _validProblems;
	_outputQueue.enqueue(message);
	_validProblems.assign(_hintMasks.size(), 0);
}
