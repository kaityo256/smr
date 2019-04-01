#pragma once
#include "mask81.hpp"
#include "uniquenesschecker.hpp"
#include "uafinder.hpp"
#include <vector>
#include <array>
#include <string>
#include <utility>

class SubsetSearcher {
public:
	SubsetSearcher() { }

	void setSolution(const char *solution, const std::vector<Mask81> &uaSets);
	bool setSuperSet(const Mask81 &superSet);
	void searchForSubsets(std::pair<int, int> numHintsBounds, std::vector<std::string> &validProblems);

private:
	bool reduceUASets(const Mask81 &superSet);
	int searchRec(int depth, uint64_t *curUASetsBegin, uint64_t *curUASetsEnd);

	UniquenessChecker _uniquenessChecker;
	std::vector<Mask81> _uaSets;
	char _solution[82];
	std::pair<int, int> _numHintsBounds;
	std::vector<std::string> *_validProblems;
	std::vector<int> _maskPoses;
	int _superSetSize;
	std::array<int, 81> _posIndices;

	std::vector<uint64_t> _reducedUASets;

	int _numHints;
	uint64_t _prefixMask, _remainingMask, _unvisitedMask;
	Mask128 _curMask128;
	std::vector<size_t> _depthCount;
};
