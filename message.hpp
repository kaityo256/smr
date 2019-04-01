#pragma once
#include <string>
#include <vector>
#include <utility>

struct WorkerConfig {
	int rateThreshold;
	int uaSize;
	int memoSize;
	std::pair<int, int> digitCountBounds;
	int verboseness;
	int randomSolution;
};

struct WorkMessage {
	enum class Type {
		End,
		SearchForSolution,
	} type;

	std::string solution;
};

struct ResponceMessage {
	enum class Type {
		WorkerIsIdle,
		FoundProblem,
		LogInfo,
		AddNumberOfValidProblems,
		WorkerEnded
	} type;

	std::string problem;
	long long rate;
	std::string info;
	std::vector<int> validProblemCounts;
};
