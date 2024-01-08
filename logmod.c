#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include "logmod.h"

logmod_err
logmod_init(struct logmod *logmod,
            const char *const application_id,
            struct logmod_logger table[],
            unsigned length)
{
    if (table == NULL) {
        fputs("Error: Missing table at logmod_init()", stderr);
        return LOGMOD_BAD_PARAMETER;
    }

    memset(logmod, 0, sizeof *logmod);

    if (application_id) logmod->application_id = application_id;
    logmod->loggers = table;
    logmod->real_length = length;

    return LOGMOD_OK;
}

logmod_err
logmod_cleanup(struct logmod *logmod)
{
    memset(logmod->loggers, 0,
           logmod->real_length * sizeof(struct logmod_logger));
    memset(logmod, 0, sizeof *logmod);

    return LOGMOD_OK;
}

static void
_logmod_lock_noop(int _)
{
}

logmod_err
logmod_logger_set_lock(struct logmod_logger *logger,
                       void (*lock)(int should_lock))
{
    if (logger == NULL) {
        fputs("Error: Missing logger at logmod_logger_set_lock()", stderr);
        return LOGMOD_BAD_PARAMETER;
    }

    if (lock == NULL) {
        fputs("Error: Missing lock at logmod_logger_set_lock()", stderr);
        return LOGMOD_BAD_PARAMETER;
    }

    logger->lock = lock;

    return LOGMOD_OK;
}

logmod_err
logmod_logger_set_options(struct logmod_logger *logger,
                          struct logmod_logger_options options)
{
    if (logger == NULL) {
        fputs("Error: Missing logger at logmod_logger_set_options()", stderr);
        return LOGMOD_BAD_PARAMETER;
    }

    logger->options = options;

    return LOGMOD_OK;
}

logmod_err
logmod_logger_set_quiet(struct logmod_logger *logger, int quiet)
{
    if (logger == NULL) {
        fputs("Error: Missing logger at logmod_logger_set_quiet()", stderr);
        return LOGMOD_BAD_PARAMETER;
    }

    logger->options.quiet = quiet;

    return LOGMOD_OK;
}

logmod_err
logmod_logger_set_logfile(struct logmod_logger *logger, char *logfile)
{
    if (logger == NULL) {
        fputs("Error: Missing logger at logmod_logger_set_logfile()", stderr);
        return LOGMOD_BAD_PARAMETER;
    }

    logger->options.logfile = logfile;

    return LOGMOD_OK;
}

struct logmod_logger *
logmod_logger_get(struct logmod *logmod, const char *const context_id)
{
    unsigned i;

    for (i = 0; i < logmod->length; ++i) {
        if (strcmp(logmod->loggers[i].context_id, context_id) != 0) continue;
        return (struct logmod_logger *)&logmod->loggers[i];
    }

    if ((logmod->length + 1) >= logmod->real_length) return NULL;

    memset(&logmod->loggers[logmod->length], 0, sizeof *logmod->loggers);

    logmod->loggers[logmod->length].context_id = context_id;
    logmod->loggers[logmod->length].lock =
        (void (*)(int))_logmod_lock_noop;

    return (struct logmod_logger *)&logmod->loggers[logmod->length++];
}

void
_logmod_print(struct logmod_logger *logger, struct tm *time_info,
              const char *color, const char *type, FILE *output, const char *fmt, va_list args)
{
    fprintf(output,
            "%02d:%02d:%02d \x1b%s%s\x1b[0m %s:%d: ", time_info->tm_hour,
            time_info->tm_min, time_info->tm_sec, color, type,
            logger->filename, logger->line);
    vfprintf(output, fmt, args), putc('\n', output);
}

logmod_err
_logmod_log(struct logmod_logger *logger, const char *fmt, ...)
{
    const char *color, *type;
    struct tm *time_info;
    time_t time_raw;
    va_list args;
    FILE *output = stdout;
    FILE *logfile;

    logmod_err code = LOGMOD_OK;

    va_start(args, fmt);

    time(&time_raw);
    time_info = localtime(&time_raw);

    switch (logger->level) {
    case LOGMOD_LEVEL_TRACE:
        color = "[94m";
        type = "TRACE";
        break;
    case LOGMOD_LEVEL_DEBUG:
        color = "[36m";
        type = "DEBUG";
        break;
    case LOGMOD_LEVEL_INFO:
        color = "[32m";
        type = "INFO";
        break;
    case LOGMOD_LEVEL_WARN:
        color = "[33m";
        type = "WARN";
        break;
    case LOGMOD_LEVEL_ERROR:
        color = "[31m";
        type = "ERROR";
        output = stderr;
        break;
    case LOGMOD_LEVEL_FATAL:
        color = "[35m";
        type = "FATAL";
        output = stderr;
        break;
    }

    logger->lock(1);
    _logmod_print(logger, time_info, color, type, output, fmt, args);

    if (logger->options.logfile && *logger->options.logfile) {
        logfile = fopen(logger->options.logfile, "a+");
        if (!logfile) {
            code = LOGMOD_MISSING_FILE;
            goto _unlock;
        }

        _logmod_print(logger, time_info, color, type, logfile, fmt, args);
        fclose(logfile);
    }

_unlock:
    logger->lock(0);
    va_end(args);
    return code;
}
