#include "symmetry.hpp"
#include <utility>
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <cstdlib>
using namespace std;

inline Symmetry::CellPermutation Symmetry::CellPermutation::identity() {
	CellPermutation perm;
	rep(i, 81)
		perm._data[i] = i;
	return perm;
}

Symmetry::CellPermutation Symmetry::CellPermutation::operator*(const CellPermutation & that) const {
	CellPermutation res;
	rep(i, 81)
		res._data[i] = that._data[_data[i]];
	return res;
}

Symmetry::CellPermutation Symmetry::CellPermutation::inverse() const {
	CellPermutation res;
	rep(i, 81)
		res._data[_data[i]] = i;
	return res;
}


ostream &operator<<(ostream & os, const Symmetry::CellPermutation &perm) {
	rep(i, 81) {
		if(i != 0) os << ' ';
		os << (int)perm.get(i);
	}
	return os;
}

istream &operator>>(istream &is, Symmetry::CellPermutation &perm) {
	bool seen[81] = {};
	rep(i, 81) {
		int x = -1;
		is >> x;
		perm._data[i] = x;
		if(!(0 <= x && x < 81 && !seen[x])) abort();
		seen[x] = true;
	}
	return is;
}


inline Symmetry::Permutation3::Permutation3(const std::initializer_list<std::pair<char, char>>& list) {
	_numSwaps = 0;
	for(auto p : list)
		_swaps[_numSwaps ++] = p;
}

const Symmetry::Permutation3 Symmetry::allPerm3s[6] = {
	{},
	{ { 1,2 } },
	{ { 0,1 } },
	{ { 0,1 },{ 1,2 } },
	{ { 0,2 },{ 1,2 } },
	{ { 0,2 } }
};

const Symmetry::Permutation3 Symmetry::perm3Diffs[6] = {
	{ { 1,2 } },
	{ { 0,1 },{ 0,2 } },
	{ { 1,2 } },
	{ { 0,1 },{ 1,2 } },
	{ { 1,2 } },
	{ { 0,2 } }
};

void Symmetry::applyRowSwap(int row1, int row2, CellPermutation & perm) {
	int a = row1 * 9, b = row2 * 9;
	rep(j, 9)
		swap(perm._data[a + j], perm._data[b + j]);
}

void Symmetry::applyColumnSwap(int col1, int col2, CellPermutation & perm) {
	rep(j, 9)
		swap(perm._data[j * 9 + col1], perm._data[j * 9 + col2]);
}

void Symmetry::applyBandSwap(int band1, int band2, CellPermutation & perm) {
	int a = band1 * 27, b = band2 * 27;
	rep(j, 27)
		swap(perm._data[a + j], perm._data[b + j]);
}

void Symmetry::applyStackSwap(int stack1, int stack2, CellPermutation & perm) {
	int a = stack1 * 3, b = stack2 * 3;
	rep(j, 9) rep(k, 3)
		swap(perm._data[j * 9 + a + k], perm._data[j * 9 + b + k]);
}

void Symmetry::applyRowPermutation(int band, const Permutation3 & perm3, CellPermutation & perm) {
	for(auto p : perm3)
		applyRowSwap(band * 3 + p.first, band * 3 + p.second, perm);
}

void Symmetry::applyColumnPermutation(int stack, const Permutation3 & perm3, CellPermutation & perm) {
	for(auto p : perm3)
		applyColumnSwap(stack * 3 + p.first, stack * 3 + p.second, perm);
}

void Symmetry::applyBandPermutation(const Permutation3 & perm3, CellPermutation & perm) {
	for(auto p : perm3)
		applyBandSwap(p.first, p.second, perm);
}

void Symmetry::applyStackPermutation(const Permutation3 & perm3, CellPermutation & perm) {
	for(auto p : perm3)
		applyStackSwap(p.first, p.second, perm);
}

void Symmetry::applyTranspose(bool transpose, CellPermutation & perm) {
	if(transpose) {
		rep(i, 9) rep(j, i)
			swap(perm._data[i * 9 + j], perm._data[j * 9 + i]);
	}
}

Symmetry::CellPermutationIterator Symmetry::CellPermutationRange::begin() const {
	CellPermutationIterator res;
	rep(i, 10)
		res._digits[i] = 0;
	res._currentPerm = res._currentInvPerm = CellPermutation::identity();
	res._loIndex = _loIndex;
	return res;
}

Symmetry::CellPermutationIterator Symmetry::CellPermutationRange::end() const {
	CellPermutationIterator res;
	rep(i, 9)
		res._digits[i] = 0;
	res._digits[9] = 1;
	res._currentPerm = res._currentInvPerm = CellPermutation::identity();
	res._loIndex = _loIndex;
	return res;
}

int Symmetry::CellPermutationIterator::increment() {
	for(int i = _loIndex; i < 8; ++ i) {
		if(++ _digits[i] < 6)
			return i;
		_digits[i] = 0;
	}
	if(_loIndex <= 8) {
		if(++ _digits[8] < 2)
			return 8;
		_digits[8] = 0;
	}
	++ _digits[9];
	return 9;
}

void Symmetry::CellPermutationIterator::recalculatePermutation() {
	_currentInvPerm = CellPermutation::identity();

	applyTranspose(_digits[8] != 0, _currentInvPerm);

	applyStackPermutation(allPerm3s[_digits[7]], _currentInvPerm);
	applyColumnPermutation(0, allPerm3s[_digits[4]], _currentInvPerm);
	applyColumnPermutation(1, allPerm3s[_digits[5]], _currentInvPerm);
	applyColumnPermutation(2, allPerm3s[_digits[6]], _currentInvPerm);

	if(_loIndex < 4) {
		applyBandPermutation(allPerm3s[_digits[3]], _currentInvPerm);
		applyRowPermutation(0, allPerm3s[_digits[0]], _currentInvPerm);
		applyRowPermutation(1, allPerm3s[_digits[1]], _currentInvPerm);
		applyRowPermutation(2, allPerm3s[_digits[2]], _currentInvPerm);
	}

	_currentPerm = _currentInvPerm.inverse();
}

const Symmetry::CellPermutation &Symmetry::CellPermutationIterator::operator*() const {
	return _currentPerm;
}

Symmetry::CellPermutationIterator &Symmetry::CellPermutationIterator::operator++() {
	int i = increment();
	if(i <= 2) {
		rep(j, i)
			applyRowPermutation(j, perm3Diffs[5], _currentInvPerm);
		applyRowPermutation(i, perm3Diffs[_digits[i] - 1], _currentInvPerm);
		_currentPerm = _currentInvPerm.inverse();
	} else {
		recalculatePermutation();
	}
	return *this;
}

bool Symmetry::CellPermutationIterator::operator!=(const CellPermutationIterator &that) const {
	rep(i, 10)
		if(_digits[i] != that._digits[i])
			return true;
	return false;
}
