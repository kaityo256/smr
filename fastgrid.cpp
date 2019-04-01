#include "fastgrid.hpp"
#include <random>

void FastGrid::init() {
	for(int d = 0; d < 9; ++ d)
		digitMasks[d] = mask81;
}

void FastGrid::assign(int cell, int digit) {
	digitMasks[digit] &= nonadjacencyMasks[cell];
	if(cell < 64) {
		const uint64_t mask = ~(uint64_t(1) << cell);
		for(int d = 0; d < 9; ++ d)
			digitMasks[d].data[0] &= mask;
	} else {
		const uint64_t mask = ~(uint64_t(1) << (cell - 64));
		for(int d = 0; d < 9; ++ d)
			digitMasks[d].data[1] &= mask;
	}
}

Mask128 FastGrid::getUnassignedMask() const {
	Mask128 total(0);
	for(const Mask128 &mask : digitMasks)
		total |= mask;
	return total;
}

Mask128 FastGrid::getSolvedSquares() const {
	const Mask128 *g = digitMasks;
	const Mask128 x1 = (g[0] ^ g[1] ^ g[2]);
	const Mask128 x2 = (g[3] ^ g[4] ^ g[5]);
	const Mask128 x3 = (g[6] ^ g[7] ^ g[8]);
	const Mask128 a1 = (g[0] & g[1] & g[2]);
	const Mask128 a2 = (g[3] & g[4] & g[5]);
	const Mask128 a3 = (g[6] & g[7] & g[8]);
	const Mask128 o1 = (g[0] | g[1] | g[2]);
	const Mask128 o2 = (g[3] | g[4] | g[5]);
	const Mask128 o3 = (g[6] | g[7] | g[8]);
	const Mask128 b1 = x1 ^ a1;
	const Mask128 b2 = x2 ^ a2;
	const Mask128 b3 = x3 ^ a3;
	const Mask128 c1 = b1 & ~(o2 | o3);
	const Mask128 c2 = b2 & ~(o3 | o1);
	const Mask128 c3 = b3 & ~(o1 | o2);
	return c1 | c2 | c3;
}

uint64_t FastGrid::computeHash() const {
	const uint64_t *s = reinterpret_cast<const uint64_t*>(digitMasks);
	const uint64_t *k = reinterpret_cast<const uint64_t*>(hashCoeffs);
	uint64_t sum = k[0];
	for(int i = 0; i < 18; ++ i)
		sum += s[i] * (k[i + 1] | 1);
	return sum;
}

void FastGrid::initMasks() {
	std::lock_guard<std::mutex> lock(_initMasksMutex);
	if(mask81) return;

	mask81 = Mask128(~uint64_t(), (uint64_t(1) << (81 - 64)) - 1);

	Mask128 rowMask((uint64_t(1) << 9) - 1);
	for(int i = 0; i < 9; ++ i) {
		unitMasks[i] = rowMask;
		rowMask.shiftLeft<9>();
	}

	Mask128 colMask(0);
	for(int i = 0; i < 9; ++ i)
		colMask.set(i * 9);
	for(int i = 0; i < 9; ++ i) {
		unitMasks[9 + i] = colMask;
		colMask.shiftLeft<1>();
	}

	Mask128 blockMask(0);
	for(int y = 0; y < 3; ++ y) for(int x = 0; x < 3; ++ x)
		blockMask.set(y * 9 + x);
	for(int y = 0; y < 3; ++ y) {
		for(int x = 0; x < 3; ++ x) {
			unitMasks[18 + y * 3 + x] = blockMask;
			blockMask.shiftLeft<3>();
		}
		blockMask.shiftLeft<18>();
	}

	for(int cell = 0; cell < 81; ++ cell) {
		int row = cell / 9, col = cell % 9, block = row / 3 * 3 + col / 3;
		nonadjacencyMasks[cell] = ~(unitMasks[row] | unitMasks[9 + col] | unitMasks[18 + block]) & mask81;
	}

	{
		std::mt19937_64 re;
		uint64_t t = 0;
		for(int i = 0; i < 10; ++ i) {
			uint64_t x0, x1;
			t *= 0xe7b20d1f8c7a5c1dULL;
			t ^= re();
			x0 = t;
			t *= 0xa62bb9807e196f39ULL;
			t ^= re();
			x1 = t;
			hashCoeffs[i] = Mask128(x0, x1);
		}

		for(int i = 0; i < 81; ++ i) {
			for(int j = 0; j < 9; ++ j) {
				t *= 0x942e8f31711bf65dULL;
				t ^= re();
				cellHashCoeffs[i][j] = t;
			}
		}
	}
}

Mask128 FastGrid::mask81;
Mask128 FastGrid::unitMasks[27];
Mask128 FastGrid::nonadjacencyMasks[81];
Mask128 FastGrid::hashCoeffs[10];
uint64_t FastGrid::cellHashCoeffs[81][9];
std::mutex FastGrid::_initMasksMutex;
