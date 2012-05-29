#include <sifteo.h>
using namespace Sifteo;

static Metadata M = Metadata()
    .title("Async Open Test")
    .cubeRange(1);

void main()
{
    SCRIPT(LUA,
        f = os.getenv("TEST_STAMP_FILE")
        if f then
            io.close(io.open(f, "w"))
        end

        os.exit(0)
    );

    LOG("Success.\n");
}
