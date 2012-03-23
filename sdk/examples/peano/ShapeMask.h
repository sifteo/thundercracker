#pragma once

#include "config.h"
#include "sifteo.h"
#include <stddef.h>

using namespace Sifteo;

namespace TotalsGame {

	struct Connection {
        Int2 pos;
        Int2 dir;

        bool Matches(const Connection &c);

        bool IsFromOrigin();

        bool IsBottom();

        bool IsRight();
	};

	struct ShapeMask {
        Int2 size;
		long bits;

		static const ShapeMask Zero;
		static const ShapeMask Unity;

        ShapeMask(Int2 size, bool *flags, size_t numFlags);

        ShapeMask(Int2 size, long bits);

        ShapeMask();

        ShapeMask GetRotation();
        ShapeMask GetReflection();

        bool BitAt(Int2 p) const;

        ShapeMask SubMask(Int2 p, Int2 s);

        bool Matches(const ShapeMask &mask);
		
        void ListOutConnections(Connection *connections, int *numConnections, int maxConnections);

        void ListInConnections(Connection *connections, int *numConnections, int maxConnections);

		static bool TryConcat(
            const ShapeMask &m1, const ShapeMask &m2, Int2 offset,
            ShapeMask *result, Int2 *d1, Int2 *d2
            );
    };

}
