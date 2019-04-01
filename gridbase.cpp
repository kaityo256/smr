#include "gridbase.hpp"
#include <random>
#include <cstdlib>
#include <cassert>
using namespace std;

mutex GridBase::_initDataMutex;

GridBase::Hash GridBase::_hashCoeffs[81 + 27][9];
GridBase::Hash GridBase::_initHashCells, GridBase::_initHashCellsAndUnits;
char GridBase::_claimPattern[512];

int GridBase::unitCell(int unit, int index) {
	if(unit < 9) {	//row
		return unit * 9 + index;
	} else if(unit < 18) {	//col
		return index * 9 + (unit - 9);
	} else {	//block
		int t = unit - 18;
		return ((t / 3) * 3 + index / 3) * 9 + ((t % 3) * 3 + index % 3);
	}
}

int GridBase::cellUnit(int cell, int type) {
	if(type == 0)	//row
		return cell / 9;
	else if(type == 1)	//col
		return 9 + cell % 9;
	else	//block
		return 18 + (cell / 9 / 3) * 3 + (cell % 9 / 3);
}

int GridBase::cellUnitIndex(int cell, int type) {
	if(type == 0)	//row
		return cell % 9;
	else if(type == 1)	//col
		return cell / 9;
	else	//block
		return (cell / 9 % 3) * 3 + (cell % 9 % 3);
}

void GridBase::initData() {
	lock_guard<mutex> lock(_initDataMutex);

	if(_hashCoeffs[0][0] == 0) {
		std::default_random_engine re;
		_initHashCells = _initHashCellsAndUnits = 0;
		rep(i, 81 + 27) rep(d, 9) {
			Hash h = re();
			h <<= 32;
			h |= re();
			_hashCoeffs[i][d] = h;
			if(i < 81)
				_initHashCells ^= h;
			_initHashCellsAndUnits ^= h;
		}
	}

	if(_claimPattern[3] == 0) {
		for(int j = 0; j < 3; ++j) {
			int a = 1 << (j * 3), b = a << 1, c = b << 1;
			_claimPattern[a | b] |= j + 1;
			_claimPattern[a | c] |= j + 1;
			_claimPattern[b | c] |= j + 1;
			_claimPattern[a | b | c] |= j + 1;
		}
		for(int j = 0; j < 3; ++j) {
			int a = 1 << j, b = a << 3, c = b << 3;
			_claimPattern[a | b] |= (j + 1) << 2;
			_claimPattern[a | c] |= (j + 1) << 2;
			_claimPattern[b | c] |= (j + 1) << 2;
			_claimPattern[a | b | c] |= (j + 1) << 2;
		}
	}
}


MaybeHint parseHint(char c) {
	if('1' <= c && c <= '9')
		return MaybeHint(c - '1');
	else if(c == '0' || c == '.')
		return MaybeHint();
	else
		abort();
}

void GridBase::makeSolutionMask(const char *solution, Mask *solutionMask) {
	rep(cell, 81) {
		if(auto h = parseHint(solution[cell])) {
			solutionMask[cell] = 1 << h.get();
		} else {
			solutionMask[cell] = FullMask();
		}
	}
}

