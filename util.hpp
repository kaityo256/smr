#pragma once

#include <iterator>
#include <utility>
#ifndef __GNUC__
#include <intrin.h>
#endif

#define rep(i,n) for(int i = 0; i < static_cast<int>(n); ++ i)

inline int findFirstBitPos(unsigned x) {
#ifdef __GNUC__
	return __builtin_ctz(x);
#else
	unsigned long res;
	_BitScanForward(&res, x);
	return res;
#endif
}

inline int findFirstBitPos64(unsigned long long x) {
#ifdef __GNUC__
	return __builtin_ctzll(x);
#else
	unsigned long res;
	_BitScanForward64(&res, x);
	return res;
#endif
}

inline int findLastBitPos(unsigned x) {
#ifdef __GNUC__
	return 31 - __builtin_clz(x);
#else
	unsigned long res;
	_BitScanReverse(&res, x);
	return res;
#endif
}

inline int countOneBits(unsigned x) {
#ifdef __GNUC__
	return __builtin_popcount(x);
#else
	return __popcnt(x);
#endif
}

inline int countOneBits64(unsigned long long x) {
#ifdef __GNUC__
	return __builtin_popcountll(x);
#else
	return (int)__popcnt64(x);
#endif
}

inline bool isSingleBitOrZero(unsigned x) {
	return (x & (x - 1)) == 0;
}

class MaskIterator : std::iterator<std::input_iterator_tag, int> {
	unsigned _mask;
public:
	MaskIterator() : _mask(0) {}
	explicit MaskIterator(unsigned mask) : _mask(mask) {}

	int operator*() const { return findFirstBitPos(_mask); }
	MaskIterator &operator++() { _mask &= _mask - 1; return *this; }
	bool operator!=(MaskIterator that) const { return _mask != that._mask; }
};

class MaskIterator64 : std::iterator<std::input_iterator_tag, int> {
	unsigned long long _mask;
public:
	MaskIterator64() : _mask(0) {}
	explicit MaskIterator64(unsigned long long mask) : _mask(mask) {}

	int operator*() const { return findFirstBitPos64(_mask); }
	MaskIterator64 &operator++() { _mask &= _mask - 1; return *this; }
	bool operator!=(MaskIterator64 that) const { return _mask != that._mask; }
};

class EachBit {
	unsigned _mask;
public:
	explicit EachBit(unsigned mask) : _mask(mask) {}
	MaskIterator begin() const { return MaskIterator(_mask); }
	MaskIterator end() const { return MaskIterator(); }
};

class EachBit64 {
	unsigned long long _mask;
public:
	explicit EachBit64(unsigned long long mask) : _mask(mask) {}
	MaskIterator64 begin() const { return MaskIterator64(_mask); }
	MaskIterator64 end() const { return MaskIterator64(); }
};


#define VALUE_IS_UNUSED(x) (void)(x)
