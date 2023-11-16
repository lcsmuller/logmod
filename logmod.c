#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include "logmod.h"

#define __BLANK
struct _logmod {
    __LOGMOD_METADATA(__BLANK);
};

struct _logmod_logger {
    __LOGMOD_LOGGER_METADATA(__BLANK);
};

#undef __LOGMOD_BLANK

logmod_err
logmod_init(struct logmod *logmod,
            const char *const application_id,
            struct logmod_logger table[],
            unsigned length)
{
    struct _logmod *_logmod = (struct _logmod *)logmod;
    if (table == NULL) {
        fputs("Error: Missing table at logmod_init()", stderr);
        return LOGMOD_BAD_PARAMETER;
    }
    memset(_logmod, 0, sizeof *_logmod);
    if (application_id) logmod->application_id = application_id;
    _logmod->loggers = table;
    _logmod->real_length = length;
    return LOGMOD_OK;
}

logmod_err
logmod_cleanup(struct logmod *logmod)
{
    struct _logmod *_logmod = (struct _logmod *)logmod;
    memset(_logmod->loggers, 0,
           _logmod->real_length * sizeof(struct logmod_logger));
    memset(_logmod, 0, sizeof *_logmod);
    return LOGMOD_OK;
}

static void
_logmod_lock_noop(int _)
{
}

struct logmod_logger *
logmod_logger_get(struct logmod *logmod, const char *const context_id)
{
    struct _logmod *_logmod = (struct _logmod *)logmod;
    unsigned i;

    for (i = 0; i < _logmod->length; ++i) {
        if (strcmp(_logmod->loggers[i].context_id, context_id) != 0) continue;
        return (struct logmod_logger *)&_logmod->loggers[i];
    }

    if ((_logmod->length + 1) >= _logmod->real_length) return NULL;

    memset(&_logmod->loggers[_logmod->length], 0, sizeof *_logmod->loggers);
    _logmod->loggers[_logmod->length].context_id = context_id;
    _logmod->loggers[_logmod->length].lock =
        (const void (*)(int))_logmod_lock_noop;
    return (struct logmod_logger *)&logmod->loggers[_logmod->length++];
}

logmod_err
_logmod_log(struct logmod_logger *logger, const char *fmt, ...)
{
    struct _logmod_logger *_logger = (struct _logmod_logger *)logger;
    const char *color, *type;
    struct tm *time_info;
    time_t time_raw;
    va_list args;

    logmod_err code = LOGMOD_OK;

    va_start(args, fmt);

    time(&time_raw);
    time_info = localtime(&time_raw);

    switch (_logger->level) {
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
        break;
    case LOGMOD_LEVEL_FATAL:
        color = "[35m";
        type = "FATAL";
        break;
    }

    _logger->lock(1);
    fprintf(stderr,
            "%02d:%02d:%02d \x1b%s%s\x1b[0m %s:%d: ", time_info->tm_hour,
            time_info->tm_min, time_info->tm_sec, color, type,
            _logger->filename, _logger->line);
    vfprintf(stderr, fmt, args), putc('\n', stderr);

    if (_logger->options.logfile && *_logger->options.logfile) {
        FILE *file = fopen(_logger->options.logfile, "a+");
        if (!file) {
            code = LOGMOD_MISSING_FILE;
            goto _unlock;
        }

        fprintf(file, "%02d:%02d:%02d %s %s:%d: ", time_info->tm_hour,
                time_info->tm_min, time_info->tm_sec, type, _logger->filename,
                _logger->line);
        vfprintf(file, fmt, args), putc('\n', file);
        fclose(file);
    }
_unlock:
    _logger->lock(0);
    va_end(args);
    return code;
}
