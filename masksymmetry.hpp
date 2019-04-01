#pragma once
#include "mask81.hpp"
#include "logger.hpp"
#include <cstdint>
#include <vector>
#include <utility>
#include <cstring>
#include <functional>
#include <memory>

class MaskSymmetry {
public:
	struct UniquePermutationPatterns {
		unsigned char bandPattern;
		unsigned char rowPatterns[3];

		int calculateSymmetryOrder() const;
	};

	struct PartialRows {
		uint16_t used;
		unsigned char bandIndex[3];

		PartialRows() : used(0) { rep(k, 3) bandIndex[k] = -1; }
	};

	class Rows {
	public:
		struct BandRows {
			uint16_t rows[3];

			uint32_t toHash() const;
			static BandRows fromHash(uint32_t h);

			int getUniqueRowPermutationPattern() const;

			void canonicalize();

			bool operator==(const BandRows &that) const;
			bool operator!=(const BandRows &that) const { return !(*this == that); }
		};

		Rows() { std::memset(_rows, 0, sizeof _rows); }

		uint16_t getRowReversed(int i) const { return _rows[i]; }
		BandRows getBandReversed(int i) const { return _bands[i]; }

		void setRowReversed(int i, uint16_t x) { _rows[i] = x; }
		void setBandReversed(int i, const BandRows &x) { _bands[i] = x; }

		static Rows fromMask81(const Mask81 &mask);
		Mask81 toMask81() const;

		Rows transpose() const;

		void swapRows(int i, int j);
		void swapBands(int i, int j);

		void canonicalize();

		bool operator==(const Rows &that) const;
		bool operator!=(const Rows &that) const { return !(*this == that); }

		friend struct std::hash<MaskSymmetry::Rows>;

		int getUniquePermutationBandPattern() const;
		UniquePermutationPatterns getUniquePermutationPattern() const;

		static const std::pair<int, int> uniquePermutationSwapPatterns[4][7];
		static const int uniquePermutationOrders[4];

	private:
		static void sortBand(BandRows &x);
		static void sortTwoBands(BandRows &x, BandRows &y);

	private:
		//(row, col): _rows[row] >> (8 - col) & 1
		union {
			uint16_t _rows[9];
			BandRows _bands[3];
		};
	};
	friend struct std::hash<MaskSymmetry::Rows>;

public:
	static void enumerateSymmetries(const Mask81 &originalMask, std::vector<Mask81> &res);
	static int calculateSymmetryOrder(const Mask81 &mask);
	//|{ m | m \in symmetries(mask), m \cap pattern = {} }|
	static int countAvoidingSymmetries(const Mask81 &mask, const Mask81 &pattern);

	class RowPermutationDiagram {
		struct DiagramEntry {
			Rows rows;
			int originalMaskIndex;
		};

	public:
		struct DiagramNodeOrLeaf {

		};

		struct DiagramEdge {
			uint16_t _rowMask;
			bool _toLeaf;
			uint32_t _nodeOffset;

			DiagramEdge() : _rowMask(0), _toLeaf(false), _nodeOffset(-1) {}
			DiagramEdge(uint16_t rowMask, bool toLeaf, uint32_t nodeOffset) : _rowMask(rowMask), _toLeaf(toLeaf), _nodeOffset(nodeOffset) {}

			uint16_t getRowMask() const { return _rowMask; }
		};

		struct DiagramLeaf : DiagramNodeOrLeaf {
			int originalMaskIndex;

			DiagramLeaf() : originalMaskIndex(-1) {}
			DiagramLeaf(int originalMaskIndex) : originalMaskIndex(originalMaskIndex) {}
		};

		struct DiagramNode : DiagramNodeOrLeaf {
			DiagramNode() : _beginOffset(-1), _endOffset(-1) {}

			uint32_t _beginOffset, _endOffset;
		};

		RowPermutationDiagram() : _root(nullptr) { }

		void buildDiagramForMasks(const std::vector<Mask81> &masks, Logger *infoLogger);

		const DiagramNode *getRoot() const { return _root; }

		const DiagramEdge *edgeListBegin(const DiagramNode *node) const { return _edgePool.get() + node->_beginOffset; }
		const DiagramEdge *edgeListEnd(const DiagramNode *node) const { return _edgePool.get() + node->_endOffset; }

		const DiagramNodeOrLeaf *getEdgeHead(const DiagramEdge *edge) const {
			if(edge->_toLeaf)
				return _leafPool.get() + edge->_nodeOffset;
			else
				return _nodePool.get() + edge->_nodeOffset;
		}

	private:
		struct BandNodeEdge {
			DiagramNodeOrLeaf *to;
			Rows::BandRows bandRows;
		};

		DiagramNode *newNode();
		DiagramLeaf *newLeaf(int originalMaskIndex);
		DiagramEdge makeEdge(uint16_t rowMask, const DiagramNodeOrLeaf *node) const;

		DiagramNode *buildDiagramMain(const std::vector<DiagramEntry> &entries);
		void buildDiagramForBand(DiagramNode *bandRoot, const std::vector<BandNodeEdge> &bandNodeEdges);

		DiagramNode *_root;
		bool _measureMode;
		size_t _totalNodes, _totalLeaves, _totalEdges;
		std::unique_ptr<DiagramNode[]> _nodePool;
		std::unique_ptr<DiagramLeaf[]> _leafPool;
		std::unique_ptr<DiagramEdge[]> _edgePool;
	};

	static int getUsableBands(const PartialRows &used, const UniquePermutationPatterns &upat, int prevBandIndex);
	static int getUsableIndices(int band, const PartialRows &used, const UniquePermutationPatterns &upat);
	static int getUniqueIndicesFor(int usableIndices, int pattern);
};

namespace std {
template<> struct hash<MaskSymmetry::Rows> {
	size_t operator()(const MaskSymmetry::Rows &rows) const {
		size_t res = 0;
		rep(i, 3) res = res * 1000000007U + rows._bands[i].toHash();
		return res;
	}
};
}
