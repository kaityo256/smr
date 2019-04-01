#include "masksymmetry.hpp"
#include <algorithm>
#include <unordered_set>
#include <unordered_map>
#include <memory>
#include <iostream>
#include <limits>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>

using namespace std;

typedef MaskSymmetry::Rows Rows;
typedef MaskSymmetry::PartialRows PartialRows;
typedef MaskSymmetry::UniquePermutationPatterns UniquePermutationPatterns;
typedef MaskSymmetry::RowPermutationDiagram::DiagramNodeOrLeaf DiagramNodeOrLeaf;
typedef MaskSymmetry::RowPermutationDiagram::DiagramNode DiagramNode;
typedef MaskSymmetry::RowPermutationDiagram::DiagramLeaf DiagramLeaf;
typedef MaskSymmetry::RowPermutationDiagram::DiagramEdge DiagramEdge;

int MaskSymmetry::UniquePermutationPatterns::calculateSymmetryOrder() const {
	int prod = 1;
	prod *= Rows::uniquePermutationOrders[bandPattern];
	rep(i, 3)
		prod *= Rows::uniquePermutationOrders[rowPatterns[i]];
	return prod;
}

inline uint32_t Rows::BandRows::toHash() const {
	return rows[0] | (uint32_t)rows[1] << 9 | (uint32_t)rows[2] << 18;
}

inline Rows::BandRows Rows::BandRows::fromHash(uint32_t h) {
	const uint16_t Mask = (1 << 9) - 1;
	return BandRows{ { (uint16_t)(h & Mask), (uint16_t)(h >> 9 & Mask), (uint16_t)(h >> 18) } };
}

int Rows::BandRows::getUniqueRowPermutationPattern() const {
	return (rows[0] == rows[1] ? 1 : 0) | (rows[1] == rows[2] ? 2 : 0);
}

void MaskSymmetry::Rows::BandRows::canonicalize() {
	if(rows[1] < rows[0]) swap(rows[0], rows[1]);
	if(rows[2] < rows[1]) swap(rows[1], rows[2]);
	if(rows[1] < rows[0]) swap(rows[0], rows[1]);
}

bool Rows::BandRows::operator==(const BandRows &that) const {
	return toHash() == that.toHash();
}

MaskSymmetry::Rows MaskSymmetry::Rows::fromMask81(const Mask81 &mask) {
	Rows res;
	for(int pos : mask)
		res._rows[pos / 9] |= 1 << (8 - pos % 9);
	return res;
}

Mask81 MaskSymmetry::Rows::toMask81() const {
	Mask81 res; res.clear();
	rep(row, 9) for(int col : EachBit(_rows[row]))
		res.set(row * 9 + (8 - col));
	return res;
}

MaskSymmetry::Rows MaskSymmetry::Rows::transpose() const {
	Rows res;
	rep(row, 9) for(int col : EachBit(_rows[row]))
		res._rows[8 - col] |= 1 << (8 - row);
	return res;
}

void MaskSymmetry::Rows::swapRows(int i, int j) {
	swap(_rows[i], _rows[j]);
}

void MaskSymmetry::Rows::swapBands(int i, int j) {
	swap(_bands[i], _bands[j]);
}

void MaskSymmetry::Rows::canonicalize() {
	_bands[0].canonicalize();
	_bands[1].canonicalize();
	_bands[2].canonicalize();
	sortTwoBands(_bands[0], _bands[1]);
	sortTwoBands(_bands[1], _bands[2]);
	sortTwoBands(_bands[0], _bands[1]);
}

bool MaskSymmetry::Rows::operator==(const Rows &that) const {
	return memcmp(_rows, that._rows, sizeof _rows) == 0;
}

int MaskSymmetry::Rows::getUniquePermutationBandPattern() const {
	return (_bands[0] == _bands[1] ? 1 : 0) | (_bands[1] == _bands[2] ? 2 : 0);
}

MaskSymmetry::UniquePermutationPatterns MaskSymmetry::Rows::getUniquePermutationPattern() const {
	UniquePermutationPatterns res;
	res.bandPattern = getUniquePermutationBandPattern();
	rep(i, 3)
		res.rowPatterns[i] = _bands[i].getUniqueRowPermutationPattern();
	return res;
}

void MaskSymmetry::Rows::sortTwoBands(BandRows &x, BandRows &y) {
	if(lexicographical_compare(y.rows, y.rows + 3, x.rows, x.rows + 3))
		swap(x, y);
}

const std::pair<int, int> MaskSymmetry::Rows::uniquePermutationSwapPatterns[4][7] = {
	{ { 1,2 },{ 0,2 },{ 1,2 },{ 0,2 },{ 1,2 },{ 0,2 },{ -1,-1 } },
	{ { 1,2 },{ 0,1 },{ 0,2 },{ -1,-1 } },
	{ { 0,1 },{ 1,2 },{ 0,2 },{ -1,-1 } },
	{ { 0,1 },{ -1,-1 } },
};

const int MaskSymmetry::Rows::uniquePermutationOrders[4] = {
	6, 3, 3, 1
};

template<typename Func>
static void enumerateRowPermutations(Func func, Rows rows) {
	rows.canonicalize();
	auto pattern = rows.getUniquePermutationPattern();
	auto bandPattern = Rows::uniquePermutationSwapPatterns[pattern.bandPattern];
	pair<int, int> const *rowPatterns[3];
	rep(i, 3) rowPatterns[i] = Rows::uniquePermutationSwapPatterns[pattern.rowPatterns[i]];
	for(auto pband = bandPattern; pband->first != -1; ++ pband) {
		for(auto prow0 = rowPatterns[0]; prow0->first != -1; ++ prow0) {
			for(auto prow1 = rowPatterns[1]; prow1->first != -1; ++ prow1) {
				for(auto prow2 = rowPatterns[2]; prow2->first != -1; ++ prow2) {
					func(rows);
					rows.swapRows(6 + prow2->first, 6 + prow2->second);
				}
				rows.swapRows(3 + prow1->first, 3 + prow1->second);
			}
			rows.swapRows(0 + prow0->first, 0 + prow0->second);
		}
		rows.swapBands(pband->first, pband->second);
		swap(rowPatterns[pband->first], rowPatterns[pband->second]);
	}
}


template<typename Func>
static void enumerateUniqueColumnPermutations(Func func, Rows rows) {
	rows.canonicalize();
	Rows cols = rows.transpose();
	cols.canonicalize();
	unordered_set<Rows> visited;
	rep(trans, 2) {
		enumerateRowPermutations([func, &visited](const Rows &r) {
			Rows t = r.transpose();
			t.canonicalize();
			if(!visited.emplace(t).second)
				return;
			func(t);
		}, cols);
		if(rows == cols)
			break;
		swap(cols, rows);
	}
}

template<typename Func>
static void enumerateUniquePermutations(Func func, const Rows &rows) {
	enumerateUniqueColumnPermutations([func](const Rows &r) {
		enumerateRowPermutations(func, r);
	}, rows);
}

void MaskSymmetry::enumerateSymmetries(const Mask81 &originalMask, vector<Mask81> &res) {
	res.clear();
	enumerateUniquePermutations([&res](const Rows &rows) {
		res.push_back(rows.toMask81());
	}, Rows::fromMask81(originalMask));
}

int MaskSymmetry::calculateSymmetryOrder(const Mask81 & mask) {
	int res = 0;
	enumerateUniqueColumnPermutations([&res](const Rows &rows) {
		res += rows.getUniquePermutationPattern().calculateSymmetryOrder();
	}, Rows::fromMask81(mask));
	return res;
}

int MaskSymmetry::getUsableBands(const PartialRows &used, const UniquePermutationPatterns &upat, int prevBandIndex) {
	int usableBands;
	if(prevBandIndex != -1) {
		usableBands = 1 << prevBandIndex;
	} else {
		usableBands = 0;
		rep(band, 3) if((used.used >> (band * 3) & 7) == 0)
			usableBands |= 1 << band;
		if((upat.bandPattern & 2) && (usableBands & 6) == 6)
			usableBands &= ~6;
		if((upat.bandPattern & 1) && (usableBands & 3) == 3)
			usableBands &= ~2;
	}
	return usableBands;
}

int MaskSymmetry::getUsableIndices(int band, const PartialRows &used, const UniquePermutationPatterns &upat) {
	int usableIndices = 7 - (used.used >> (band * 3) & 7);
	return getUniqueIndicesFor(usableIndices, upat.rowPatterns[band]);
}

int MaskSymmetry::getUniqueIndicesFor(int usableIndices, int pattern) {
	if((pattern & 2) && (usableIndices & 6) == 6)
		usableIndices &= ~4;
	if((pattern & 1) && (usableIndices & 3) == 3)
		usableIndices &= ~2;
	return usableIndices;
}

template<typename Func>
static void dfsNext(Func func, int newRow, PartialRows &used, const UniquePermutationPatterns &upat) {
	int newBand = newRow / 3;
	int prevBandIndex = (signed char)used.bandIndex[newBand];
	int usableBands = MaskSymmetry::getUsableBands(used, upat, prevBandIndex);
	rep(band, 3) if(usableBands >> band & 1) {
		used.bandIndex[newBand] = band;
		int usableIndices = MaskSymmetry::getUsableIndices(band, used, upat);

		rep(index, 3) if(usableIndices >> index & 1) {
			int row = band * 3 + index;
			used.used |= 1 << row;
			func(row);
			used.used &= ~(1 << row);
		}
	}
	used.bandIndex[newBand] = prevBandIndex;
}

static int countAvoidingSymmetriesDfs(int i, PartialRows &used, const UniquePermutationPatterns &upat, const Rows &rows, const Rows &pat) {
	if(i == 9)
		return 1;
	int newRow = i;
	int res = 0;
	dfsNext([&](int row) {
		if((pat.getRowReversed(newRow) & rows.getRowReversed(row)) != 0)
			return;
		res += countAvoidingSymmetriesDfs(i + 1, used, upat, rows, pat);
	}, newRow, used, upat);
	return res;
}

int MaskSymmetry::countAvoidingSymmetries(const Mask81 &mask, const Mask81 &pattern) {
	int res = 0;
	Rows pat = Rows::fromMask81(pattern);
	enumerateUniqueColumnPermutations([&res, pat](const Rows &rows) {
		auto upat = rows.getUniquePermutationPattern();
		PartialRows used;
		res += countAvoidingSymmetriesDfs(0, used, upat, rows, pat);
	}, Rows::fromMask81(mask));
	return res;
}

DiagramNode *MaskSymmetry::RowPermutationDiagram::newNode() {
	if(_measureMode) {
		++ _totalNodes;
		return nullptr;
	} else {
		return new(&_nodePool[_totalNodes ++]) DiagramNode();
	}
}

DiagramLeaf *MaskSymmetry::RowPermutationDiagram::newLeaf(int originalMaskIndex) {
	if(_measureMode) {
		++ _totalLeaves;
		return nullptr;
	} else {
		return new(&_leafPool[_totalLeaves ++]) DiagramLeaf(originalMaskIndex);
	}
}

DiagramEdge MaskSymmetry::RowPermutationDiagram::makeEdge(uint16_t rowMask, const DiagramNodeOrLeaf *node) const {
	bool toLeaf;
	uint32_t offset;
	if(_leafPool.get() <= node && node < _leafPool.get() + _totalLeaves) {
		toLeaf = true;
		offset = (uint32_t)(static_cast<const DiagramLeaf*>(node) - _leafPool.get());
	} else {
		toLeaf = false;
		offset = (uint32_t)(static_cast<const DiagramNode*>(node) - _nodePool.get());
	}
	return DiagramEdge(rowMask, toLeaf, offset);
}

static uint32_t toPartialHashBandRows(const Rows::BandRows &band, int mask) {
	Rows::BandRows t{ {} };
	int k = 0;
	for(int i : EachBit(mask))
		t.rows[k ++] = band.rows[i];
	return t.toHash();
}

void MaskSymmetry::RowPermutationDiagram::buildDiagramForBand(DiagramNode * bandRoot, const vector<BandNodeEdge> &bandNodeEdges) {
	unordered_map<uint32_t, pair<DiagramNodeOrLeaf*,vector<DiagramEdge>>> nodeMaps[3];

	auto makeNode = [&nodeMaps, this](const Rows::BandRows &band, int mask) -> vector<DiagramEdge>& {
		uint32_t h = toPartialHashBandRows(band, mask);
		auto p = nodeMaps[countOneBits(mask)].emplace(make_pair(h, make_pair(nullptr, vector<DiagramEdge>())));
		if(p.second)
			p.first->second.first = newNode();
		return p.first->second.second;
	};

	nodeMaps[0][toPartialHashBandRows(Rows::BandRows{ {} }, 0)].first = bandRoot;

	for(const auto &edge : bandNodeEdges) {
		DiagramNodeOrLeaf *childNode = edge.to;
		Rows::BandRows bandRows = edge.bandRows;
		int pattern = bandRows.getUniqueRowPermutationPattern();

		makeNode(bandRows, 1 << 0);
		if((pattern & 1) == 0) makeNode(bandRows, 1 << 1);
		if((pattern & 2) == 0) makeNode(bandRows, 1 << 2);

		int uniqueIndices = 1 << 2;
		if((pattern & 2) == 0) uniqueIndices |= 1 << 1;
		if((pattern & 1) == 0) uniqueIndices |= 1 << 0;

		for(int i : EachBit(uniqueIndices)) {
			auto &v = makeNode(bandRows, (1 << 0 | 1 << 1 | 1 << 2) & ~(1 << i));
			if(_measureMode)
				++ _totalEdges;
			else
				v.push_back(makeEdge(bandRows.rows[i], childNode));
		}
	}

	for(int k = 1; k <= 2; ++ k) {
		for(const auto &p : nodeMaps[k]) {
			Rows::BandRows bandRows = Rows::BandRows::fromHash(p.first);
			DiagramNodeOrLeaf *node = p.second.first;
			int uniqueIndices;
			if(k == 2) {
				if(bandRows.rows[0] == bandRows.rows[1])
					uniqueIndices = 1 << 1;
				else
					uniqueIndices = 1 << 0 | 1 << 1;
			} else {
				uniqueIndices = 1 << 0;
			}

			for(int i : EachBit(uniqueIndices)) {
				uint32_t parentHash = toPartialHashBandRows(bandRows, ((1 << k) - 1) & ~(1 << i));

				if(_measureMode) {
					++ _totalEdges;
				}else {
					nodeMaps[k - 1][parentHash].second.push_back(makeEdge(bandRows.rows[i], node));
				}
			}
		}
	}

	if(!_measureMode) {
		for(int k = 0; k <= 2; ++ k) {
			for(const auto &p : nodeMaps[k]) {
				DiagramNode *node = static_cast<DiagramNode*>(p.second.first);
				assert(node != nullptr);

				const vector<DiagramEdge> &edgeList = p.second.second;
				node->_beginOffset = (uint32_t)_totalEdges;
				node->_endOffset = (uint32_t)(_totalEdges + edgeList.size());

				for(const DiagramEdge &edge : edgeList)
					_edgePool[_totalEdges ++] = edge;
			}
		}
	}
}

static uint64_t toPartialBandsHash(const Rows &rows, int mask) {
	assert(countOneBits(mask) <= 2);
	uint64_t res = 0;
	for(int i : EachBit(mask))
		res = res << 32 | rows.getBandReversed(i).toHash();
	return res;
}

static Rows fromPartialBandsHash(int k, uint64_t h) {
	Rows res;
	for(int i = k - 1; i >= 0; -- i) {
		auto bandRows = Rows::BandRows::fromHash((uint32_t)h);
		res.setBandReversed(i, bandRows);
		h >>= 32;
	}
	return res;
}


DiagramNode *MaskSymmetry::RowPermutationDiagram::buildDiagramMain(const vector<DiagramEntry> &entries) {
	struct BandNode {
		DiagramNode *node;
		vector<BandNodeEdge> edges;

		BandNode() : node(nullptr), edges() {}
	};

	unordered_map<uint64_t, BandNode> nodeMaps[3];

	auto makeNode = [&nodeMaps, this](const Rows &rows, int mask) -> BandNode& {
		auto h = toPartialBandsHash(rows, mask);
		auto p = nodeMaps[countOneBits(mask)].emplace(make_pair(h, BandNode()));
		if(p.second)
			p.first->second.node = newNode();

		return p.first->second;
	};

	DiagramNode *rootNode = newNode();
	nodeMaps[0][toPartialBandsHash(Rows(), 0)].node = rootNode;

	for(const auto &entry : entries) {
		const Rows &rows = entry.rows;
		int pattern = rows.getUniquePermutationBandPattern();

		DiagramLeaf *leaf = newLeaf(entry.originalMaskIndex);

		makeNode(rows, 1 << 0);
		if((pattern & 1) == 0) makeNode(rows, 1 << 1);
		if((pattern & 2) == 0) makeNode(rows, 1 << 2);

		makeNode(rows, 1 << 0 | 1 << 1)
			.edges.push_back(BandNodeEdge{ leaf, rows.getBandReversed(2) });
		if((pattern & 2) == 0) {
			makeNode(rows, 1 << 0 | 1 << 2)
				.edges.push_back(BandNodeEdge{ leaf, rows.getBandReversed(1) });
		}
		if((pattern & 1) == 0) {
			makeNode(rows, 1 << 1 | 1 << 2)
				.edges.push_back(BandNodeEdge{ leaf, rows.getBandReversed(0) });
		}
	}

	for(int k = 1; k <= 2; ++ k) {
		for(const auto &p : nodeMaps[k]) {
			Rows rows = fromPartialBandsHash(k, p.first);
			DiagramNode *node = p.second.node;

			int uniqueBands;
			if(k == 2) {
				if(rows.getBandReversed(0) == rows.getBandReversed(1))
					uniqueBands = 1 << 1;
				else
					uniqueBands = 1 << 0 | 1 << 1;
			} else {
				uniqueBands = 1 << 0;
			}

			for(int i : EachBit(uniqueBands)) {
				int mask = k == 2 ? (1 << 0 | 1 << 1) & ~(1 << i) : 0;
				auto parentHash = toPartialBandsHash(rows, mask);

				auto it = nodeMaps[k - 1].find(parentHash);
				assert(it != nodeMaps[k - 1].end());

				it->second.edges.push_back(BandNodeEdge{node, rows.getBandReversed(i)});
			}
		}
	}

	for(int k = 0; k <= 2; ++ k) {
		for(const auto &p : nodeMaps[k]) {
			buildDiagramForBand(p.second.node, p.second.edges);
		}
	}

	return rootNode;
}

void MaskSymmetry::RowPermutationDiagram::buildDiagramForMasks(const vector<Mask81> &masks, Logger *infoLogger) {
	size_t entriesSize = 0;
	for(const Mask81 &mask : masks) {
		enumerateUniqueColumnPermutations([&entriesSize](const Rows &) {
			++ entriesSize;
		}, Rows::fromMask81(mask));
	}
	infoLogger->log("total leaves = ", entriesSize);
	long long totalSymmetries = 0;
	vector<DiagramEntry> entries; entries.reserve(entriesSize);
	rep(i, masks.size()) {
		enumerateUniqueColumnPermutations([i, &entries, &totalSymmetries](const Rows &rows) {
			auto pat = rows.getUniquePermutationPattern();
			entries.push_back(DiagramEntry{rows, i});
			totalSymmetries += pat.calculateSymmetryOrder();
		}, Rows::fromMask81(masks[i]));
	}
	infoLogger->log("total symmetries = ", totalSymmetries);

	_measureMode = true;
	_totalNodes = _totalLeaves = _totalEdges = 0;

	buildDiagramMain(entries);

	assert(_totalLeaves == entries.size());
	infoLogger->log("total nodes = ", _totalNodes);
	infoLogger->log("total edges = ", _totalEdges);
	infoLogger->log("memory: ", double(_totalNodes * sizeof(DiagramNode) + _totalLeaves * sizeof(DiagramLeaf) + _totalEdges * sizeof(DiagramEdge)) / (1024 * 1024), "MiB");

	assert(_totalNodes <= _totalEdges && _totalLeaves <= _totalEdges);
	if(_totalEdges >= numeric_limits<uint32_t>::max()) {
		cerr << "error: too many edges" << endl;
		abort();
	}

	_nodePool.reset(new DiagramNode[_totalNodes]);
	_leafPool.reset(new DiagramLeaf[_totalLeaves]);
	_edgePool.reset(new DiagramEdge[_totalEdges]);

	_measureMode = false;
	_totalNodes = _totalLeaves = _totalEdges = 0;

	_root = buildDiagramMain(entries);

	infoLogger->log("built");
}
