// Macros that work in gtest rather than simulator.

#import <assert.h>
#pragma warning "hi there.!!!!!"

#define ASSERT(_x) do { \
    assert(_x)
} while (0)
