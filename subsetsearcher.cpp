#include "subsetsearcher.hpp"
#include "rater.hpp"
#include <iostream>
#include <algorithm>
#include <numeric>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdlib>
#include <cmath>

void SubsetSearcher::setSolution(const char *solution, const std::vector<Mask81> &uaSets) {
	_uniquenessChecker.setSolution(solution);
	rep(i, 81)
		_solution[i] = solution[i];
	_solution[81] = 0;
	_uaSets = uaSets;
}

bool SubsetSearcher::setSuperSet(const Mask81 &superSet) {
	assert(!_uaSets.empty());
	_maskPoses.clear();
	std::fill(_posIndices.begin(), _posIndices.end(), -1);
	_superSetSize = 0;
	for(int pos : superSet) {
		_posIndices[pos] = _superSetSize ++;
		_maskPoses.push_back(pos);
	}

	if(_superSetSize >= 64) {
		std::cerr << "unsupported: superSet size >= 64" << std::endl;
		std::abort();
	}

	if(!reduceUASets(superSet))
		return false;

	return true;
}

void SubsetSearcher::searchForSubsets(std::pair<int, int> numHintsBounds, std::vector<std::string> &validProblems) {
	_numHintsBounds = numHintsBounds;
	_validProblems = &validProblems;

	_numHints = 0;
	_prefixMask = 0;
	_unvisitedMask = (uint64_t(1) << _superSetSize) - 1;
	_remainingMask = (uint64_t(1) << _superSetSize) - 1;
	_curMask128 = Mask128(0);
	for(int pos : _maskPoses)
		_curMask128.set(pos);
	_depthCount.assign(_superSetSize + 1, 0);

	std::vector<uint64_t> uaSetBuffer(_reducedUASets.size() * (_superSetSize + 1));
	std::copy(_reducedUASets.begin(), _reducedUASets.end(), uaSetBuffer.begin());

	if(numHintsBounds.first <= _superSetSize && 0 <= numHintsBounds.second)
		searchRec(0, uaSetBuffer.data(), uaSetBuffer.data() + _reducedUASets.size());

	std::cerr << validProblems.size() << " problems found" << std::endl;
/*
	std::cerr << "depthCount: ";
	for(int d = 0; d <= _superSetSize; ++ d) std::cerr << _depthCount[d] << ", ";
	std::cerr << std::endl;
//*/
}

bool SubsetSearcher::reduceUASets(const Mask81 &superSet) {
	_reducedUASets.resize(_uaSets.size());

	rep(i, _uaSets.size()) {
		uint64_t reduced = 0;
		for(int pos : superSet.getIntersection(_uaSets[i])) {
			int index = _posIndices[pos];
			reduced |= uint64_t(1) << index;
		}
		if(reduced == 0)
			return false;
		_reducedUASets[i] = reduced;
	}
	std::sort(_reducedUASets.begin(), _reducedUASets.end(), [](uint64_t x, uint64_t y) -> bool {
		int nx = countOneBits64(x), ny = countOneBits64(y);
		if(nx != ny)
			return nx < ny;
		return x < y;
	});
	_reducedUASets.erase(std::unique(_reducedUASets.begin(), _reducedUASets.end()), _reducedUASets.end());
	int num = (int)_reducedUASets.size();
	for(int i = 0; i != num; ++ i) {
		uint64_t uaSet = _reducedUASets[i];
		rep(j, i) {
			if((_reducedUASets[j] & uaSet) == _reducedUASets[j]) {
				_reducedUASets.erase(_reducedUASets.begin() + i);
				-- num, -- i;
				break;
			}
		}
	}

//	std::cerr << "uaSets.size() = " << _uaSets.size() << ", reducedUASets.size() = " << _reducedUASets.size() << std::endl;
	return true;
}

int SubsetSearcher::searchRec(int depth, uint64_t *curUASetsBegin, uint64_t *curUASetsEnd) {
	++ _depthCount[depth];

	if(depth == _superSetSize - 2 || depth == _superSetSize) {
		if(!_uniquenessChecker.isSolutionUnique(_curMask128))
			return 99;
		
		if(depth == _superSetSize) {
			char problem[82] = {};
			std::fill(problem, problem + 81, '0');
			for(int index : EachBit64(_remainingMask)) {
				int pos = _maskPoses[index];
				problem[pos] = _solution[pos];
			}
			_validProblems->push_back(problem);
			return _numHints;
		}
	}

	int remNum = std::min(_numHintsBounds.second - _numHints, _superSetSize - depth);
	{
		uint64_t unionSet = 0;
		int independentSetSize = 0;
		for(auto it = curUASetsBegin; it != curUASetsEnd; ++ it) {
			if((unionSet & *it) == 0) {
				if((++ independentSetSize) > remNum)
					break;
				unionSet |= *it;
			}
		}
		if(independentSetSize > remNum) {
			return _numHints + independentSetSize;
		}
	}

	short indexDegrees[64];
	for(int i : EachBit64(_unvisitedMask))
		indexDegrees[i] = 0;
	for(auto it = curUASetsBegin; it != curUASetsEnd; ++ it) {
		for(int i : EachBit64(*it))
			++ indexDegrees[i];
	}

	int index = -1, indexDegree = -1;
	for(int i : EachBit64(_unvisitedMask)) {
		if(indexDegree < indexDegrees[i])
			index = i, indexDegree = indexDegrees[i];
	}
	uint64_t indexBit = uint64_t(1) << index;

	_unvisitedMask &= ~indexBit;
	uint64_t *nextUASetsBegin, *nextUASetsEnd;

	int minNum = 99;
	if(_numHints + 1 <= _numHintsBounds.second) {
		nextUASetsBegin = nextUASetsEnd = curUASetsEnd;
		if(indexDegree == 0) {
			nextUASetsEnd = std::copy(curUASetsBegin, curUASetsEnd, nextUASetsBegin);
		} else {
			for(auto it = curUASetsBegin; it != curUASetsEnd; ++ it) {
				if((*it & indexBit) == 0)
					*(nextUASetsEnd ++) = *it;
			}
		}
		
		_prefixMask |= indexBit;
		++ _numHints;
		minNum = searchRec(depth + 1, nextUASetsBegin, nextUASetsEnd);
		-- _numHints;
		_prefixMask &= ~indexBit;
	} else {
		minNum = _numHintsBounds.second + 1;
	}

	int originalLB = _numHintsBounds.first;
	if(originalLB < minNum - 1)
		_numHintsBounds.first = minNum - 1;

	if(_numHints + (_superSetSize - depth - 1) >= _numHintsBounds.first) {
		bool noEmptyUASet = true;

		if(indexDegree == 0) {
			nextUASetsBegin = curUASetsBegin;
			nextUASetsEnd = curUASetsEnd;
		} else {
			nextUASetsBegin = nextUASetsEnd = curUASetsBegin;
			for(auto it = curUASetsBegin; it != curUASetsEnd; ++ it) {
				uint64_t newUASet = *it &= ~indexBit;
				if(newUASet == 0) {
					noEmptyUASet = false;
					break;
				}
				bool noSubset = true;
				for(auto jt = nextUASetsBegin; jt != nextUASetsEnd; ++ jt) {
					if((*jt & newUASet) == *jt) {
						noSubset = false;
						break;
					}
				}
				if(noSubset)
					*(nextUASetsEnd ++) = newUASet;
			}

			if(noEmptyUASet) {
				std::sort(nextUASetsBegin, nextUASetsEnd, [](uint64_t x, uint64_t y) {
					int nx = countOneBits64(x), ny = countOneBits64(y);
					if(nx != ny)
						return nx < ny;
					return x < y;
				});
			}
		}

		if(noEmptyUASet) {
			_remainingMask &= ~indexBit;
			_curMask128.unset(_maskPoses[index]);
			int num = searchRec(depth + 1, nextUASetsBegin, nextUASetsEnd);
			if(minNum > num)
				minNum = num;
			_curMask128.set(_maskPoses[index]);
			_remainingMask |= indexBit;
		}
	}

	_unvisitedMask |= indexBit;

	_numHintsBounds.first = originalLB;

	return minNum;
}
