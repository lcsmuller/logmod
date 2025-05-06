#define _POSIX_SOURCE
#include "../logmod.h"
#include "greatest.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define TABLE_LENGTH 5

enum { LOGMOD_LEVEL_HTTP = LOGMOD_LEVEL_CUSTOM, LOGMOD_LEVEL_TESTMODE };

static const struct logmod_label custom_labels[] = {
    /*[LOGMOD_LEVEL_HTTP]:*/
    { "HTTP", LOGMOD_LABEL_COLOR(BLUE, REGULAR, FOREGROUND), 0 },
    /*[LOGMOD_LEVEL_TEST]:*/
    { "TEST", LOGMOD_LABEL_COLOR(MAGENTA, REGULAR, INTENSITY), 0 }
};

static int callback_was_called = 0;
static const char *last_message = NULL;
static const char *last_label = NULL;

static logmod_err
test_callback(const struct logmod_logger *logger,
              const struct logmod_entry_info *info,
              const char *fmt,
              va_list args)
{
    callback_was_called = 1;
    last_label = info->label->name;
    last_message = fmt;
    if (logger->options.logfile) {
        vfprintf(logger->options.logfile, fmt, args);
    }
    return LOGMOD_OK;
}

TEST
should_initialize_application(void)
{
    static const char *const application_id = "APPLICATION_A";
    struct logmod_logger table[TABLE_LENGTH];
    struct logmod logmod;

    logmod_err code = logmod_init(&logmod, application_id, table,
                                  sizeof(table) / sizeof *table);
    ASSERT_EQ(LOGMOD_OK, code);
    ASSERT_EQ(application_id, logmod.application_id);
    ASSERT_EQ(table, logmod.loggers);
    ASSERT_EQ(0, logmod.length);
    ASSERT_EQ(TABLE_LENGTH, logmod.real_length);

    PASS();
}

TEST
should_initialize_context(void)
{
    static const char *const application_id = "APPLICATION_A";
    static const char *const context_id = "MODULE_A";
    struct logmod_logger table[TABLE_LENGTH], *logger;
    struct logmod logmod;

    logmod_init(&logmod, application_id, table, sizeof(table) / sizeof *table);
    logger = logmod_get_logger(&logmod, context_id);
    ASSERT_EQ((void *)&table[0], logger);
    ASSERT_EQ(1, logmod.length);

    PASS();
}

/* Custom lock function for testing */
static int lock_was_called = 0;
static void
test_lock_function(const struct logmod_logger *logger, int should_lock)
{
    (void)logger;
    lock_was_called = should_lock ? 1 : 0;
}

TEST
should_set_logger_options(void)
{
    static const char *const application_id = "APPLICATION_A";
    static const char *const context_id = "MODULE_A";
    struct logmod_logger table[TABLE_LENGTH], *logger;
    struct logmod logmod;
    struct logmod_options options = { 0 };
    FILE *fp = tmpfile();
    logmod_err code;

    logmod_init(&logmod, application_id, table, sizeof(table) / sizeof *table);
    logger = logmod_get_logger(&logmod, context_id);
    ASSERT_EQ(1, logmod.length);

    options.quiet = 1;
    options.color = 1;
    options.logfile = fp;
    code = logmod_logger_set_options(logger, options);
    ASSERT_EQ(LOGMOD_OK, code);
    ASSERT_EQ(1, logger->options.quiet);
    ASSERT_EQ(1, logger->options.color);
    ASSERT_EQ(fp, logger->options.logfile);

    code = logmod_logger_set_quiet(logger, 0);
    ASSERT_EQ(LOGMOD_OK, code);
    ASSERT_EQ(0, logger->options.quiet);

    code = logmod_logger_set_color(logger, 0);
    ASSERT_EQ(LOGMOD_OK, code);
    ASSERT_EQ(0, logger->options.color);

    fp = tmpfile();
    code = logmod_logger_set_logfile(logger, fp);
    ASSERT_EQ(LOGMOD_OK, code);
    ASSERT_EQ(fp, logger->options.logfile);

    code = logmod_set_lock(&logmod, test_lock_function);
    ASSERT_EQ(LOGMOD_OK, code);
    ASSERT_EQ(test_lock_function, logmod.lock);

    PASS();
}

TEST
should_log_message(void)
{
    static const char *const application_id = "APPLICATION_A";
    static const char *const context_id = "MODULE_A";
    struct logmod_logger table[TABLE_LENGTH], *logger;
    struct logmod logmod;

    logmod_init(&logmod, application_id, table, sizeof(table) / sizeof *table);
    logger = logmod_get_logger(&logmod, context_id);

    /* Test logging without a file - should not fail */
    lock_was_called = 0;
    logmod_set_lock(&logmod, test_lock_function);
    logmod_logger_set_callback(logger, custom_labels,
                               sizeof(custom_labels) / sizeof *custom_labels,
                               test_callback);
    logmod_logger_set_quiet(logger, 1);

    /* Test that different log levels can be used */
    logmod_nlog(DEBUG, logger, ("Debug message"), 0);
    ASSERT_EQ(0, lock_was_called); /* Lock should be released after logging */
    ASSERT_STR_EQ("DEBUG", last_label);

    logmod_nlog(INFO, logger, ("Info message"), 0);
    ASSERT_STR_EQ("INFO", last_label);

    logmod_nlog(WARN, logger, ("Warning message"), 0);
    ASSERT_STR_EQ("WARN", last_label);

    logmod_nlog(ERROR, logger, ("Error message"), 0);
    ASSERT_STR_EQ("ERROR", last_label);

    PASS();
}

TEST
should_log_to_file(void)
{
    static const char *const application_id = "APPLICATION_A";
    static const char *const context_id = "MODULE_A";
    struct logmod_logger table[TABLE_LENGTH], *logger;
    struct logmod logmod;
    FILE *fp = tmpfile();
    char buffer[256];
    size_t bytes_read;

    logmod_init(&logmod, application_id, table, sizeof(table) / sizeof *table);
    logger = logmod_get_logger(&logmod, context_id);

    logmod_logger_set_logfile(logger, fp);
    logmod_logger_set_quiet(logger, 1);

    logmod_nlog(INFO, logger, ("Test log message to file"), 0);

    rewind(fp);
    bytes_read = fread(buffer, 1, sizeof(buffer) - 1, fp);
    buffer[bytes_read] = '\0';

    ASSERT_NEQ(NULL, strstr(buffer, "Test log message to file"));

    PASS();
}

TEST
should_format_log_message(void)
{
    static const char *const application_id = "APPLICATION_A";
    static const char *const context_id = "MODULE_A";
    static const char *const test_string = "formatted";
    static const int test_value = 42;
    struct logmod_logger table[TABLE_LENGTH], *logger;
    struct logmod logmod;
    char buffer[256];
    size_t bytes_read;
    FILE *fp = tmpfile();

    logmod_init(&logmod, application_id, table, sizeof(table) / sizeof *table);
    logger = logmod_get_logger(&logmod, context_id);

    logmod_logger_set_logfile(logger, fp);
    logmod_logger_set_quiet(logger, 1);

    logmod_nlog(INFO, logger, ("Integer value: %d", test_value), 1);
    logmod_nlog(INFO, logger, ("String value: %s", test_string), 1);
    logmod_nlog(INFO, logger,
                ("Combined values: %s = %d", test_string, test_value), 2);

    rewind(fp);
    bytes_read = fread(buffer, 1, sizeof(buffer) - 1, fp);
    buffer[bytes_read] = '\0';

    ASSERT_NEQ(NULL, strstr(buffer, "Integer value: 42"));
    ASSERT_NEQ(NULL, strstr(buffer, "String value: formatted"));
    ASSERT_NEQ(NULL, strstr(buffer, "Combined values: formatted = 42"));

    PASS();
}

TEST
should_cleanup_logmod(void)
{
    static const char *const application_id = "APPLICATION_A";
    struct logmod_logger table[TABLE_LENGTH];
    struct logmod logmod;
    logmod_err code;
    unsigned i;

    logmod_init(&logmod, application_id, table, sizeof(table) / sizeof *table);
    code = logmod_cleanup(&logmod);

    ASSERT_EQ(LOGMOD_OK, code);
    ASSERT_EQ(NULL, logmod.application_id);
    ASSERT_EQ(NULL, logmod.loggers);
    ASSERT_EQ(0, logmod.length);
    ASSERT_EQ(0, logmod.real_length);

    for (i = 0; i < TABLE_LENGTH; i++) {
        ASSERT_EQ(NULL, table[i].context_id);
    }

    PASS();
}

TEST
should_log_with_custom_levels(void)
{
    static const char *const application_id = "APPLICATION_A";
    static const char *const context_id = "MODULE_A";
    struct logmod_logger table[TABLE_LENGTH], *logger;
    struct logmod logmod;
    FILE *fp = tmpfile();
    char buffer[256];
    size_t bytes_read;

    logmod_init(&logmod, application_id, table, sizeof(table) / sizeof *table);
    logger = logmod_get_logger(&logmod, context_id);

    logmod_logger_set_logfile(logger, fp);
    logmod_logger_set_quiet(logger, 1);
    logmod_logger_set_callback(logger, custom_labels,
                               sizeof(custom_labels) / sizeof *custom_labels,
                               test_callback);

    callback_was_called = 0;
    last_label = NULL;
    last_message = NULL;

    logmod_nlog(HTTP, logger,
                ("[%s] HTTP request received",
                 logmod_logger_get_label(logger, LOGMOD_LEVEL_HTTP)->name),
                1);
    ASSERT_EQ(1, callback_was_called);
    ASSERT_STR_EQ("HTTP", last_label);
    ASSERT_STR_EQ("[%s] HTTP request received", last_message);

    rewind(fp);
    bytes_read = fread(buffer, 1, sizeof(buffer) - 1, fp);
    buffer[bytes_read] = '\0';
    ASSERT_NEQ(NULL, strstr(buffer, "HTTP"));
    ASSERT_NEQ(NULL, strstr(buffer, "HTTP request received"));

    callback_was_called = 0;
    last_label = NULL;
    last_message = NULL;

    logmod_nlog(TESTMODE, logger,
                ("[%s] Test mode activated",
                 logmod_logger_get_label(logger, LOGMOD_LEVEL_TESTMODE)->name),
                1);
    ASSERT_EQ(1, callback_was_called);
    ASSERT_STR_EQ("TEST", last_label);
    ASSERT_STR_EQ("[%s] Test mode activated", last_message);

    rewind(fp);
    bytes_read = fread(buffer, 1, sizeof(buffer) - 1, fp);
    buffer[bytes_read] = '\0';
    ASSERT_NEQ(NULL, strstr(buffer, "TEST"));
    ASSERT_NEQ(NULL, strstr(buffer, "Test mode activated"));

    PASS();
}

TEST
should_get_correct_level_labels(void)
{
    static const char *const application_id = "APPLICATION_A";
    static const char *const context_id = "MODULE_A";
    struct logmod_logger table[TABLE_LENGTH], *logger;
    struct logmod logmod;
    const struct logmod_label *label;

    logmod_init(&logmod, application_id, table, sizeof(table) / sizeof *table);
    logger = logmod_get_logger(&logmod, context_id);

    label = logmod_logger_get_label(logger, LOGMOD_LEVEL_DEBUG);
    ASSERT_STR_EQ("DEBUG", label->name);

    label = logmod_logger_get_label(logger, LOGMOD_LEVEL_INFO);
    ASSERT_STR_EQ("INFO", label->name);

    label = logmod_logger_get_label(logger, LOGMOD_LEVEL_ERROR);
    ASSERT_STR_EQ("ERROR", label->name);

    logmod_logger_set_callback(logger, custom_labels,
                               sizeof(custom_labels) / sizeof *custom_labels,
                               test_callback);

    label = logmod_logger_get_label(logger, LOGMOD_LEVEL_HTTP);
    ASSERT_STR_EQ("HTTP", label->name);

    label = logmod_logger_get_label(logger, LOGMOD_LEVEL_TESTMODE);
    ASSERT_STR_EQ("TEST", label->name);

    PASS();
}

TEST
should_get_level_by_label_name(void)
{
    static const char *const application_id = "APPLICATION_A";
    static const char *const context_id = "MODULE_A";
    struct logmod_logger table[TABLE_LENGTH], *logger;
    struct logmod logmod;
    long level;

    logmod_init(&logmod, application_id, table, sizeof(table) / sizeof *table);
    logger = logmod_get_logger(&logmod, context_id);

    level = logmod_logger_get_level(logger, "DEBUG");
    ASSERT_EQ(LOGMOD_LEVEL_DEBUG, level);

    level = logmod_logger_get_level(logger, "INFO");
    ASSERT_EQ(LOGMOD_LEVEL_INFO, level);

    level = logmod_logger_get_level(logger, "ERROR");
    ASSERT_EQ(LOGMOD_LEVEL_ERROR, level);

    logmod_logger_set_callback(logger, custom_labels,
                               sizeof(custom_labels) / sizeof *custom_labels,
                               test_callback);

    level = logmod_logger_get_level(logger, "HTTP");
    ASSERT_EQ(LOGMOD_LEVEL_HTTP, level);

    level = logmod_logger_get_level(logger, "TEST");
    ASSERT_EQ(LOGMOD_LEVEL_TESTMODE, level);

    level = logmod_logger_get_level(logger, "NONEXISTENT");
    ASSERT_EQ(-1, level);

    freopen("/dev/null", "w", stderr); /* disable stderr */
    level = logmod_logger_get_level(NULL, "INFO");
    ASSERT_EQ(-2, level);

    level = logmod_logger_get_level(logger, NULL);
    ASSERT_EQ(-2, level);
    freopen("/dev/tty", "w", stderr); /* restore stderr */

    PASS();
}

#define TEST_STRING "test string"
TEST
should_encode_ansi_string(void)
{
    static const char *const application_id = "APPLICATION_A";
    struct logmod_logger table[TABLE_LENGTH], *logger;
    struct logmod logmod;
    const char *result;

    logmod_init(&logmod, application_id, table, sizeof(table) / sizeof *table);
    logger = logmod_get_logger(&logmod, "MODULE_A");
    logmod_logger_set_color(logger, 1);

    result = LML(logger, TEST_STRING, RED, BOLD, FOREGROUND);
    ASSERT_STR_EQm(result, "\x1b[1;31mtest string\x1b[0m", result);

    PASS();
}

TEST
should_return_original_string_when_color_disabled(void)
{
    static const char *const application_id = "APPLICATION_A";
    struct logmod_logger table[TABLE_LENGTH], *logger;
    struct logmod logmod;
    const char *result;

    logmod_init(&logmod, application_id, table, sizeof(table) / sizeof *table);
    logger = logmod_get_logger(&logmod, "MODULE_A");
    logmod_logger_set_color(logger, 0);

    result = LML(logger, TEST_STRING, RED, BOLD, FOREGROUND);
    ASSERT_MEM_EQm(result, TEST_STRING, result, sizeof(TEST_STRING) - 1);

    PASS();
}

TEST
should_handle_different_ansi_visibilities(void)
{
    static const char *const application_id = "APPLICATION_A";
    struct logmod_logger table[TABLE_LENGTH], *logger;
    struct logmod logmod;
    const char *result;

    logmod_init(&logmod, application_id, table, sizeof(table) / sizeof *table);
    logger = logmod_get_logger(&logmod, "MODULE_A");
    logmod_logger_set_color(logger, 1);

    result = LML(logger, TEST_STRING, GREEN, REGULAR, FOREGROUND);
    ASSERT_NEQ(NULL, strstr(result, "\x1b[0;32m"));

    result = LML(logger, TEST_STRING, BLUE, REGULAR, BACKGROUND);
    ASSERT_NEQ(NULL, strstr(result, "\x1b[0;44m"));

    result = LML(logger, TEST_STRING, RED, REGULAR, INTENSITY);
    ASSERT_NEQ(NULL, strstr(result, "\x1b[0;91m"));

    PASS();
}

TEST
should_handle_different_ansi_styles(void)
{
    static const char *const application_id = "APPLICATION_A";
    struct logmod_logger table[TABLE_LENGTH], *logger;
    struct logmod logmod;
    const char *result;

    logmod_init(&logmod, application_id, table, sizeof(table) / sizeof *table);
    logger = logmod_get_logger(&logmod, "MODULE_A");
    logmod_logger_set_color(logger, 1);

    result = LML(logger, TEST_STRING, CYAN, REGULAR, FOREGROUND);
    ASSERT_NEQ(NULL, result);
    ASSERT_NEQ(NULL, strstr(result, "\x1b[0;36m"));

    result = LML(logger, TEST_STRING, CYAN, BOLD, FOREGROUND);
    ASSERT_NEQ(NULL, result);
    ASSERT_NEQ(NULL, strstr(result, "\x1b[1;36m"));

    result = LML(logger, TEST_STRING, CYAN, UNDERLINE, FOREGROUND);
    ASSERT_NEQ(NULL, result);
    ASSERT_NEQ(NULL, strstr(result, "\x1b[4;36m"));

    PASS();
}
#undef TEST_STRING

TEST
should_use_fallback_logger(void)
{
    const char *const test_message = "Test message using fallback logger";
    FILE *fp = tmpfile();
    int fd = fileno(fp);
    char buffer[256];
    size_t bytes_read;
    int original_stdout = dup(STDOUT_FILENO);

    dup2(fd, STDOUT_FILENO);
    logmod_nlog(INFO, NULL, (test_message), 0);
    fflush(stdout);
    dup2(original_stdout, STDOUT_FILENO);
    close(original_stdout);

    rewind(fp);
    bytes_read = fread(buffer, 1, sizeof(buffer) - 1, fp);
    buffer[bytes_read] = '\0';

    ASSERT_GT(bytes_read, 0);
    ASSERT_NEQ(NULL, strstr(buffer, test_message));

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

SUITE(cleanup)
{
    RUN_TEST(should_cleanup_logmod);
}

SUITE(logging)
{
    RUN_TEST(should_log_message);
    RUN_TEST(should_log_to_file);
    RUN_TEST(should_format_log_message);
    RUN_TEST(should_log_with_custom_levels);
    RUN_TEST(should_get_correct_level_labels);
    RUN_TEST(should_get_level_by_label_name);
    RUN_TEST(should_use_fallback_logger);
}

SUITE(ansi)
{
    RUN_TEST(should_encode_ansi_string);
    RUN_TEST(should_return_original_string_when_color_disabled);
    RUN_TEST(should_handle_different_ansi_visibilities);
    RUN_TEST(should_handle_different_ansi_styles);
}

GREATEST_MAIN_DEFS();

int
main(int argc, char *argv[])
{
    GREATEST_MAIN_BEGIN();

    RUN_SUITE(initialization);
    RUN_SUITE(logger_options);
    RUN_SUITE(cleanup);
    RUN_SUITE(logging);
    RUN_SUITE(ansi);

    GREATEST_MAIN_END();
}
