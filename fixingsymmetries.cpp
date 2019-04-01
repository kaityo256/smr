#include "fixingsymmetries.hpp"
#include "masksymmetry.hpp"
#include <vector>
#include <unordered_set>
#include <iostream>
#include <algorithm>
#include <cstdlib>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>

bool FixingSymmetries::enumerateFixingSymmetries(const Mask81 &mask, std::vector<HintPosPermutation>& res) {
	std::vector<int> hintPoses;
	for(int pos : mask)
		hintPoses.push_back(pos);

	int numHints = (int)hintPoses.size();
	if(numHints > MaxHintCount) {
		std::cerr << "hint count > " << MaxHintCount << std::endl;
		std::abort();
	}
	int posIndex[81];
	std::fill(posIndex, posIndex + 81, -1);
	for(int i = 0; i < numHints; ++ i)
		posIndex[hintPoses[i]] = i;
	typedef MaskSymmetry::Rows Rows;
	typedef Rows::BandRows BandRows;
	Rows originalRows = Rows::fromMask81(mask), canonicalizedOriginal = originalRows;
	canonicalizedOriginal.canonicalize();
	BandRows originalBands[3], canonicalizedOriginalBands[3];
	rep(i, 3) {
		originalBands[i] = originalRows.getBandReversed(i);
		canonicalizedOriginalBands[i] = originalBands[i];
		canonicalizedOriginalBands[i].canonicalize();
	}
	const int perm3s[6][3] = {
		{ 0,1,2 },{ 0,2,1 },{ 1,0,2 },{ 1,2,0 },{ 2,0,1 },{ 2,1,0 }
	};
	const BandRows emptyBand = { {} };
	std::unordered_set<HintPosPermutation> permSet;
	bool dup = false;
	for(const auto &columnPerm : Symmetry::allColumnPermutations()) {
		Mask81 colPermutedMask; colPermutedMask.clear();
		for(int pos : hintPoses)
			colPermutedMask.set(columnPerm.get(pos));
		Rows rows = Rows::fromMask81(colPermutedMask), canonicalized = rows;
		canonicalized.canonicalize();
		if(canonicalizedOriginal != canonicalized) continue;
		BandRows bands[3];
		rep(i, 3)
			bands[i] = rows.getBandReversed(i);
		for(const auto &bandPerm : perm3s) {
			int rowPermPossibilities[3] = {};
			bool matched = true;
			rep(band, 3) {
				BandRows canonicalizedBand = bands[band];
				canonicalizedBand.canonicalize();
				if(canonicalizedBand != canonicalizedOriginalBands[bandPerm[band]]) {
					matched = false;
					break;
				}
				const BandRows &a = bands[band];
				const BandRows &b = originalBands[bandPerm[band]];
				bool isEmptyBand = a == emptyBand;
				rep(rowPermIndex, 6) {
					if(isEmptyBand && rowPermIndex != 0) continue;
					const auto &rowPerm = perm3s[rowPermIndex];
					bool rowMatched = true;
					rep(i, 3) {
						if(a.rows[i] != b.rows[rowPerm[i]]) {
							rowMatched = false;
							break;
						}
					}
					if(rowMatched)
						rowPermPossibilities[band] |= 1 << rowPermIndex;
				}
				assert(rowPermPossibilities[band] != 0);
			}
			if(!matched) continue;

			for(int i0 : EachBit(rowPermPossibilities[0])) {
				for(int i1 : EachBit(rowPermPossibilities[1])) {
					for(int i2 : EachBit(rowPermPossibilities[2])) {
						const int *rowPerms[3] = { perm3s[i0], perm3s[i1], perm3s[i2] };
						HintPosPermutation hintPerm;
						rep(originalIndex, numHints) {
							int originalPos = hintPoses[originalIndex];
							int pos = columnPerm.get(originalPos);
							int row = pos / 9, col = pos % 9;
							int band = row / 3, rowIndex = row % 3;
							int newBand = bandPerm[band];
							int newRow = newBand * 3 + rowPerms[band][rowIndex];
							int newPos = newRow * 9 + col;
							int newIndex = posIndex[newPos];
							assert(newIndex != -1);
							hintPerm.perm[originalIndex] = newIndex;
						}

						if(!permSet.emplace(hintPerm).second)
							dup = true;
					}
				}
			}
		}
	}
	res.assign(permSet.begin(), permSet.end());
	permSet.clear();
	std::sort(res.begin(), res.end());

	return dup;
}
