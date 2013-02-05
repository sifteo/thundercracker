/*
 * Explicit tests for binary- and source-compatibility features.
 */

#include <sifteo.h>
using namespace Sifteo;

void testFeatureBits()
{
    /*
     * This is a test for the compatibility bits which can
     * be used to version parts of our ABI. When we add a new group of
     * features, a new bit in this mask will be set.
     *
     * Note that _SYS_getFeatures() has no direct equivalent in the
     * documented SDK. The SDK must either provide
     * transparent fallback mechanisms for supporting older firmwares, or
     * return an appropriate error or export a feature
     * test predicate such that the game can deal with the error as best
     * it can (for example, by asking the user to upgrade firmware).
     */

    // NOTE: explicitly list each feature bit here to ensure _SYS_getFeatures()
    // is returning the appropriate value in the event that the feature flags
    // have been updated
    ASSERT(_SYS_getFeatures() == _SYS_FEATURE_SYS_VERSION);

    // _SYS_FEATURE_SYS_VERSION - this syscall didn't exist on older OS versions,
    // so make sure we don't fault when calling it, if it's advertised as available
    ASSERT(_SYS_version() != _SYS_OS_VERSION_NONE);
}

void main()
{
    testFeatureBits();

    LOG("Success.\n");
}
