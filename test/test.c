#include "../logmod.h"
#include "greatest.h"

TEST
should_initialize(void)
{
    struct logmod *logmod = logmod_init("APPLICATION_A");
    struct logmod_logger *LOGMOD_LOGGER =
        logmod_logger_get(logmod, "MODULE_A");

    ASSERT_NEQ(logmod, NULL);
    ASSERT_NEQ(LOGMOD_LOGGER, NULL);

    PASS();
}

SUITE(initialization)
{
    RUN_TEST(should_initialize);
}

GREATEST_MAIN_DEFS();

int
main(int argc, char *argv[])
{
    GREATEST_MAIN_BEGIN();

    RUN_SUITE(initialization);

    GREATEST_MAIN_END();
}
