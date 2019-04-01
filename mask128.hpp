#pragma once
#include <type_traits>
#include <string>
#include <cstdint>
#ifndef __GNUC__
#include <intrin.h>
#endif

struct alignas(16) Mask128 {
	enum { HalfSize = 64 };

	uint64_t data[2];

	Mask128() {}
	explicit Mask128(uint64_t data0) : data{ data0, 0 } {}
	Mask128(uint64_t data0, uint64_t data1) : data{ data0, data1 } {}

	explicit operator bool() const {
		return (data[0] | data[1]) != 0;
	}

	bool get(unsigned pos) const {
		return data[pos / HalfSize] >> (pos % HalfSize) & 1;
	}

	void set(unsigned pos) {
		data[pos / HalfSize] |= uint64_t(1) << (pos % HalfSize);
	}

	void unset(unsigned pos) {
		data[pos / HalfSize] &= ~(uint64_t(1) << (pos % HalfSize));
	}

	Mask128 &operator&=(Mask128 that) {
		data[0] &= that.data[0];
		data[1] &= that.data[1];
		return *this;
	}
	Mask128 &operator|=(Mask128 that) {
		data[0] |= that.data[0];
		data[1] |= that.data[1];
		return *this;
	}
	Mask128 &operator^=(Mask128 that) {
		data[0] ^= that.data[0];
		data[1] ^= that.data[1];
		return *this;
	}
	Mask128 operator&(Mask128 that) const {
		return Mask128(data[0] & that.data[0], data[1] & that.data[1]);
	}
	Mask128 operator|(Mask128 that) const {
		return Mask128(data[0] | that.data[0], data[1] | that.data[1]);
	}
	Mask128 operator^(Mask128 that) const {
		return Mask128(data[0] ^ that.data[0], data[1] ^ that.data[1]);
	}
	Mask128 operator~() const {
		return Mask128(~data[0], ~data[1]);
	}

	template<int Shift>
	typename std::enable_if<Shift == 0, Mask128&>::type shiftLeft() {
		return *this;
	}
	template<int Shift>
	typename std::enable_if<(0 <= Shift && Shift < HalfSize), Mask128&>::type shiftLeft() {
		data[1] <<= Shift;
		data[1] |= data[0] >> (HalfSize - Shift);
		data[0] <<= Shift;
		return *this;
	}
	template<int Shift>
	typename std::enable_if<(HalfSize <= Shift && Shift < HalfSize * 2), Mask128&>::type shiftLeft() {
		data[1] = data[0] << (Shift - HalfSize);
		data[0] = 0;
		return *this;
	}

	Mask128 getLowestBit() const {
		if(data[0] != 0)
			return Mask128(data[0] & (~data[0] + 1), 0);
		else
			return Mask128(0, data[1] & (~data[1] + 1));
	}

	int getBitPos() const {
		if(data[0] != 0)
			return bsf(data[0]);
		else
			return HalfSize + bsf(data[1]);
	}

	int countOneBits() const {
		return popcount(data[0]) + popcount(data[1]);
	}

	std::string toBitString() const {
		std::string res(128, '0');
		for(int i = 0; i < 128; ++ i)
			res[i] = '0' + get(i);
		return res;
	}

	static int bsf(uint64_t x) {
#if defined(__GNUC__)
		return __builtin_ctzll(x);
#else
		unsigned long res;
		_BitScanForward64(&res, x);
		return res;
#endif
	}

	static int popcount(uint64_t x) {
#if defined(__GNUC__)
		return __builtin_popcountll(x);
#else
		return (int)__popcnt64(x);
#endif
	}
};
