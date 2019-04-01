#include "mask81.hpp"
using namespace std;

void Mask81::clear() {
	_data[0] = _data[1] = _data[2] = 0;
}

void Mask81::unset(int i) {
	_data[i / 32] &= ~(1U << (i % 32));
}

void Mask81::set(int i) {
	_data[i / 32] |= 1U << (i % 32);
}

bool Mask81::get(int i) const {
	return _data[i / 32] >> (i % 32) & 1;
}

bool Mask81::operator==(const Mask81 &that) const {
	for(int k = 0; k < 3; ++ k) {
		if(_data[k] != that._data[k])
			return false;
	}
	return true;
}

bool Mask81::isSubsetOf(const Mask81 & that) const {
	for(int k = 0; k < 3; ++ k) {
		if((_data[k] & that._data[k]) != _data[k])
			return false;
	}
	return true;
}

bool Mask81::isIntersectTo(const Mask81 & that) const {
	for(int k = 0; k < 3; ++ k) {
		if((_data[k] & that._data[k]) != 0)
			return true;
	}
	return false;
}

Mask81 Mask81::getIntersection(const Mask81 & that) const {
	Mask81 res;
	for(int k = 0; k < 3; ++ k)
		res._data[k] = _data[k] & that._data[k];
	return res;
}

Mask81 Mask81::getUnion(const Mask81 & that) const {
	Mask81 res;
	for(int k = 0; k < 3; ++ k)
		res._data[k] = _data[k] | that._data[k];
	return res;
}

int Mask81::count() const {
	int res = 0;
	for(int k = 0; k < 3; ++ k)
		res += countOneBits(_data[k]);
	return res;
}

Mask81 Mask81::fromBitString(const char *str) {
	Mask81 res; res.clear();
	rep(i, 81) if(str[i] != '0')
		res.set(i);
	return res;
}

string Mask81::toBitString() const {
	string res(81, '0');
	for(int pos : *this)
		res[pos] = '1';
	return res;
}

Mask81::Mask81Iterator::Mask81Iterator(const Mask81 & mask) {
	rep(k, 3) _data[k] = mask._data[k];
	_pos = 0;
	while(_pos < 3 && _data[_pos] == 0) ++ _pos;
}

int Mask81::Mask81Iterator::operator*() const {
	return _pos * 32 + findFirstBitPos(_data[_pos]);
}

Mask81::Mask81Iterator &Mask81::Mask81Iterator::operator++() {
	if((_data[_pos] &= _data[_pos] - 1) == 0) {
		++ _pos;
		while(_pos < 3 && _data[_pos] == 0) ++ _pos;
	}
	return *this;
}

bool Mask81::Mask81Iterator::operator!=(const Mask81Iterator &that) const {
	return _pos != that._pos;
}

ostream &operator<<(ostream &os, const Mask81 &mask) {
	os << mask.count();
	for(int pos : mask)
		os << " " << pos;
	return os;
}

istream &operator>>(istream &is, Mask81 &mask) {
	int cnt = -1;
	is >> cnt;
	mask.clear();
	rep(i, cnt) {
		int pos;
		is >> pos;
		mask.set(pos);
	}
	return is;
}

//random numbers
const uint64_t Mask81::_hashCoeffs[81] = {
	0x20fc92afccf89959ULL, 0x3f2fb884d4e1ad5aULL, 0x9fead9f8ae45a6ebULL, 0x8ee18558be767cc5ULL, 0x0f3e575427765c8aULL, 0x56a713bf880e4951ULL, 0xb6c20a34430db0bcULL, 0x20c9c0fca9122b56ULL, 0xca480c884dd63551ULL,
	0xf9a72fa35dd9d4b6ULL, 0xfb53d9b9f28e8c31ULL, 0x1f64042f929cf781ULL, 0x76c01988d17a9df9ULL, 0x8eade822102aabc4ULL, 0xc83110b771ebe375ULL, 0x1fc3dd903c67f557ULL, 0xe64b95ac4c4764acULL, 0x166d4ef4cf2b080cULL,
	0x85bc4c84bd42e1baULL, 0x2f1e55bf87b86180ULL, 0x342c6f4a5e747006ULL, 0x75ce969492785a94ULL, 0xc285b0acdf25e156ULL, 0x922b00517b8737edULL, 0x66164c17cc99c1f5ULL, 0xa3dae50ef255df71ULL, 0x51f78b8d55d92095ULL,
	0x2f571fb45370180bULL, 0x583c253403492313ULL, 0xf5a4ec6e23be7d6cULL, 0xd1cedd5a9264c9b1ULL, 0xbc5963798acfa109ULL, 0x83c3be75234d2e76ULL, 0x40b1b0724ec9b5ffULL, 0xebeba3b39586c815ULL, 0xeb2971f1a872d3f5ULL,
	0x7cc7b9ab26ec90ceULL, 0x63b00bf1d88f6596ULL, 0xf178ff8a81a75aaaULL, 0x759dc0985e3e0344ULL, 0xdc8a4f6aa9412156ULL, 0x8c65606ea72c93f7ULL, 0x97637633fa58ed9bULL, 0x698f8c94a39023c8ULL, 0xac47aab19912b10aULL,
	0x37eb48cefedcf86cULL, 0x3d620d9db12dd0c6ULL, 0xc1a4d77fbd57749eULL, 0xcf382cf6165bee94ULL, 0x8f3fc9741a11a047ULL, 0x91118741a38ddda0ULL, 0x6be46391fa1b189dULL, 0x92d791d154c882cfULL, 0xa0d5c264e2386fb5ULL,
	0xe22145c17df936bdULL, 0xd519bd4d582f0669ULL, 0xe0dd6bcd83fcb940ULL, 0x341144fdb9223640ULL, 0xc9b74c7f3311db8fULL, 0x7df16b2caae95f0bULL, 0x714e862054c40ef8ULL, 0x04dee62b287d4b51ULL, 0xed741b95de527c15ULL,
	0xaa3ab817325ee04dULL, 0x550f661a2bc99b80ULL, 0xee6f770769f4c5e3ULL, 0xb0f1e97f7b7448f9ULL, 0xd4eb017a54335d68ULL, 0xe3e2bd7cb346a4d3ULL, 0x576844f9b1b4e79eULL, 0xe1287d69c0aa43fbULL, 0xca1da6110ed1160eULL,
	0xb813ac232f62f9e0ULL, 0x36a03754509ee29eULL, 0xe04b856dd8492743ULL, 0xb5d2fe3463f1b717ULL, 0x63eaecab50037049ULL, 0xbe57e7419f23f344ULL, 0x7c86561140e2fb62ULL, 0xd5abdd2b24c45f11ULL, 0xf4c2e649e63f69abULL
};
