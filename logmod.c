#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <time.h>

#include "logmod.h"

#define __COLOR(_color, _visibility)                                          \
    LOGMOD_COLOR_##_color + LOGMOD_VISIBILITY_##_visibility
static const struct logmod_label default_labels[__LOGMOD_LEVEL_MAX] = {
    /*[LOGMOD_LEVEL_TRACE]:*/ { "TRACE", __COLOR(BLUE, INTENSITY), 0 },
    /*[LOGMOD_LEVEL_DEBUG]:*/ { "DEBUG", __COLOR(CYAN, FOREGROUND), 0 },
    /*[LOGMOD_LEVEL_INFO]: */ { "INFO", __COLOR(GREEN, FOREGROUND), 0 },
    /*[LOGMOD_LEVEL_WARN]: */ { "WARN", __COLOR(YELLOW, FOREGROUND), 1 },
    /*[LOGMOD_LEVEL_ERROR]:*/ { "ERROR", __COLOR(RED, FOREGROUND), 1 },
    /*[LOGMOD_LEVEL_FATAL]:*/ { "FATAL", __COLOR(MAGENTA, FOREGROUND), 1 },
};
#undef __COLOR

/** @brief Get @ref logmod from any @ref logmod_logger */
#define LOGMOD_FROM_LOGGER(_ctx)                                              \
    ((struct logmod *)((char *)(_ctx)->counter                                \
                       - offsetof(struct logmod, counter)))

static void
_logmod_lock_noop(const struct logmod_logger *_, int __)
{
}

logmod_err
logmod_init(struct logmod *logmod,
            const char *const application_id,
            struct logmod_logger table[],
            unsigned length)
{
    size_t *mut_real_length = (size_t *)&logmod->real_length;
    if (!application_id || !*application_id) {
        logmod_nlog(FATAL, NULL, ("Missing application_id at logmod_init()"),
                    0);
        return LOGMOD_BAD_PARAMETER;
    }
    if (table == NULL) {
        logmod_nlog(FATAL, NULL, ("Missing table at logmod_init()"), 0);
        return LOGMOD_BAD_PARAMETER;
    }
    if (length == 0) {
        logmod_nlog(FATAL, NULL, ("Invalid length at logmod_init()"), 0);
        return LOGMOD_BAD_PARAMETER;
    }
    memset(logmod, 0, sizeof *logmod);
    memset(table, 0, length * sizeof *table);

    logmod->application_id = application_id;
    logmod->loggers = table;
    *mut_real_length = length;
    logmod->lock = _logmod_lock_noop;

    return LOGMOD_OK;
}

logmod_err
logmod_cleanup(struct logmod *logmod)
{
    size_t i;
    for (i = 0; i < logmod->length; ++i) {
        const struct logmod_mut_logger *mut_logger =
            (struct logmod_mut_logger *)&logmod->loggers[i];
        if (mut_logger->buf.content) free(mut_logger->buf.content);
    }
    memset((void *)logmod->loggers, 0,
           logmod->real_length * sizeof *logmod->loggers);
    memset(logmod, 0, sizeof *logmod);

    return LOGMOD_OK;
}

logmod_err
logmod_set_lock(struct logmod *logmod, logmod_lock lock)
{
    if (logmod == NULL) {
        logmod_nlog(ERROR, NULL,
                    ("Missing logmod at logmod_logger_set_lock()"), 0);
        return LOGMOD_BAD_PARAMETER;
    }
    if (lock == NULL) {
        logmod_nlog(ERROR, NULL, ("Missing lock at logmod_logger_set_lock()"),
                    0);
        return LOGMOD_BAD_PARAMETER;
    }

    logmod->lock = lock;

    return LOGMOD_OK;
}

logmod_err
logmod_logger_set_data(struct logmod_logger *logger, void *user_data)
{
    struct logmod_mut_logger *mut_logger = (struct logmod_mut_logger *)logger;
    if (logger == NULL) {
        logmod_nlog(ERROR, NULL,
                    ("Missing logger at logmod_logger_set_data()"), 0);
        return LOGMOD_BAD_PARAMETER;
    }

    mut_logger->user_data = user_data;

    return LOGMOD_OK;
}

logmod_err
logmod_logger_set_callback(struct logmod_logger *logger,
                           const struct logmod_label *const custom_labels,
                           const size_t num_custom_labels,
                           logmod_callback callback)
{
    struct logmod_mut_logger *mut_logger = (struct logmod_mut_logger *)logger;
    if (mut_logger == NULL) {
        logmod_nlog(ERROR, NULL,
                    ("Missing logger at logmod_logger_set_callback()"), 0);
        return LOGMOD_BAD_PARAMETER;
    }
    if (callback == NULL) {
        logmod_nlog(ERROR, NULL,
                    ("Missing callback at logmod_logger_set_callback()"), 0);
        return LOGMOD_BAD_PARAMETER;
    }

    mut_logger->callback = callback;
    if (custom_labels != NULL) {
        mut_logger->custom_labels = custom_labels;
        mut_logger->num_custom_labels = num_custom_labels;
    }

    return LOGMOD_OK;
}

logmod_err
logmod_logger_set_options(struct logmod_logger *logger,
                          struct logmod_logger_options options)
{
    struct logmod_mut_logger *mut_logger = (struct logmod_mut_logger *)logger;
    if (mut_logger == NULL) {
        logmod_nlog(ERROR, NULL,
                    ("Missing logger at logmod_logger_set_options()"), 0);
        return LOGMOD_BAD_PARAMETER;
    }

    mut_logger->options = options;

    return LOGMOD_OK;
}

logmod_err
logmod_logger_set_quiet(struct logmod_logger *logger, int quiet)
{
    struct logmod_mut_logger *mut_logger = (struct logmod_mut_logger *)logger;
    if (mut_logger == NULL) {
        logmod_nlog(ERROR, NULL,
                    ("Missing logger at logmod_logger_set_quiet()"), 0);
        return LOGMOD_BAD_PARAMETER;
    }

    mut_logger->options.quiet = quiet;

    return LOGMOD_OK;
}

logmod_err
logmod_logger_set_color(struct logmod_logger *logger, int color)
{
    struct logmod_mut_logger *mut_logger = (struct logmod_mut_logger *)logger;
    if (mut_logger == NULL) {
        logmod_nlog(ERROR, NULL,
                    ("Missing logger at logmod_logger_set_color()"), 0);
        return LOGMOD_BAD_PARAMETER;
    }

    mut_logger->options.color = color;

    return LOGMOD_OK;
}

logmod_err
logmod_logger_set_logfile(struct logmod_logger *logger, FILE *logfile)
{
    struct logmod_mut_logger *mut_logger = (struct logmod_mut_logger *)logger;
    if (mut_logger == NULL) {
        logmod_nlog(ERROR, NULL,
                    ("Missing logger at logmod_logger_set_logfile()"), 0);
        return LOGMOD_BAD_PARAMETER;
    }

    mut_logger->options.logfile = logfile;

    return LOGMOD_OK;
}

const struct logmod_label *const
logmod_logger_get_label(const struct logmod_logger *logger,
                        const unsigned level)
{
    if (level >= 0 && level < LOGMOD_LEVEL_CUSTOM) {
        return &default_labels[level];
    }
    if (logger && level >= LOGMOD_LEVEL_CUSTOM
        && level < (LOGMOD_LEVEL_CUSTOM + logger->num_custom_labels))
    {
        return &logger->custom_labels[level - LOGMOD_LEVEL_CUSTOM];
    }
    return NULL;
}

long
logmod_logger_get_level(const struct logmod_logger *logger,
                        const char *const label)
{
    size_t i;

    if (logger == NULL) {
        logmod_nlog(ERROR, NULL, ("Missing logger at logmod_get_level()"), 0);
        return -2;
    }
    if (label == NULL) {
        logmod_nlog(ERROR, logger, ("Missing label at logmod_get_level()"), 0);
        return -2;
    }

    for (i = 0; i < __LOGMOD_LEVEL_MAX; ++i) {
        if (strcmp(label, default_labels[i].name) == 0) {
            return i;
        }
    }
    for (i = 0; i < logger->num_custom_labels; ++i) {
        if (strcmp(label, logger->custom_labels[i].name) == 0) {
            return i + LOGMOD_LEVEL_CUSTOM;
        }
    }
    return -1;
}

long
logmod_logger_get_counter(const struct logmod_logger *logger)
{
    struct logmod *logmod = LOGMOD_FROM_LOGGER(logger);
    long counter;

    if (logger == NULL) {
        logmod_nlog(ERROR, NULL,
                    ("Missing logger at logmod_logger_get_counter()"), 0);
        return -1;
    }

    logmod->lock(logger, 1);
    counter = logmod->counter;
    logmod->lock(logger, 0);

    return counter;
}

struct logmod_logger *
logmod_get_logger(struct logmod *logmod, const char *const context_id)
{
    struct logmod_mut_logger *mut_loggers =
        (struct logmod_mut_logger *)logmod->loggers;
    size_t *mut_length = (size_t *)&logmod->length;
    size_t i;

    for (i = 0; i < logmod->length; ++i) {
        if (0 == strcmp(logmod->loggers[i].context_id, context_id))
            return (struct logmod_logger *)&logmod->loggers[i];
    }

    if ((logmod->length + 1) >= logmod->real_length) {
        return NULL;
    }

    memset(&mut_loggers[logmod->length], 0, sizeof *mut_loggers);
    mut_loggers[logmod->length].context_id = context_id;
    mut_loggers[logmod->length].counter = &logmod->counter;

    return (struct logmod_logger *)&logmod->loggers[(*mut_length)++];
}

static logmod_err
_logmod_print(const struct logmod_logger *logger,
              const struct tm *time_info,
              const struct logmod_label *const label,
              const char *fmt,
              va_list args,
              const int color,
              FILE *output)
{
    if (color) {
        if (0 >= fprintf(output, "%02d:%02d:%02d \x1b%um%s\x1b[0m %s:%d: ",
                         time_info->tm_hour, time_info->tm_min,
                         time_info->tm_sec, label->color, label->name,
                         logger->filename, logger->line))
        {
            logmod_nlog(ERROR, logger, ("Failed to write to output stream"),
                        0);
            return LOGMOD_ERRNO;
        }
    }
    else if (0 >= fprintf(output,
                          "%02d:%02d:%02d %s %s:%d: ", time_info->tm_hour,
                          time_info->tm_min, time_info->tm_sec, label->name,
                          logger->filename, logger->line))
    {
        logmod_nlog(ERROR, logger, ("Failed to write to output stream"), 0);
        return LOGMOD_ERRNO;
    }

    if (0 >= vfprintf(output, fmt, args)) {
        logmod_nlog(ERROR, logger, ("Failed to write to output stream"), 0);
        return LOGMOD_ERRNO;
    }
    if (putc('\n', output) == EOF) {
        logmod_nlog(ERROR, logger, ("Failed to write to output stream"), 0);
        return LOGMOD_ERRNO;
    }
    if (fflush(output) == EOF) {
        logmod_nlog(ERROR, logger, ("Failed to write to output stream"), 0);
        return LOGMOD_ERRNO;
    }

    return LOGMOD_OK;
}

static struct logmod g_logmod;

/** global logger used as a fallback */
static struct logmod_logger g_loggers[] = {
    { 0,
      NULL,
      0,
      LOGMOD_FALLBACK_CONTEXT_ID,
      { NULL },
      &g_logmod.counter,
      NULL,
      NULL,
      default_labels,
      0 },
};
/** global logmod used as a fallback */
static struct logmod g_logmod = {
    LOGMOD_FALLBACK_APPLICATION_ID,
    g_loggers,
    sizeof(g_loggers) / sizeof *g_loggers,
    sizeof(g_loggers) / sizeof *g_loggers,
    0,
    _logmod_lock_noop,
};

logmod_err
_logmod_log(const struct logmod_logger *logger,
            const unsigned line,
            const char *const filename,
            const unsigned level,
            const char *fmt,
            ...)
{
    struct logmod *logmod = logger ? LOGMOD_FROM_LOGGER(logger)
                                   : ((logger = &g_loggers[0]), &g_logmod);
    struct logmod_mut_logger *mut_logger = (struct logmod_mut_logger *)logger;
    const struct logmod_label *const label =
        logmod_logger_get_label(logger, level);
    logmod_err code = LOGMOD_OK;
    va_list args;

    mut_logger->line = line;
    mut_logger->filename = filename;
    mut_logger->level = level;

    if (logger->callback) {
        va_start(args, fmt);
        if ((code = logger->callback(logger, label, fmt, args)) != LOGMOD_OK) {
            return va_end(args), code;
        }
        va_end(args);
    }
    else {
        if (!logger->options.quiet || level == LOGMOD_LEVEL_FATAL) {
            const time_t time_raw = time(NULL);
            const struct tm *time_info = localtime(&time_raw);
            va_start(args, fmt);
            if ((code = _logmod_print(logger, time_info, label, fmt, args,
                                      logger->options.color,
                                      label->output == 0 ? stdout : stderr))
                != LOGMOD_OK)
            {
                return va_end(args), code;
            }
            va_end(args);
        }
        if (logger->options.logfile) {
            const time_t time_raw = time(NULL);
            const struct tm *time_info = localtime(&time_raw);
            va_start(args, fmt);
            if ((code = _logmod_print(logger, time_info, label, fmt, args, 0,
                                      logger->options.logfile))
                != LOGMOD_OK)
            {
                return va_end(args), code;
            }
            va_end(args);
        }
    }

    logmod->lock(logger, 1);
    ++logmod->counter;
    logmod->lock(logger, 0);

    return code;
}

const char *
_logmod_encode(const struct logmod_logger *logger,
               const char *buf,
               enum logmod_colors color,
               enum logmod_styles style,
               enum logmod_visibility visibility)
{
    static const size_t ansi_extra_bytes = sizeof("\x1b[-;---m\x1b[0m");
    const size_t expected_new_length = strlen(buf) + ansi_extra_bytes;
    struct logmod_mut_logger *mut_logger = (struct logmod_mut_logger *)logger;

    if (logger == NULL) {
        logmod_nlog(ERROR, NULL, ("Missing logger at logmod_encode()"), 0);
        return NULL;
    }
    if (!logger->options.color) {
        return buf;
    }

    if (logger->buf.length < expected_new_length) {
        void *tmp = realloc(mut_logger->buf.content, expected_new_length);
        if (tmp == NULL) {
            logmod_nlog(ERROR, logger, ("Out of memory at logmod_encode()"),
                        0);
            return NULL;
        }
        mut_logger->buf.content = tmp;
        mut_logger->buf.length = expected_new_length;
    }
    if (0 >= sprintf(mut_logger->buf.content, "\x1b[%u;%um%s\x1b[0m", style,
                     color + visibility, buf))
    {
        logmod_nlog(ERROR, logger,
                    ("Error: sprintf() failure at logmod_encode()"), 0);
        return NULL;
    }
    return logger->buf.content;
}
