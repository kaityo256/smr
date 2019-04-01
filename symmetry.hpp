#pragma once
#include "util.hpp"
#include <utility>
#include <initializer_list>
#include <iterator>
#include <istream>
#include <ostream>

class Symmetry {
public:
	class CellPermutation {
		friend class Symmetry;
		unsigned char _data[81];

	public:
		CellPermutation() = default;
		int get(int i) const { return _data[i]; }

		static CellPermutation identity();
		CellPermutation operator*(const CellPermutation &that) const;
		CellPermutation inverse() const;

		friend std::ostream &operator<<(std::ostream &os, const CellPermutation &perm);
		friend std::istream &operator>>(std::istream &is, CellPermutation &perm);
	};

public:
	class CellPermutationIterator;
	class Permutation3 {
		friend class Symmetry;
		friend class CellPermutationIterator;
		std::pair<unsigned char, unsigned char> _swaps[2];
		int _numSwaps;
		Permutation3() {}
		Permutation3(const std::initializer_list<std::pair<char, char> > &list);
	public:
		const std::pair<unsigned char, unsigned char> *begin() const { return _swaps; }
		const std::pair<unsigned char, unsigned char> *end() const { return _swaps + _numSwaps; }
	};

	static const Permutation3 allPerm3s[6];
	static const Permutation3 perm3Diffs[6];

	static void applyRowSwap(int row1, int row2, CellPermutation &perm);
	static void applyColumnSwap(int col1, int col2, CellPermutation &perm);
	static void applyBandSwap(int band1, int band2, CellPermutation &perm);
	static void applyStackSwap(int stack1, int stack2, CellPermutation &perm);

	static void applyRowPermutation(int band, const Permutation3 &perm3, CellPermutation &perm);
	static void applyColumnPermutation(int stack, const Permutation3 &perm3, CellPermutation &perm);
	static void applyBandPermutation(const Permutation3 &perm3, CellPermutation &perm);
	static void applyStackPermutation(const Permutation3 &perm3, CellPermutation &perm);
	static void applyTranspose(bool transpose, CellPermutation &perm);

	class CellPermutationRange;
	class CellPermutationIterator : public std::iterator<std::input_iterator_tag, CellPermutation> {
		friend class CellPermutationRange;
		unsigned char _digits[10];
		CellPermutation _currentInvPerm;
		CellPermutation _currentPerm;
		int _loIndex;

		int increment();
		void recalculatePermutation();

	public:
		const CellPermutation &operator*() const;
		CellPermutationIterator &operator++();
		bool operator!=(const CellPermutationIterator &that) const;
	};

	class CellPermutationRange {
		friend class Symmetry;
		int _loIndex;
		CellPermutationRange(int loIndex) : _loIndex(loIndex) {}
	public:
		CellPermutationIterator begin() const;
		CellPermutationIterator end() const;
	};

	static CellPermutationRange allCellPermutations() { return CellPermutationRange(0); }
	static CellPermutationRange allColumnPermutations() { return CellPermutationRange(4); }

	static const int CELL_PERMUTATIONS_ORDER = 3359232;
	static const int COLUMN_PERMUTATIONS_ORDER = 2592;
};
