#include <gtest/gtest.h>

extern "C" {
    #include <mcon/placeholder.h>
}

TEST(mcon, placeholder) {
    const int expected = 3;
    const int actual = sum(1, 2);

    EXPECT_EQ(expected, actual);
}
