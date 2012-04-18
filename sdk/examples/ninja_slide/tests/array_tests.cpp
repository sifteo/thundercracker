#include <gtest/gtest.h>
#include "array.h"

class C { 
  public:
    int i;
};

TEST(ArrayTest, arrayOfClass) {
    Array<C, 4> a;
    EXPECT_EQ((unsigned)0, a.count());
    EXPECT_TRUE(a.empty());

    C c4 = {4};
    a.append(c4);
    EXPECT_EQ(4, a[0].i);

    a.clear()
    EXPECT_TRUE(a.empty());

//     a.append(4);
//     EXPECT_EQ(1, a.count());
}
