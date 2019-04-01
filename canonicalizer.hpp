#pragma once
#include "util.hpp"
#include "symmetry.hpp"
#include "mask81.hpp"
#include <vector>
#include <string>
#include <algorithm>

class Canonicalizer {
	std::vector<Symmetry::CellPermutation> _columnPermutations;

public:
	class GridPermutation {
		friend class Canonicalizer;
		Symmetry::CellPermutation _cellPerm;
		char _digitPerm[9];

	public:
		GridPermutation() = default;
		std::string applyTo(const char *grid) const;

		friend std::ostream &operator<<(std::ostream &os, const GridPermutation &perm);
		friend std::istream &operator>>(std::istream &is, GridPermutation &perm);
	};

	Canonicalizer();

	Mask81 canonicalizeMask(const Mask81 &mask, Symmetry::CellPermutation *resPerm = nullptr) const;
	std::string canonicalizeSolution(const char *solution, GridPermutation *resPerm = nullptr) const;
	std::string canonicalizeProblem(const char *problem, GridPermutation *resPerm = nullptr) const;

	static int findLexMinRowPermutation(unsigned rows[9]);

private:
	static int sortBand(unsigned x[3]);
	static int sortTwoBands(unsigned x[3], unsigned y[3]);

	static void applyRowSwaps(Symmetry::CellPermutation &perm, int rowSwaps);
};
