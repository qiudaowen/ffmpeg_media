#pragma once

//BitSet
#define QmBitUnit sizeof(unsigned int)
template<size_t size>
struct QsBitSet
{
	QsBitSet() { zero(); }
	void zero() { memset(this, 0, sizeof(*this)); }
	void setall() { memset(this, 0xFF, sizeof(*this)); }
	void set(int i) { bits[i / QmBitUnit] |= (1 << (i & (QmBitUnit - 1))); }
	void unset(int i) { bits[i / QmBitUnit] &= ~(1 << (i & (QmBitUnit - 1))); }
	unsigned int operator[](int i) const { return (bits[i / QmBitUnit] & (1 << (i & (QmBitUnit - 1)))); }
protected:
	unsigned int bits[(size + QmBitUnit - 1) / QmBitUnit];
};

#define QmBitTest(bit, ptr) ( ((unsigned char*)ptr)[bit/8] & (1<<(bit%8)))
#define QmBitSet(bit, ptr)  ((unsigned char*)ptr)[bit/8] |= (1 << (bit & (8 - 1)))