#include "../logmod.h"
#include "greatest.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TABLE_SIZE 5

TEST
should_initialize_application(void)
{
    static const char *const application_id = "APPLICATION_A";
    struct logmod_logger table[TABLE_SIZE];
    struct logmod logmod;

    logmod_err code = logmod_init(&logmod, application_id, table,
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
    static const char *const application_id = "APPLICATION_A";
    static const char *const context_id = "MODULE_A";
    struct logmod_logger *LOGMOD_LOGGER, table[TABLE_SIZE];
    struct logmod logmod;

    logmod_init(&logmod, application_id, table, sizeof(table) / sizeof *table);
    LOGMOD_LOGGER = logmod_logger_get(&logmod, context_id);
    ASSERT_EQ(LOGMOD_LOGGER, &table[0]);
    ASSERT_EQ(logmod.length, 1);

    PASS();
}

/* Custom lock function for testing */
static int lock_was_called = 0;
static void
test_lock_function(int should_lock)
{
    lock_was_called = should_lock ? 1 : 0;
}

TEST
should_set_logger_options(void)
{
    static const char *const application_id = "APPLICATION_A";
    static const char *const context_id = "MODULE_A";
    struct logmod_logger *LOGMOD_LOGGER, table[TABLE_SIZE];
    struct logmod logmod;
    struct logmod_logger_options options = { 0 };
    FILE *fp = tmpfile();
    logmod_err code;

    /* Initialize logger */
    logmod_init(&logmod, application_id, table, sizeof(table) / sizeof *table);
    LOGMOD_LOGGER = logmod_logger_get(&logmod, context_id);
    ASSERT_EQ(logmod.length, 1);

    /* Set logger options */
    options.quiet = 1;
    options.color = 1;
    options.logfile = fp;
    code = logmod_logger_set_options(LOGMOD_LOGGER, options);
    ASSERT_EQ(code, LOGMOD_OK);
    ASSERT_EQ(LOGMOD_LOGGER->options.quiet, 1);
    ASSERT_EQ(LOGMOD_LOGGER->options.color, 1);
    ASSERT_EQ(LOGMOD_LOGGER->options.logfile, fp);

    /* Test set_quiet function */
    code = logmod_logger_set_quiet(LOGMOD_LOGGER, 0);
    ASSERT_EQ(code, LOGMOD_OK);
    ASSERT_EQ(LOGMOD_LOGGER->options.quiet, 0);

    /* Test set_logfile function */
    fp = tmpfile();
    code = logmod_logger_set_logfile(LOGMOD_LOGGER, fp);
    ASSERT_EQ(code, LOGMOD_OK);
    ASSERT_EQ(LOGMOD_LOGGER->options.logfile, fp);

    /* Test set_lock function */
    code = logmod_logger_set_lock(LOGMOD_LOGGER, test_lock_function);
    ASSERT_EQ(code, LOGMOD_OK);
    ASSERT_EQ(LOGMOD_LOGGER->lock, test_lock_function);

    PASS();
}

TEST
should_log_message(void)
{
    static const char *const application_id = "APPLICATION_A";
    static const char *const context_id = "MODULE_A";
    struct logmod_logger *LOGMOD_LOGGER, table[TABLE_SIZE];
    struct logmod logmod;

    /* Initialize logger */
    logmod_init(&logmod, application_id, table, sizeof(table) / sizeof *table);
    LOGMOD_LOGGER = logmod_logger_get(&logmod, context_id);

    /* Test logging without a file - should not fail */
    lock_was_called = 0;
    logmod_logger_set_lock(LOGMOD_LOGGER, test_lock_function);
    logmod_logger_set_quiet(LOGMOD_LOGGER, 1);

    /* Test that different log levels can be used */
    LOGMOD_DEBUG((LOGMOD_LOGGER, "Debug message"));
    ASSERT_EQ(lock_was_called, 0); /* Lock should be released after logging */
    ASSERT_EQ(LOGMOD_LOGGER->level, LOGMOD_LEVEL_DEBUG);

    LOGMOD_INFO((LOGMOD_LOGGER, "Info message"));
    ASSERT_EQ(LOGMOD_LOGGER->level, LOGMOD_LEVEL_INFO);

    LOGMOD_WARN((LOGMOD_LOGGER, "Warning message"));
    ASSERT_EQ(LOGMOD_LOGGER->level, LOGMOD_LEVEL_WARN);

    LOGMOD_ERROR((LOGMOD_LOGGER, "Error message"));
    ASSERT_EQ(LOGMOD_LOGGER->level, LOGMOD_LEVEL_ERROR);

    PASS();
}

TEST
should_log_to_file(void)
{
    static const char *const application_id = "APPLICATION_A";
    static const char *const context_id = "MODULE_A";
    struct logmod_logger *LOGMOD_LOGGER, table[TABLE_SIZE];
    struct logmod logmod;
    FILE *fp = tmpfile();
    char buffer[256];
    size_t bytes_read;

    /* Initialize logger */
    logmod_init(&logmod, application_id, table, sizeof(table) / sizeof *table);
    LOGMOD_LOGGER = logmod_logger_get(&logmod, context_id);

    /* Configure for logging to file */
    logmod_logger_set_logfile(LOGMOD_LOGGER, fp);
    logmod_logger_set_quiet(LOGMOD_LOGGER, 1);

    /* Log a test message */
    LOGMOD_INFO((LOGMOD_LOGGER, "Test log message to file"));

    rewind(fp);
    bytes_read = fread(buffer, 1, sizeof(buffer) - 1, fp);
    buffer[bytes_read] = '\0';

    /* Verify log content contains our message */
    ASSERT_NEQ(strstr(buffer, "Test log message to file"), NULL);

    PASS();
}

TEST
should_format_log_message(void)
{
    static const char *const application_id = "APPLICATION_A";
    static const char *const context_id = "MODULE_A";
    static const char *const test_string = "formatted";
    static const int test_value = 42;
    struct logmod_logger *LOGMOD_LOGGER, table[TABLE_SIZE];
    struct logmod logmod;
    char buffer[256];
    size_t bytes_read;
    FILE *fp = tmpfile();

    /* Initialize logger */
    logmod_init(&logmod, application_id, table, sizeof(table) / sizeof *table);
    LOGMOD_LOGGER = logmod_logger_get(&logmod, context_id);

    /* Configure for logging to file */
    logmod_logger_set_logfile(LOGMOD_LOGGER, fp);
    logmod_logger_set_quiet(LOGMOD_LOGGER, 1);

    /* Log formatted messages */
    LOGMOD_INFO((LOGMOD_LOGGER, "Integer value: %d", test_value));
    LOGMOD_INFO((LOGMOD_LOGGER, "String value: %s", test_string));
    LOGMOD_INFO(
        (LOGMOD_LOGGER, "Combined values: %s = %d", test_string, test_value));

    rewind(fp);
    bytes_read = fread(buffer, 1, sizeof(buffer) - 1, fp);
    buffer[bytes_read] = '\0';

    /* Verify log content contains our formatted messages */
    ASSERT_NEQ(strstr(buffer, "Integer value: 42"), NULL);
    ASSERT_NEQ(strstr(buffer, "String value: formatted"), NULL);
    ASSERT_NEQ(strstr(buffer, "Combined values: formatted = 42"), NULL);

    PASS();
}

TEST
should_cleanup_logmod(void)
{
    static const char *const application_id = "APPLICATION_A";
    struct logmod_logger table[TABLE_SIZE];
    struct logmod logmod;
    logmod_err code;
    unsigned i;

    /* Initialize and then clean up */
    logmod_init(&logmod, application_id, table, sizeof(table) / sizeof *table);
    code = logmod_cleanup(&logmod);

    /* Verify cleanup success */
    ASSERT_EQ(code, LOGMOD_OK);
    ASSERT_EQ(logmod.application_id, NULL);
    ASSERT_EQ(logmod.loggers, NULL);
    ASSERT_EQ(logmod.length, 0);
    ASSERT_EQ(logmod.real_length, 0);

    /* Verify logger table is zeroed */
    for (i = 0; i < TABLE_SIZE; i++) {
        ASSERT_EQ(table[i].context_id, NULL);
    }

    PASS();
}

#undef TABLE_SIZE

SUITE(initialization)
{
    RUN_TEST(should_initialize_application);
    RUN_TEST(should_initialize_context);
}

SUITE(logger_options)
{
    RUN_TEST(should_set_logger_options);
}

SUITE(logging)
{
    RUN_TEST(should_log_message);
    RUN_TEST(should_log_to_file);
    RUN_TEST(should_format_log_message);
}

SUITE(cleanup)
{
    RUN_TEST(should_cleanup_logmod);
}

GREATEST_MAIN_DEFS();

int
main(int argc, char *argv[])
{
    GREATEST_MAIN_BEGIN();

    RUN_SUITE(initialization);
    RUN_SUITE(logger_options);
    RUN_SUITE(logging);
    RUN_SUITE(cleanup);

    GREATEST_MAIN_END();
}
