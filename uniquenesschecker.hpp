#pragma once
#include "mask128.hpp"
#include "fastgrid.hpp"
#include "memotable.hpp"
#include <cstdint>
#include <mutex>
#include <cstring>

class UniquenessChecker {
private:

	struct CheckState {
		FastGrid grid;
		uint8_t solvedDigits[81];
		uint64_t solvedDigitsHash;

		explicit CheckState() {
			grid.init();
			std::memset(solvedDigits, -1, sizeof solvedDigits);
			solvedDigitsHash = 0;
		}

		uint64_t computeHash() const { return grid.computeHash() ^ solvedDigitsHash; }
	};
public:

	UniquenessChecker();

	void setSolution(const char *solution, size_t memoSize = 1 << 16);
	void unsetSolution(size_t memoSize = 1 << 16);

	bool isSolutionUnique(Mask128 mask);

	int countSolutions(const char *problem);

private:
	int checkRec(CheckState &state);
	int resolveSolvedSquares(CheckState &state);
	int resolveHiddenSingles(CheckState &state);
	int checkStatus(CheckState &state);

	int assignDigit(int cell, int digit, CheckState &state);

	bool _hasOriginalSolution;
	uint8_t _originalSolution[81];
	MemoTable<int> _memoTable;
};
