/*
 * This test exposes a subtle bug in slinky's symbol resolver. 
 *
 * Due to the way the optimizer calculates a base address for 'array',
 * the symbol has an offset of -16 applied via symbol decoration, followed
 * by an offset of +12 applied via a subsequent 'add' instruction.
 *
 * This triggers a bug in the symbol decorator due to the '-' character
 * in the negative offset being mangled. The negative offset is lost, and
 * as a result, we have a net offset of +12 instead of -4. This causes us to
 * read past the end of the array.
 */

#include <sifteo.h>
using namespace Sifteo;

int item;
static const int *array[] = { &item, &item, &item, &item, &item };

void main()
{
    for (int i = 0; i < 3; i++) {
        if (i > 0) {
            const int *p = array[i-1];
            LOG("%d %p %p\n", i, array, p);
            if (!p) _SYS_abort();
        }
    }
    LOG("Success.\n");
}
