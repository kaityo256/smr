#pragma once
#include "uniquenesschecker.hpp"
#include "canonicalizer.hpp"
#include "symmetry.hpp"
#include "mask81.hpp"
#include "logger.hpp"
#include "masksymmetry.hpp"
#include <vector>
#include <string>
#include <array>
#include <cstdint>
#include <utility>

class SymmetrySearcher {
	size_t _memoSize;
	UniquenessChecker _uniquenessChecker;
	std::vector<Mask81> _hintMasks;
	char _solution[82];
	std::vector<Mask81> _uaSets;
	int _rowDepth[9], _depthRow[9];
	std::vector<uint64_t> _lastDepthUASets2[6];
	std::vector<std::pair<uint64_t,uint32_t>> _lastDepthUASets3[3];
	std::vector<Symmetry::CellPermutation> _columnPermutations;
	Logger &_infoLogger;
	const MaskSymmetry::RowPermutationDiagram *_rowPermutationDiagram;

public:
	SymmetrySearcher(size_t memoSize, Logger &infoLogger);

	void setMasks(const std::vector<Mask81> &masks, const MaskSymmetry::RowPermutationDiagram *rowPermutationDiagram);
	void setSolution(const char *solution, int uaSize);

	struct SearchResultEntry {
		Mask81 mask;
		int originalMaskIndex;
	};

	void searchForSymmetries(std::pair<int, int> digitCountBounds, std::vector<SearchResultEntry> &result);

private:
	void addUASet(const Mask81 &mask);

	void dfsForRowPermutation(int depth, const MaskSymmetry::RowPermutationDiagram::DiagramNodeOrLeaf *nodeOrLeaf);

	unsigned char _digitCount[9];
	bool _digitCountEnabled;
	std::pair<int, int> _digitCountBounds;
	std::vector<SearchResultEntry> _candidates;
	uint32_t _curMaskArray[3];
	Mask81 _foundUASet;
};
