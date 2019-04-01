#pragma once
#include "mask81.hpp"
#include "uniquenesschecker.hpp"
#include "rater.hpp"
#include "logger.hpp"
#include <vector>
#include <cstdint>

class FullSearcher {
public:
	FullSearcher(size_t memoSize, long long rateThreshold, Logger &outputLogger, Logger &infoLogger);
	void searchStronglyUnique(Mask81 mask);

private:
	size_t _memoSize;
	long long _rateThreshold;
	Logger &_outputLogger, &_infoLogger;

	UniquenessChecker _checker;
	Rater _rater;

	std::vector<int> _hintPoses;
	std::vector<int> _boxPattern;
	uint32_t _usedDigits;
	uint32_t _usedBandStack[6];
	char _curProblem[82];
	long long _checkedProblems;
	long long _noSolutionProblems;
	long long _uniqueSolutionProblems;

	void searchStronglyUniqueRec(int i);
};
