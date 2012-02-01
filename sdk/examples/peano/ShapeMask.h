#pragma once

#include "sifteo.h"


namespace TotalsGame {

	struct ShapeMask {
		Vec2 size;
		long bits;

		static const ShapeMask Zero;
		static const ShapeMask Unity;

		ShapeMask(Vec2 size, bool *flags, size_t numFlags) {
			this->size = size;
			this->bits = 0L;
			for(int x=0; x<size.x; ++x) {
				for(int y=0; y<size.y; ++y) {
					size_t index = (y * size.x + x);
					assert(index < numFlags);
					if (flags[index]) {
						bits |= (1L<<index);
					}
				}
			}
		}

		ShapeMask(Vec2 size, long bits) {
			this->size = size;
			this->bits = bits;
		}

		ShapeMask() : size(0,0)
		{
			bits = 0;
		}

		ShapeMask GetRotation() {
			Vec2 s = Vec2(size.y, size.x);
			long bts = 0L;
			for(int x=0; x<s.x; ++x) {
				for(int y=0; y<s.y; ++y) {
					if (BitAt(Vec2(y, size.y-1-x))) {
						int shift = y * s.x + x;
						bts |= (1L<<shift);
					}
				}
			}
			return ShapeMask(s, bts);
		}

		ShapeMask GetReflection() {
			Vec2 s = Vec2(size.x, size.y);
			long bts = 0L;
			for(int x=0; x<s.x; ++x) {
				for(int y=0; y<s.y; ++y) {
					if (BitAt(Vec2(x, size.y-1-y))) {
						int shift = y * s.x + x;
						bts |= (1L<<shift);
					}
				}
			}
			return ShapeMask(s, bts);
		}

		bool BitAt(Vec2 p) {
			if (p.x < 0 || p.x >= size.x || p.y < 0 || p.y >= size.y) { return false; }
			int shift = p.y * size.x + p.x;
			return (bits & (1L<<shift))!= 0L;
		}

		ShapeMask SubMask(Vec2 p, Vec2 s) {
			long bts = 0L;
			for(int x=0; x<s.x; ++x) {
				for(int y=0; y<s.y; ++y) {
					if (BitAt(p+Vec2(x,y))) {
						int shift = y * s.x + x;
						bts |= (1L<<shift);
					}
				}
			}
			return ShapeMask(s, bts);
		}

		bool Matches(const ShapeMask &mask) {
			return size.x == mask.size.x && size.y == mask.size.y && bits == mask.bits;
		}
		/*
		public IEnumerable<Connection> ListOutConnections() {
		Int2 p;
		for(p.x=0; p.x<size.x; ++p.x) {
		for(p.y=0; p.y<size.y; ++p.y) {
		if (BitAt(p)) {
		if (!BitAt(p+Int2.Right)) {
		yield return new Connection() { pos = p, dir = Int2.Right };
		}
		if (!BitAt(p+Int2.Down)) {
		yield return new Connection() { pos = p, dir = Int2.Down };
		}
		}
		}
		}
		}

		public IEnumerable<Connection> ListInConnections() {
		Int2 p;
		for(p.x=0; p.x<size.x; ++p.x) {
		for(p.y=0; p.y<size.y; ++p.y) {
		if (BitAt(p)) {
		if (!BitAt(p-Int2.Right)) {
		yield return new Connection() { pos = p, dir = Int2.Right };
		}
		if (!BitAt(p-Int2.Down)) {
		yield return new Connection() { pos = p, dir = Int2.Down };
		}
		}
		}
		}
		}
		*/

		static bool TryConcat(
			ShapeMask m1, ShapeMask m2, Vec2 offset, 
			ShapeMask *result,Vec2 *d1, Vec2 *d2
			) 
		{
			Vec2 min = Vec2(
				Math::min(offset.x, 0),
				Math::min(offset.y, 0)
				);
			Vec2 max = Vec2(
				Math::max(m1.size.x, offset.x + m2.size.x),
				Math::max(m1.size.y, offset.y + m2.size.y)
				);
			long newbits = 0L;
			Vec2 newsize = max - min;
			Vec2 p;
			for(p.x = min.x; p.x < max.x; ++p.x)
			{
				for(p.y = min.y; p.y < max.y; ++p.y)
				{
					bool b1 = m1.BitAt(p);
					bool b2 = m2.BitAt(p - offset);
					if (b1 && b2)
					{ 
						*result = Zero;
						d1->set(0,0);
						d2->set(0,0);
						return false; 
					}
					if (b1 || b2) 
					{
						Vec2 d = p - min;
						int shift = d.y * newsize.x + d.x;
						newbits |= (1L<<shift);
					}
				}
			}
			*result = ShapeMask(newsize, newbits);
			//d1 = min - Int2.Zero;
			*d1 = Vec2(0,0) - min;

			*d2 = offset - min;
			return true;
		}


	};


}
