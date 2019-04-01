#pragma once

#include "util.hpp"
#include <mutex>

class GridBase {
public:
	typedef unsigned short Mask;	//9bit•ª‚Ìƒrƒbƒg—ñ
	typedef unsigned long long Hash;

protected:
	static void initData();

	static Hash getCellHashCoeff(int cell, int d) { return _hashCoeffs[cell][d]; }
	static Hash getUnitHashCoeff(int unit, int d) { return _hashCoeffs[81 + unit][d]; }
	static Hash getInitHashCells() { return _initHashCells; }
	static Hash getInitHashCellsAndUnits() { return _initHashCellsAndUnits; }

private:
	static std::mutex _initDataMutex;

	static Hash _hashCoeffs[81 + 27][9];
	static Hash _initHashCells, _initHashCellsAndUnits;

public:
	static Mask FullMask() {
		return (1 << 9) - 1;
	}
	static bool isFullMask(Mask mask) {
		return mask == FullMask();
	}

	static int unitCell(int unit, int index);
	static int cellUnit(int cell, int type);
	static int cellUnitIndex(int cell, int type);

	template<typename GetUnitMask, typename Eliminate>
	static int processBoxLineReduction(GetUnitMask getUnitMask, Eliminate eliminate);

private:
	static int checkBoxRowPattern(Mask mask) {
		return _claimPattern[mask] & 3;
	}
	static int checkBoxColPattern(Mask mask) {
		return _claimPattern[mask] >> 2;
	}
	static int checkLinePattern(Mask mask) {
		return _claimPattern[mask] & 3;
	}

	static char _claimPattern[512];

public:
	static void makeSolutionMask(const char *solution, Mask *solutionMask);
};

template<typename GetUnitMask, typename Eliminate>
int GridBase::processBoxLineReduction(GetUnitMask getUnitMask, Eliminate eliminate) {
	initData();
	int moves = 0;
	for(int boxi = 0; boxi < 9; ++ boxi) {
		int box = 18 + boxi;
		for(int d = 0; d < 9; ++ d) {
			Mask boxMask = getUnitMask(box, d);
			int rowPos = checkBoxRowPattern(boxMask) - 1;
			if(rowPos != -1) {
				int line = boxi / 3 * 3 + rowPos;
				Mask lineMask = getUnitMask(line, d);
				lineMask &= ~(7 << (boxi % 3 * 3));
				for(int index : EachBit(lineMask)) {
					int cell = unitCell(line, index);
					eliminate(cell, d);
					++moves;
				}
			}
			int colPos = checkBoxColPattern(boxMask) - 1;
			if(colPos != -1) {
				int line = 9 + boxi % 3 * 3 + colPos;
				Mask lineMask = getUnitMask(line, d);
				lineMask &= ~(7 << (boxi / 3 * 3));
				for(int index : EachBit(lineMask)) {
					int cell = unitCell(line, index);
					eliminate(cell, d);
					++moves;
				}
			}
		}
	}
	for(int type = 1; type >= 0; --type) for(int linei = 0; linei < 9; ++ linei) {
		int line = type * 9 + linei;
		for(int d = 0; d < 9; ++ d) {
			Mask lineMask = getUnitMask(line, d);
			int boxPos = checkLinePattern(lineMask) - 1;
			if(boxPos == -1) continue;
			int box;
			if(type == 0)
				box = 18 + linei / 3 * 3 + boxPos;
			else
				box = 18 + boxPos * 3 + linei / 3;
			Mask boxMask = getUnitMask(box, d);
			if(type == 0)
				boxMask &= ~(7 << (linei % 3 * 3));
			else
				boxMask &= ~(73 << (linei % 3));
			for(int index : EachBit(boxMask)) {
				int cell = unitCell(box, index);
				eliminate(cell, d);
				++moves;
			}
		}
	}
	return moves;
}


class MaybeHint {
	int _digit;

public:
	explicit MaybeHint() : _digit(-1) {}
	explicit MaybeHint(int d) : _digit(d) {}

	explicit operator bool() const { return _digit != -1; }
	int get() const { return _digit; }
};

MaybeHint parseHint(char c);
