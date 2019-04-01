#pragma once
#include "util.hpp"
#include <cstdint>
#include <functional>
#include <istream>
#include <iterator>
#include <ostream>
#include <string>

class Mask81 {
  unsigned _data[3];

public:
  Mask81() {
  }

  void clear();
  unsigned getData(int i) const {
    return _data[i];
  }

  void unset(int i);
  void set(int i);

  bool get(int i) const;

  bool operator==(const Mask81 &that) const;
  bool operator!=(const Mask81 &that) const {
    return !(*this == that);
  }

  bool isSubsetOf(const Mask81 &that) const;
  bool isIntersectTo(const Mask81 &that) const;

  Mask81 getIntersection(const Mask81 &that) const;
  Mask81 getUnion(const Mask81 &that) const;

  int count() const;

  friend class Mask81Iterator;
  class Mask81Iterator : std::iterator<std::input_iterator_tag, int> {
    unsigned _data[3];
    int _pos;

  public:
    Mask81Iterator()
        : _pos(3) {
    }
    explicit Mask81Iterator(const Mask81 &mask);

    int operator*() const;

    Mask81Iterator &operator++();

    bool operator!=(const Mask81Iterator &that) const;
  };

  Mask81Iterator begin() const {
    return Mask81Iterator(*this);
  }
  Mask81Iterator end() const {
    return Mask81Iterator();
  }

  friend std::ostream &operator<<(std::ostream &os, const Mask81 &mask);
  friend std::istream &operator>>(std::istream &is, Mask81 &mask);

  static Mask81 fromBitString(const char *str);
  std::string toBitString() const;

  friend struct std::hash<Mask81>;

private:
  static const uint64_t _hashCoeffs[81];
};

namespace std {
template <>
struct hash<Mask81> {
  size_t operator()(const Mask81 &a) const {
    size_t r = 1;
    for (int i : a)
      r ^= static_cast<size_t>(Mask81::_hashCoeffs[i]);
    return r;
  }
};
}; // namespace std
