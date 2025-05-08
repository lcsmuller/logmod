/**
 * LogMod Basic Example
 *
 * This example demonstrates the core functionality of the LogMod library:
 * - Initializing the logging context
 * - Creating multiple loggers with different contexts
 * - Using different log levels
 * - Configuring logger options (colors, file output, etc.)
 * - Using custom log levels
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "../logmod.h"

/* Define custom log levels */
enum { LOGMOD_LEVEL_NETWORK = LOGMOD_LEVEL_CUSTOM, LOGMOD_LEVEL_DATABASE };

/* Define custom log labels with their colors and styles */
static const struct logmod_label custom_labels[] = {
    { "NETWORK", LOGMOD_LABEL_COLOR(BOLD, BACKGROUND_INTENSITY, BLUE), 0 },
    { "DB", LOGMOD_LABEL_COLOR(REGULAR, BACKGROUND_INTENSITY, MAGENTA), 0 }
};

/* Simple mutex for thread safety */
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Lock function for thread safety */
static void
log_lock(const struct logmod_logger *logger, int should_lock)
{
    (void)logger; /* Unused parameter */
    if (should_lock)
        pthread_mutex_lock(&log_mutex);
    else
        pthread_mutex_unlock(&log_mutex);
}

/* Custom logging callback for additional processing */
static logmod_err
custom_callback(const struct logmod_logger *logger,
                const struct logmod_info *info,
                const char *fmt,
                va_list args)
{
    /* Access user data if needed */
    void *user_data = logger->user_data;
    (void)user_data; /* Unused in this example */

    /* For network messages, we might want special handling */
    if (info->level == LOGMOD_LEVEL_NETWORK) {
        /* Here you could implement custom network log processing */
        /* Just print a message for this example */
        printf("[Network traffic intercepted by callback]\n");
    }

    /* Continue with normal logging */
    return LOGMOD_OK_CONTINUE;
}

int
main()
{
    /* Initialize logging context */
    struct logmod logmod;
    struct logmod_logger loggers[3]; /* Space for 3 loggers */
    FILE *log_file;

    /* Open log file */
    log_file = fopen("example.log", "w");
    if (!log_file) {
        perror("Failed to open log file");
        return EXIT_FAILURE;
    }

    /* Initialize the logging context */
    if (logmod_init(&logmod, "EXAMPLE_APP", loggers, 3) != LOGMOD_OK) {
        fprintf(stderr, "Failed to initialize logging context\n");
        fclose(log_file);
        return EXIT_FAILURE;
    }
    /* Set default options for all loggers */
    logmod_set_options(&logmod, (struct logmod_options){
                                    .color = 1, .show_application_id = 1 });

    /* Set the lock function for thread safety */
    logmod_set_lock(&logmod, log_lock);

    /* Get loggers for different parts of the application */
    struct logmod_logger *main_logger = logmod_get_logger(&logmod, "MAIN");
    struct logmod_logger *network_logger =
        logmod_get_logger(&logmod, "NETWORK");
    struct logmod_logger *db_logger = logmod_get_logger(&logmod, "DATABASE");

    /* Configure the main logger */
    logmod_logger_set_logfile(main_logger, log_file);
    logmod_logger_set_level(main_logger,
                            LOGMOD_LEVEL_INFO); /* Only INFO and above */

    /* Configure the network logger with custom labels and callback */
    logmod_logger_set_logfile(network_logger, log_file);
    logmod_logger_set_callback(
        network_logger, custom_labels,
        sizeof(custom_labels) / sizeof(custom_labels[0]), custom_callback);
    /* Show timestamps and message counters */
    logmod_logger_set_time(network_logger, 1);
    logmod_logger_set_counter(network_logger, 1);

    /* Configure the database logger */
    logmod_logger_set_logfile(db_logger, log_file);
    logmod_logger_set_quiet(db_logger, 1); /* No console output for DB logs */
    logmod_logger_set_callback(
        db_logger, custom_labels,
        sizeof(custom_labels) / sizeof(custom_labels[0]), NULL);

    /* Log some messages with the main logger */
    logmod_log(INFO, main_logger, "Application started");
    logmod_log(DEBUG, main_logger,
               "This debug message won't appear due to level filter");
    logmod_log(WARN, main_logger, "System resources are at %d%% capacity", 80);

    /* Log network activity */
    logmod_log(INFO, network_logger, "Network interface initialized");
    logmod_log(NETWORK, network_logger, "Connection from %s:%d",
               "192.168.1.10", 8080);
    logmod_log(ERROR, network_logger, "Connection timeout after %d seconds",
               30);

    /* Log database operations - these only go to the file */
    logmod_log(INFO, db_logger, "Connected to database");
    logmod_log(DATABASE, db_logger, "Query executed: SELECT * FROM users");
    logmod_log(WARN, db_logger, "Slow query detected: %s",
               "UPDATE large_table SET value = 1");

    /* Toggle the network logger off and try to log something */
    logmod_toggle_logger(&logmod, "NETWORK");
    logmod_log(INFO, network_logger, "This message will be skipped");

    /* Toggle it back on */
    logmod_toggle_logger(&logmod, "NETWORK");
    logmod_log(INFO, network_logger, "Network logger re-enabled");

    /* Clean up */
    logmod_log(INFO, main_logger, "Application shutting down");
    logmod_cleanup(&logmod);
    fclose(log_file);

    printf("\nExample complete! Check example.log for the full log output.\n");

    return EXIT_SUCCESS;
}
