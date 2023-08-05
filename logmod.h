#ifndef LOGMOD_H
#define LOGMOD_H

#ifndef LOGMOD_LOGGER
#define LOGMOD_LOGGER __logmod_logger
#endif

enum logmod_levels {
    LOGMOD_LEVEL_TRACE = 0,
    LOGMOD_LEVEL_DEBUG,
    LOGMOD_LEVEL_INFO,
    LOGMOD_LEVEL_WARN,
    LOGMOD_LEVEL_ERROR,
    LOGMOD_LEVEL_FATAL
};

#define __LOGMOD_METADATA                                                     \
    char *filename;                                                           \
    int line;                                                                 \
    int level

struct logmod_logger {
    __LOGMOD_METADATA;
};

#define LOGMOD_TRACE(log) _LOGMOD_LOG(LOGMOD_LEVEL_TRACE, log)
#define LOGMOD_DEBUG(log) _LOGMOD_LOG(LOGMOD_LEVEL_DEBUG, log)
#define LOGMOD_INFO(log)  _LOGMOD_LOG(LOGMOD_LEVEL_INFO, log)
#define LOGMOD_WARN(log)  _LOGMOD_LOG(LOGMOD_LEVEL_WARN, log)
#define LOGMOD_ERROR(log) _LOGMOD_LOG(LOGMOD_LEVEL_ERROR, log)
#define LOGMOD_FATAL(log) _LOGMOD_LOG(LOGMOD_LEVEL_FATAL, log)
#define _LOGMOD_LOG(_level, _func_args)                                       \
    do {                                                                      \
        (LOGMOD_LOGGER)->line = __LINE__ - 1;                                 \
        (LOGMOD_LOGGER)->filename = __FILE__;                                 \
        (LOGMOD_LOGGER)->level = _level;                                      \
        (_logmod_log _func_args);                                             \
    } while (0)

struct logmod *logmod_init(const char *const application_id);

void logmod_cleanup(struct logmod *logmod);

struct logmod_logger *logmod_logger_get(struct logmod *logmod,
                                        const char *const context_id);

void _logmod_log(struct logmod_logger *logger, const char *fmt, ...);

#endif /* LOGMOD_H */
