#include "solvergrid.hpp"
#include <iostream>
#include <vector>
#include <functional>
#include <bitset>
#include <algorithm>
#include <cassert>

using namespace std;

void SolverGrid::init() {
	std::fill(_cellMasks, _cellMasks + 81, FullMask());
	rep(unit, 27) rep(d, 9)
		_unitMasks[unit][d] = FullMask();
	_invalid = false;
	initData();
	_hash = getInitHashCells();
}

void SolverGrid::load(const char * problem) {
	init();
	rep(cell, 81) {
		if(auto h = parseHint(problem[cell]))
			applyMask(cell, 1 << h.get());
	}
#ifdef _DEBUG
	debugCheckUnitMasks();
#endif
}

void SolverGrid::assign(int cell, int d) {
	applyMask(cell, 1 << d);
}

void SolverGrid::eliminate(int cell, int d) {
	applyMask(cell, ~(1 << d));
}

inline void divmod3(int x, int &quot, int &rem) {
	quot = (x * 43) >> 7;
	rem = x - quot * 3;
}
inline void divmod9(int x, int &quot, int &rem) {
	quot = (x * 57) >> 9;
	rem = x - quot * 9;
}


void SolverGrid::applyMask(int cell, Mask mask) {
	Mask changed = _cellMasks[cell] & ~mask;
	if(changed == 0) return;

	Mask applied;
	applied = _cellMasks[cell] &= mask;

	for(int d : EachBit(changed))
		_hash ^= getCellHashCoeff(cell, d);

	int row, col;
	divmod9(cell, row, col);
	int band, stack;
	int blockrow, blockcol;
	divmod3(row, band, blockrow);
	divmod3(col, stack, blockcol);
	int blockindex = blockrow * 3 + blockcol;
	int block = band * 3 + stack;
	for(int d : EachBit(changed)) {
		_unitMasks[row][d] &= ~(1 << col);
		_unitMasks[9 + col][d] &= ~(1 << row);
		_unitMasks[18 + block][d] &= ~(1 << blockindex);
	}
	if(applied == 0) {
		_invalid = true;
	} else if(isSingleBitOrZero(applied)) {
		int d = findFirstBitPos(applied);
		_unitMasks[row][d] = 0;
		_unitMasks[9 + col][d] = 0;
		_unitMasks[18 + block][d] = 0;
		rep(x, 9) if(x != col) {
			int cell2 = row * 9 + x;
			if(_cellMasks[cell2] & applied)
				applyMask(cell2, ~applied);
		}
		rep(y, 9) if(y != row) {
			int cell2 = y * 9 + col;
			if(_cellMasks[cell2] & applied)
				applyMask(cell2, ~applied);
		}
		rep(y, 3) if(y != blockrow) {
			rep(x, 3) if(x != blockcol) {
				int cell2 = (band * 3 + y) * 9 + (stack * 3 + x);
				if(_cellMasks[cell2] & applied)
					applyMask(cell2, ~applied);
			}
		}
	}
}

bool SolverGrid::isDetermined(int cell) const {
	return isSingleBitOrZero(getCellMask(cell));
}

int SolverGrid::getSolutionDigit(int cell) const {
	Mask mask = getCellMask(cell);
	assert(mask != 0 && isSingleBitOrZero(mask));
	return findFirstBitPos(mask);
}

void SolverGrid::debugCheckValidity() const {
	if(isInvalid()) return;

	SolverGrid tmp;
	tmp.init();
	rep(cell, 81) {
		Mask mask = getCellMask(cell);
		tmp.applyMask(cell, mask);
		if(tmp.isInvalid()) {
			cerr << "invalid" << endl;
			abort();
		}
	}
}

void SolverGrid::debugCheckUnitMasks() const {
	rep(unit, 27) rep(d, 9) {
		Mask mask = 0;
		rep(index, 9) {
			int cell = unitCell(unit, index);
			if(!isDetermined(cell) && (getCellMask(cell) >> d & 1))
				mask |= 1 << index;
		}
		if(mask != _unitMasks[unit][d]) {
			cerr << "err" << endl;
			abort();
		}
	}
}

void SolverGrid::debugOut() const {
	cerr << "SolverGrid";
	if(isInvalid()) cerr << "[invalid]";
	cerr << " hash = " << _hash << endl;
	if(!isInvalid()) {
		rep(cell, 81) {
			Mask mask = getCellMask(cell);
			if(mask != 0 && isSingleBitOrZero(mask))
				cerr << findFirstBitPos(mask) + 1;
			else
				cerr << '0';
		}
		cerr << endl;
	}
	rep(row, 9) {
		rep(col, 9) {
			int cell = row * 9 + col;
			Mask mask = getCellMask(cell);
			rep(d, 9)
				cerr << (mask >> d & 1 ? char('1' + d) : '.');
			if(col != 8) {
				cerr << (col % 3 == 2 ? '|' : ' ');
			}
		}
		cerr << endl;
		if(row % 3 == 2 && row != 8) {
			rep(i, 10 * 9 - 1)
				cerr << "-";
			cerr << endl;
		}
	}
}

void SolverGrid::makeStep(StepResult &result) {
	if(isInvalid()) {
		result.type = ResultType::Invalid;
		return;
	}

#ifdef _DEBUG
	debugCheckUnitMasks();
	debugCheckValidity();
#endif

	int minSize = 10;

	int moves = 0;
	rep(unit, 27) {
		rep(d, 9) {
			Mask mask = getUnitMask(unit, d);
			if(mask == 0)
				continue;
			int cnt = countOneBits(mask);
			if(cnt == 1) {
				int index = findFirstBitPos(mask);
				int cell = SolverGrid::unitCell(unit, index);
				assign(cell, d);
				++ moves;
			} else if(cnt >= 2) {
				if(minSize > cnt) {
					minSize = 0;
					for(int index : EachBit(mask)) {
						int cell = SolverGrid::unitCell(unit, index);
						result.smallestTuple[minSize ++] = TupleElement(cell, d);
					}
				}
			}
		}
	}

	if(moves == 0 && minSize == 10) {
		result.type = ResultType::Solved;
		return;
	}

	if(moves > 0) {
		result.type = ResultType::MadeMoves;
		return;
	}

	moves += processBoxLineReduction(
		[this](int unit, int d) { return getUnitMask(unit, d); },
		[this](int cell, int d) { eliminate(cell, d); }
	);

	if(moves > 0) {
		result.type = ResultType::MadeMoves;
		return;
	}

	rep(cell, 81) {
		Mask mask = getCellMask(cell);
		if(isSingleBitOrZero(mask))
			continue;

		int cnt = countOneBits(mask);
		if(cnt >= 2) {
			if(minSize > cnt) {
				minSize = 0;
				for(int d : EachBit(mask))
					result.smallestTuple[minSize ++] = TupleElement(cell, d);
			}
		}
	}

	result.type = ResultType::TimeToGuess;
	result.tupleSize = minSize;
	return;
}
