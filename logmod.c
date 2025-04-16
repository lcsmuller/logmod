#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include "logmod.h"

struct logmod_level {
    const char *const color;
    const char *const type;
    int output; /* 0 = stdout, 1 = stderr */
};

static const struct logmod_level logmod_levels[__LOGMOD_LEVEL_MAX] = {
    { "\x1b[94m", "TRACE", 0 }, { "\x1b[36m", "DEBUG", 0 },
    { "\x1b[32m", "INFO", 0 },  { "\x1b[33m", "WARN", 1 },
    { "\x1b[31m", "ERROR", 1 }, { "\x1b[35m", "FATAL", 1 },
};

logmod_err
logmod_init(struct logmod *logmod,
            const char *const application_id,
            struct logmod_logger table[],
            unsigned length)
{
    unsigned i;

    if (table == NULL) {
        fputs("Error: Missing table at logmod_init()", stderr);
        return LOGMOD_BAD_PARAMETER;
    }

    memset(logmod, 0, sizeof *logmod);
    if (application_id) {
        logmod->application_id = application_id;
    }
    for (i = 0; i < length; ++i) {
        if (table[i].context_id == NULL) {
            fputs("Error: Missing context_id at logmod_init()", stderr);
            return LOGMOD_BAD_PARAMETER;
        }
    }
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
logmod_logger_set_logfile(struct logmod_logger *logger, FILE *logfile)
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
        if (0 == strcmp(logmod->loggers[i].context_id, context_id))
            return &logmod->loggers[i];
    }

    if ((logmod->length + 1) >= logmod->real_length) {
        return NULL;
    }

    memset(&logmod->loggers[logmod->length], 0, sizeof *logmod->loggers);

    logmod->loggers[logmod->length].context_id = context_id;
    logmod->loggers[logmod->length].lock = _logmod_lock_noop;

    return &logmod->loggers[logmod->length++];
}

static void
_logmod_print(const struct logmod_logger *logger,
              const struct tm *time_info,
              const struct logmod_level *const level,
              const char *fmt,
              va_list args,
              FILE *output)
{
    const char *const color_start = !logger->options.color ? "" : level->color;
    const char *const color_end = !logger->options.color ? "" : "\x1b[0m";
    fprintf(output, "%02d:%02d:%02d %s%s%s %s:%d: ", time_info->tm_hour,
            time_info->tm_min, time_info->tm_sec, color_start, level->type,
            color_end, logger->filename, logger->line);
    vfprintf(output, fmt, args);
    putc('\n', output);
}

logmod_err
_logmod_log(const struct logmod_logger *logger, const char *fmt, ...)
{
    const struct logmod_level *const level = &logmod_levels[logger->level];
    const time_t time_raw = time(NULL);
    const struct tm *time_info = localtime(&time_raw);
    logmod_err code = LOGMOD_OK;
    va_list args;

    logger->lock(1);
    if (!logger->options.quiet) {
        va_start(args, fmt);
        _logmod_print(logger, time_info, level, fmt, args,
                      level->output == 0 ? stdout : stderr);
        va_end(args);
    }
    if (logger->options.logfile) {
        va_start(args, fmt);
        _logmod_print(logger, time_info, level, fmt, args,
                      logger->options.logfile);
        va_end(args);
    }
    logger->lock(0);
    return code;
}
