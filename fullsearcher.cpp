#include "fullsearcher.hpp"
#include <vector>
#include <algorithm>
#include <iostream>
#include <cstdint>

FullSearcher::FullSearcher(size_t memoSize, long long rateThreshold, Logger &outputLogger, Logger &infoLogger) :
	_memoSize(memoSize), _rateThreshold(rateThreshold),
	_outputLogger(outputLogger), _infoLogger(infoLogger) {

	_checker.unsetSolution(_memoSize);
}

void FullSearcher::searchStronglyUnique(Mask81 mask) {
	_hintPoses.clear();
	for(int pos : mask)
		_hintPoses.push_back(pos);
	std::sort(_hintPoses.begin(), _hintPoses.end());

	_boxPattern.clear();
	for(auto p : _hintPoses)
		_boxPattern.push_back(p / 27 * 3 + p % 9 / 3);

	_usedDigits = 0;
	std::memset(_usedBandStack, 0, sizeof _usedBandStack);

	std::fill(_curProblem, _curProblem + 81, '0');
	_curProblem[81] = 0;

	_checkedProblems = _noSolutionProblems = _uniqueSolutionProblems = 0;

	searchStronglyUniqueRec(0);

	_infoLogger.log(
		"search for ", mask.toBitString(), " ended\n",
		"  checked: ", _checkedProblems, ", no sol: ", _noSolutionProblems, ", valid: ", _uniqueSolutionProblems);
}

void FullSearcher::searchStronglyUniqueRec(int i) {
	if(i == (int)_boxPattern.size()) {
		int status = _checker.countSolutions(_curProblem);

		++ _checkedProblems;

		if(status == 0) {
			++ _noSolutionProblems;
			return;
		} else if(status == 1) {
			++ _uniqueSolutionProblems;
			auto rate = _rater.rate(_curProblem);
			if(rate >= _rateThreshold) {
				_outputLogger.log(_curProblem, ' ', rate);
				_infoLogger.log(_curProblem, ": ", rate);
			}
		}

		return;
	}

	int pos = _hintPoses[i];
	int box = _boxPattern[i], band = box / 3, stack = box % 3;
	int mask = ((1 << 9) - 1) & ~_usedBandStack[band] & ~_usedBandStack[3 + stack];
	mask &= _usedDigits << 1 | 1;
	uint32_t prevUsedDigits = _usedDigits;

	for(int d : EachBit(mask)) {
		uint32_t dmask = 1 << d;

		_usedBandStack[band] |= dmask;
		_usedBandStack[3 + stack] |= dmask;
		_usedDigits |= dmask;

		_curProblem[pos] = '1' + d;

		searchStronglyUniqueRec(i + 1);

		_curProblem[pos] = '0';

		_usedDigits = prevUsedDigits;
		_usedBandStack[3 + stack] &= ~dmask;
		_usedBandStack[band] &= ~dmask;
	}
}
