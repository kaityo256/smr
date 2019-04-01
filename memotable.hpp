#pragma once
#include <memory>
#include <cstring>

template<typename Val>
class MemoTable {
public:
	typedef unsigned long long Key;

	struct KeyVal {
		Key key;
		Val val;
	};

	MemoTable() : _size(0) {}

	void init(size_t size) {
		_size = size;
		_table.reset(new KeyVal[size]);
		clear();
	}

	void clear() {
		std::memset(_table.get(), -1, _size * sizeof(KeyVal));
		_elements = 0;
	}

	size_t size() const { return _size; }
	size_t numElements() const { return _elements; }

	Val *find(Key key) {
		if(_size == 0)
			return nullptr;
		KeyVal &p = _table[key % _size];
		if(p.key == key)
			return &p.val;
		else
			return nullptr;
	}

	void insert(Key key, const Val &val) {
		if(_size == 0)
			return;
		KeyVal &kv = _table[key % _size];
		if(kv.key == Key(-1)) ++ _elements;
		kv = KeyVal{ key, val };
	}

private:
	size_t _size;
	size_t _elements;
	std::unique_ptr<KeyVal[]> _table;
};
