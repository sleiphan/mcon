#include <gtest/gtest.h>

extern "C" {
    #include <mcon/mcon.h>
}

TEST(mcon, placeholder) {
    struct mcon* mcon;
    if (mcon_create(&mcon, mcon_config_default))
        FAIL();
}
