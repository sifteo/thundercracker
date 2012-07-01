/*
 * Infinite loop test.
 *
 * This generates an infinite loop with no system calls or SVCs at all
 * inside. For testing the OS's ability to escape from this.
 */

#include <sifteo.h>

static Sifteo::Metadata M = Sifteo::Metadata()
    .title("Infinite Loop")
    .package("com.sifteo.extras.loopy", "1.0")
    .cubeRange(0);

void main()
{
    while (1);
}
