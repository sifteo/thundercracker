#include <sifteo.h>

static Sifteo::Metadata M = Sifteo::Metadata()
    .title("Abort fault test")
    .package("com.sifteo.extras.abort", "1.0")
    .cubeRange(0, CUBE_ALLOCATION);

void main()
{
    _SYS_abort();
}
