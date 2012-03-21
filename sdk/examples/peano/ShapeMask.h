#pragma once

#include "sifteo.h"
#include <stddef.h>

using namespace Sifteo::Math;

namespace TotalsGame {

	struct Connection {
        Vector2<int> pos;
        Vector2<int> dir;

        bool Matches(const Connection &c);

        bool IsFromOrigin();

        bool IsBottom();

        bool IsRight();
	};

	struct ShapeMask {
        Vector2<int> size;
		long bits;

		static const ShapeMask Zero;
		static const ShapeMask Unity;

        ShapeMask(Vector2<int> size, bool *flags, size_t numFlags);

        ShapeMask(Vector2<int> size, long bits);

        ShapeMask();

        ShapeMask GetRotation();
        ShapeMask GetReflection();

        bool BitAt(Vector2<int> p) const;

        ShapeMask SubMask(Vector2<int> p, Vector2<int> s);

        bool Matches(const ShapeMask &mask);
		
        void ListOutConnections(Connection *connections, int *numConnections, int maxConnections);

        void ListInConnections(Connection *connections, int *numConnections, int maxConnections);


		static bool TryConcat(
            const ShapeMask &m1, const ShapeMask &m2, Vector2<int> offset,
            ShapeMask *result, Vector2<int> *d1, Vector2<int> *d2
            );
    };

}
