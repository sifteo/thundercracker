/*
 * Infinite loop test.
 *
 * This generates an infinite loop with no system calls or SVCs at all
 * inside. For testing the OS's ability to escape from this.
 */

#include <sifteo.h>

void main()
{
    Sifteo::Metadata().title("Infinite Loop");
    while (1);
}
