#include <sifteo.h>
using namespace Sifteo;

int luaGetInteger(const char *expr)
{
    int result;
    SCRIPT_FMT(LUA, "Runtime():poke(%p, %s)", &result, expr);
    return result;
}

void luaSetInteger(const char *varName, int value)
{
    SCRIPT_FMT(LUA, "%s = %d", varName, value);
}

void main()
{
    SCRIPT(LUA,
        foobar = 0
        System():setOptions{ numCubes = 4 }
    );
    ASSERT(luaGetInteger("foobar") == 0);

    luaSetInteger("foobar", 100);
    ASSERT(luaGetInteger("foobar") == 100);
    ASSERT(luaGetInteger("System():numCubes()") == 4);

    SCRIPT(LUA, Cube(0):saveScreenshot("myScreenshot.png"));
    SCRIPT(LUA, Cube(0):testScreenshot("myScreenshot.png"));

    SCRIPT_FMT(LUA, "Frontend():postMessage('Power is >= %d')", 9000);

    SCRIPT(LUA, System():setOptions{ svmTrace = true });
    SCRIPT(LUA, System():setOptions{ svmTrace = false });

    LOG("Success.\n");
}
