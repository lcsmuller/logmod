#ifndef LOGMOD_H
#define LOGMOD_H

#include <stdio.h>

#ifndef LOGMOD_LOGGER
#define LOGMOD_LOGGER __logmod_logger
#endif

enum logmod_levels {
    LOGMOD_LEVEL_TRACE = 0,
    LOGMOD_LEVEL_DEBUG,
    LOGMOD_LEVEL_INFO,
    LOGMOD_LEVEL_WARN,
    LOGMOD_LEVEL_ERROR,
    LOGMOD_LEVEL_FATAL,
    __LOGMOD_LEVEL_MAX
};

typedef enum { LOGMOD_OK = 0, LOGMOD_BAD_PARAMETER = -1 } logmod_err;

struct logmod_logger_options {
    FILE *logfile;
    char *buffer;
    int quiet;
    int color;
};

#define __LOGMOD_LOGGER_PREDEFINED_ARGS                                       \
    const int line;                                                           \
    const char *filename;                                                     \
    const enum logmod_levels level

struct __logmod_predef {
    __LOGMOD_LOGGER_PREDEFINED_ARGS;
};

struct logmod_logger {
    __LOGMOD_LOGGER_PREDEFINED_ARGS;
    const char *context_id;
    void (*lock)(int should_lock);
    struct logmod_logger_options options;
};

#undef __LOGMOD_LOGGER_PREDEFINED_ARGS

struct logmod {
    const char *application_id;
    struct logmod_logger *loggers;
    unsigned length;
    unsigned real_length;
};

#define LOGMOD_TRACE(log) _LOGMOD_LOG(LOGMOD_LEVEL_TRACE, log)
#define LOGMOD_DEBUG(log) _LOGMOD_LOG(LOGMOD_LEVEL_DEBUG, log)
#define LOGMOD_INFO(log)  _LOGMOD_LOG(LOGMOD_LEVEL_INFO, log)
#define LOGMOD_WARN(log)  _LOGMOD_LOG(LOGMOD_LEVEL_WARN, log)
#define LOGMOD_ERROR(log) _LOGMOD_LOG(LOGMOD_LEVEL_ERROR, log)
#define LOGMOD_FATAL(log) _LOGMOD_LOG(LOGMOD_LEVEL_FATAL, log)
#define _LOGMOD_LOG(_level, _params)                                          \
    do {                                                                      \
        static const struct __logmod_predef __args = { __LINE__ - 1,          \
                                                       __FILE__, _level };    \
        memcpy((LOGMOD_LOGGER), &__args, sizeof(__args));                     \
        (_logmod_log _params);                                                \
    } while (0)

logmod_err logmod_init(struct logmod *logmod,
                       const char *const application_id,
                       struct logmod_logger table[],
                       unsigned length);

logmod_err logmod_cleanup(struct logmod *logmod);

logmod_err logmod_logger_set_lock(struct logmod_logger *logger,
                                  void (*lock)(int should_lock));

logmod_err logmod_logger_set_options(struct logmod_logger *logger,
                                     struct logmod_logger_options options);

logmod_err logmod_logger_set_quiet(struct logmod_logger *logger, int quiet);

logmod_err logmod_logger_set_logfile(struct logmod_logger *logger,
                                     FILE *logfile);

struct logmod_logger *logmod_logger_get(struct logmod *logmod,
                                        const char *const context_id);

logmod_err _logmod_log(const struct logmod_logger *logger,
                       const char *fmt,
                       ...);

#endif /* LOGMOD_H */
