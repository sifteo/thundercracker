#pragma once

#include "sifteo.h"
#include <stddef.h>

namespace TotalsGame {

	struct Connection {
		Vec2 pos;
		Vec2 dir;

        bool Matches(const Connection &c);

        bool IsFromOrigin();

        bool IsBottom();

        bool IsRight();
	};

	struct ShapeMask {
		Vec2 size;
		long bits;

		static const ShapeMask Zero;
		static const ShapeMask Unity;

        ShapeMask(Vec2 size, bool *flags, size_t numFlags);

        ShapeMask(Vec2 size, long bits);

        ShapeMask();

        ShapeMask GetRotation();
        ShapeMask GetReflection();

        bool BitAt(Vec2 p);

        ShapeMask SubMask(Vec2 p, Vec2 s);

        bool Matches(const ShapeMask &mask);
		
        void ListOutConnections(Connection *connections, int *numConnections, int maxConnections);

        void ListInConnections(Connection *connections, int *numConnections, int maxConnections);


		static bool TryConcat(
			ShapeMask m1, ShapeMask m2, Vec2 offset, 
			ShapeMask *result,Vec2 *d1, Vec2 *d2
            );
    };

}
