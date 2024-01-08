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

typedef enum {
    LOGMOD_OK = 0,
    LOGMOD_BAD_PARAMETER = -1,
    LOGMOD_MISSING_FILE = -2
} logmod_err;

struct logmod_logger_options {
    char *logfile;
    char *buffer;
    int quiet;
};

struct logmod_logger {
    char *filename;
    int line;
    int level;
    const char *context_id;
    void (*lock)(int should_lock);
    struct logmod_logger_options options;
};

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
#define _LOGMOD_LOG(_level, _func_args)                                       \
    do {                                                                      \
        (LOGMOD_LOGGER)->line = __LINE__ - 1;                                 \
        (LOGMOD_LOGGER)->filename = __FILE__;                                 \
        (LOGMOD_LOGGER)->level = _level;                                      \
        (_logmod_log _func_args);                                             \
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

logmod_err logmod_logger_set_logfile(struct logmod_logger *logger, char *logfile);

struct logmod_logger *logmod_logger_get(struct logmod *logmod,
                                        const char *const context_id);

logmod_err _logmod_log(struct logmod_logger *logger, const char *fmt, ...);

#endif /* LOGMOD_H */
