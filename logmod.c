#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include "logmod.h"

typedef void (*logmod_logger_lock)(int should_lock);

struct logmod_logger_options {
    char *logfile;
    char *buffer;
    int quiet;
};

struct _logmod_logger {
    __LOGMOD_METADATA;
    const char *context_id;
    logmod_logger_lock lock;
    struct logmod_logger_options options;
};

struct logmod {
    const char *application_id;
    struct _logmod_logger *loggers;
    size_t size;
};

struct logmod *
logmod_init(const char *const application_id)
{
    struct logmod *new_logmod = calloc(1, sizeof *new_logmod);
    if (application_id) new_logmod->application_id = application_id;
    return new_logmod;
}

void
logmod_cleanup(struct logmod *logmod)
{
    if (logmod->loggers) free(logmod->loggers);
    free(logmod);
}

static void
_logmod_lock_noop(int _)
{
}

struct logmod_logger *
logmod_logger_get(struct logmod *logmod, const char *const context_id)
{
    void *tmp;
    size_t i;

    for (i = 0; i < logmod->size; ++i) {
        if (strcmp(logmod->loggers[i].context_id, context_id) != 0) continue;
        return (struct logmod_logger *)&logmod->loggers[i];
    }

    tmp =
        realloc(logmod->loggers, sizeof *logmod->loggers * (logmod->size + 1));
    if (!tmp) return NULL;

    logmod->loggers = tmp;
    memset(&logmod->loggers[logmod->size], 0, sizeof *logmod->loggers);
    logmod->loggers[logmod->size].context_id = context_id;
    logmod->loggers[logmod->size].lock = _logmod_lock_noop;
    return (struct logmod_logger *)&logmod->loggers[logmod->size++];
}

void
_logmod_log(struct logmod_logger *_logger, const char *fmt, ...)
{
    struct _logmod_logger *logger = (struct _logmod_logger *)_logger;
    const char *color, *type;
    struct tm *timeInfo;
    time_t timeRaw;
    va_list args;

    va_start(args, fmt);

    time(&timeRaw);
    timeInfo = localtime(&timeRaw);

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
        break;
    case LOGMOD_LEVEL_FATAL:
        color = "[35m";
        type = "FATAL";
        break;
    }

    logger->lock(1);
    fprintf(stderr,
            "%02d:%02d:%02d \x1b%s%s\x1b[0m %s:%d: ", timeInfo->tm_hour,
            timeInfo->tm_min, timeInfo->tm_sec, color, type, logger->filename,
            logger->line);
    vfprintf(stderr, fmt, args), putc('\n', stderr);

    if (logger->options.logfile && *logger->options.logfile) {
        FILE *pFile = fopen(logger->options.logfile, "a+");
        if (!pFile) goto _unlock;

        fprintf(pFile, "%02d:%02d:%02d %s %s:%d: ", timeInfo->tm_hour,
                timeInfo->tm_min, timeInfo->tm_sec, type, logger->filename,
                logger->line);
        vfprintf(pFile, fmt, args), putc('\n', stderr);
        fclose(pFile);
    }
_unlock:
    logger->lock(0);
    va_end(args);
}
