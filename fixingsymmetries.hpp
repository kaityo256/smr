#pragma once
#include "symmetry.hpp"
#include "mask81.hpp"
#include <cstring>
#include <vector>

class FixingSymmetries {
public:
	enum { MaxHintCount = 32 };

	struct HintPosPermutation {
		uint8_t perm[MaxHintCount];

		HintPosPermutation() {
			std::memset(perm, -1, sizeof perm);
		}

		int get(int i) const { return perm[i]; }

		bool operator==(const HintPosPermutation &that) const {
			return std::memcmp(perm, that.perm, sizeof perm) == 0;
		}
		bool operator<(const HintPosPermutation &that) const {
			return std::memcmp(perm, that.perm, sizeof perm) < 0;
		}
		friend struct std::hash<HintPosPermutation>;
	};

	static bool enumerateFixingSymmetries(const Mask81 &mask, std::vector<HintPosPermutation> &res);
};


namespace std {
template<> struct hash<FixingSymmetries::HintPosPermutation> {
	size_t operator()(const FixingSymmetries::HintPosPermutation &a) const {
		size_t r = 1;
		for(int x : a.perm)
			r = r * 123 + x;
		return r;
	}
};
};
