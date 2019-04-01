#include "symmetrysearcher.hpp"
#include "symmetry.hpp"
#include "uafinder.hpp"
#include "mask81.hpp"
#include "masksymmetry.hpp"
#include <cstdlib>
#include <cmath>
#include <algorithm>
#include <unordered_set>
#include <array>
#include <bitset>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
using namespace std;

//#include "C:\Dropbox\backup\implements\Util\CPUTime.hpp"

SymmetrySearcher::SymmetrySearcher(size_t memoSize, Logger &infoLogger) :
	_memoSize(memoSize), _infoLogger(infoLogger) {
	_columnPermutations.resize(Symmetry::COLUMN_PERMUTATIONS_ORDER);
	auto generator = Symmetry::allColumnPermutations();
	copy(generator.begin(), generator.end(), _columnPermutations.begin());
}

void SymmetrySearcher::setMasks(const vector<Mask81> &masks, const MaskSymmetry::RowPermutationDiagram *rowPermutationDiagram) {
	assert(rowPermutationDiagram != nullptr);
	_hintMasks = masks;
	_rowPermutationDiagram = rowPermutationDiagram;
}

void SymmetrySearcher::setSolution(const char *solution, int uaSize) {
	rep(i, 81)
		_solution[i] = solution[i];
	_solution[81] = '\0';

	_uniquenessChecker.setSolution(_solution, _memoSize);

	UAFinder _uafinder;
//	_infoLogger->log("finding uaSets...");
//	CPUTIMEIT("find ua sets")
	_uafinder.findAll(solution, uaSize);

	_uaSets.clear();
	for(int i = 0; i <= 81; ++ i) {
		const auto &list = _uafinder.getList(i);
		_uaSets.insert(_uaSets.end(), list.begin(), list.end());
	}

	int depthWeights[10];
	depthWeights[9] = 1;
	for(int i = 8; i >= 0; -- i)
		depthWeights[i] = depthWeights[i + 1] * (i % 3 == 0 ? 3 - i / 3 : 3 - i % 3);

	vector<unsigned short> rowMasks;
	for(int i = 0; i <= min(uaSize, 6); ++ i) {
		const auto &list = _uafinder.getList(i);
		for(const Mask81 &mask : list) {
			unsigned short rowMask = 0;
			for(int pos : mask)
				rowMask |= 1 << (pos / 9);
			rowMasks.push_back(rowMask);
		}
	}

	rep(i, 9) _rowDepth[i] = i;

	int bandPerm[3] = { 0, 1, 2 };

	long long maxWeight = -1;
	do {
		long long sumWeight = 0;
		for(unsigned short rowMask : rowMasks) {
			int lastDepth = 0;
			for(int row : EachBit(rowMask))
				lastDepth = max(lastDepth, bandPerm[row / 3] * 3 + row % 3);
			sumWeight += depthWeights[lastDepth + 1];
		}
		if(maxWeight < sumWeight) {
			maxWeight = sumWeight;
			rep(i, 9) _rowDepth[i] = bandPerm[i / 3] * 3 + i % 3;
		}
	} while(next_permutation(bandPerm, bandPerm + 3));

	rep(i, 9) _depthRow[_rowDepth[i]] = i;
	
	for(auto &v : _lastDepthUASets2) v.clear();
	for(auto &v : _lastDepthUASets3) v.clear();
	for(const Mask81 &mask : _uaSets)
		addUASet(mask);

/*
	string uaSetCounts;
	rep(i, 6)
		uaSetCounts += to_string(_lastDepthUASets2[i].size()) + " ";
	rep(i, 3)
		uaSetCounts += to_string(_lastDepthUASets3[i].size()) + (i == 2 ? "" : " ");

	_infoLogger.log(
		"setSolution"
		"\n  ", _solution,
		"\n  maxWeight: ", maxWeight,
		"\n  uaSets: ", uaSetCounts);
//*/
}

void SymmetrySearcher::searchForSymmetries(pair<int, int> digitCountBounds, vector<SearchResultEntry> &result) {
	result.clear();

	_digitCountBounds = digitCountBounds;
	_digitCountEnabled = _digitCountBounds.first > 0 || _digitCountBounds.second < 81;

	rep(i, 9) _digitCount[i] = 0;
	rep(i, 3) _curMaskArray[i] = 0;

	_candidates.clear();
	SolverGrid grid; grid.init();
//	CPUTIMEIT("dfs for row permutation")
	dfsForRowPermutation(0, _rowPermutationDiagram->getRoot());

//	_infoLogger.log("number of candidates: ", _candidates.size());

//	CPUTIMEIT("check candidates")
	for(const SearchResultEntry &entry : _candidates) {
		result.push_back(entry);
	}
}

void SymmetrySearcher::addUASet(const Mask81 & mask) {
	int lastDepth = 0;
	array<unsigned, 3> maskArray = { {} };
	for(int pos : mask) {
		int d = _rowDepth[pos / 9];
		lastDepth = max(lastDepth, d);
		maskArray[d / 3] |= 1U << (d % 3 * 9 + pos % 9);
	}
	auto maskFirst = maskArray[0] | (uint64_t)maskArray[1] << 32;
	if(lastDepth < 6)
		_lastDepthUASets2[lastDepth].push_back(maskFirst);
	else
		_lastDepthUASets3[lastDepth - 6].emplace_back(maskFirst, maskArray[2]);
}

void SymmetrySearcher::dfsForRowPermutation(int depth, const MaskSymmetry::RowPermutationDiagram::DiagramNodeOrLeaf *nodeOrLeaf) {
	if(depth == 9) {
		Mask128 mask128(0);
		rep(depthBand, 3) for(int i : EachBit(_curMaskArray[depthBand])) {
			int row = _depthRow[depthBand * 3 + i / 9], col = i % 9;
			mask128.set(row * 9 + col);
		}

		if(!_uniquenessChecker.isSolutionUnique(mask128))
			return;

		Mask81 mask; mask.clear();
		rep(depthBand, 3) for(int i : EachBit(_curMaskArray[depthBand])) {
			int row = _depthRow[depthBand * 3 + i / 9], col = i % 9;
			mask.set(row * 9 + col);
		}

		auto leaf = static_cast<const MaskSymmetry::RowPermutationDiagram::DiagramLeaf*>(nodeOrLeaf);
		_candidates.push_back(SearchResultEntry{ mask, leaf->originalMaskIndex });

/*
		static Canonicalizer canon;
		Mask81 a = canon.canonicalizeMask(mask);
		assert(a == _hintMasks[leaf->originalMaskIndex]);
//*/

		return;
	}

	int depthBand = depth / 3, depthIndexOffset = depth % 3 * 9;
	int newRow = _depthRow[depth];

	auto node = static_cast<const MaskSymmetry::RowPermutationDiagram::DiagramNode*>(nodeOrLeaf);
	for(auto edgeIt = _rowPermutationDiagram->edgeListBegin(node), edgeEnd = _rowPermutationDiagram->edgeListEnd(node); edgeIt != edgeEnd; ++ edgeIt) {
		uint16_t rowMask = edgeIt->getRowMask();

		_curMaskArray[depthBand] |= (uint32_t)rowMask << depthIndexOffset;

		bool ok = true;
		if(_digitCountEnabled) {
			int lb = _digitCountBounds.first, ub = _digitCountBounds.second;
			for(int col : EachBit(rowMask))
				ok &= ++ _digitCount[_solution[newRow * 9 + col] - '1'] <= ub;
			if(8 - depth < lb) {
				int lb2 = lb - (8 - depth);
				rep(d, 9)
					ok &= _digitCount[d] >= lb2;
			}
		}

		if(ok) {
			auto curMaskFirst = _curMaskArray[0] | (uint64_t)_curMaskArray[1] << 32;
			if(depth < 6) {
				for(auto uaSet : _lastDepthUASets2[depth]) {
					if((curMaskFirst & uaSet) == 0) {
						ok = false;
						break;
					}
				}
			} else {
				auto curMaskSecond = _curMaskArray[2];
				for(auto uaSet : _lastDepthUASets3[depth - 6]) {
					if(((curMaskFirst & uaSet.first) | (curMaskSecond & uaSet.second)) == 0) {
						ok = false;
						break;
					}
				}
			}
		}

		if(ok) {
			auto child = _rowPermutationDiagram->getEdgeHead(edgeIt);
			dfsForRowPermutation(depth + 1, child);
		}

		if(_digitCountEnabled) {
			for(int col : EachBit(rowMask))
				-- _digitCount[_solution[newRow * 9 + col] - '1'];
		}

		_curMaskArray[depthBand] &= ~(((uint32_t(1) << 9) - 1) << depthIndexOffset);
	}
}


