#include <sifteo.h>

static Sifteo::Metadata M = Sifteo::Metadata()
    .title("Abort fault test")
    .package("com.sifteo.extras.abort", "1.0")
    .cubeRange(0);

void main()
{
    _SYS_abort();
}
