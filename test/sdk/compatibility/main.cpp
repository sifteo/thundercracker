/*
 * Explicit tests for binary- and source-compatibility features.
 */

#include <sifteo.h>
using namespace Sifteo;

void testFeatureBits()
{
    /*
     * This is a placeholder for future compatibility bits which can
     * be used to version parts of our ABI. When we add a new group of
     * features, a new bit in this mask will be set.
     *
     * Currently no such features exist, but we still want to verify
     * that this syscall is callable without faulting, and that it returns
     * the expected value of zero.
     *
     * Note that _SYS_getFeatures() has no direct equivalent in the
     * documented SDK. The SDK in the future will need to either provide
     * transparent fallback mechanisms for supporting older firmwares, or
     * they will need to return an appropriate error or export a feature
     * test predicate such that the game can deal with the error as best
     * it can (for example, by asking the user to upgrade firmware).
     */

    ASSERT(_SYS_getFeatures() == 0);
}

void main()
{
    testFeatureBits();

    LOG("Success.\n");
}
