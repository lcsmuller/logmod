#include "../logmod.h"
#include "greatest.h"

#define TABLE_SIZE 5

TEST
should_initialize_application(void)
{
    const char *application_id = "APPLICATION_A";
    struct logmod_logger table[TABLE_SIZE];
    struct logmod logmod;
    logmod_err code;

    code = logmod_init(&logmod, application_id, table,
                       sizeof(table) / sizeof *table);
    ASSERT_EQ(code, LOGMOD_OK);
    ASSERT_EQ(logmod.application_id, application_id);
    ASSERT_EQ(logmod.loggers, table);
    ASSERT_EQ(logmod.length, 0);
    ASSERT_EQ(logmod.real_length, TABLE_SIZE);

    PASS();
}

TEST
should_initialize_context(void)
{
    const char *application_id = "APPLICATION_A";
    const char *context_id = "MODULE_A";
    struct logmod_logger *LOGMOD_LOGGER, table[TABLE_SIZE];
    struct logmod logmod;

    logmod_init(&logmod, application_id, table, sizeof(table) / sizeof *table);
    LOGMOD_LOGGER = logmod_logger_get(&logmod, context_id);
    ASSERT_EQ(LOGMOD_LOGGER, &table[0]);
    ASSERT_EQ(logmod.length, 1);

    PASS();
}

#undef TABLE_SIZE

SUITE(initialization)
{
    RUN_TEST(should_initialize_application);
    RUN_TEST(should_initialize_context);
}

GREATEST_MAIN_DEFS();

int
main(int argc, char *argv[])
{
    GREATEST_MAIN_BEGIN();

    RUN_SUITE(initialization);

    GREATEST_MAIN_END();
}
