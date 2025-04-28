#ifndef LOGMOD_H
#define LOGMOD_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdio.h>
#include <stdarg.h>

#if defined(__MINGW32__)                                                      \
    || (defined(__GNUC__) && __GNUC__ > 4 ? true : __GNUC_PATCHLEVEL__ >= 4)  \
    || defined(__USE_MINGW_ANSI_STDIO)
#define LOGMOD_PRINTF_LIKE(a, b) __attribute__((format(gnu_printf, a, b)))
#else
#define LOGMOD_PRINTF_LIKE(a, b)
#endif

#ifndef LOGMOD_FALLBACK_APPLICATION_ID
#define LOGMOD_FALLBACK_APPLICATION_ID "APPLICATION"
#endif /* LOGMOD_FALLBACK_APPLICATION_ID */

#ifndef LOGMOD_FALLBACK_CONTEXT_ID
#define LOGMOD_FALLBACK_CONTEXT_ID "GLOBAL"
#endif /* LOGMOD_FALLBACK_CONTEXT_ID */

enum logmod_levels {
    LOGMOD_LEVEL_TRACE = 0,
    LOGMOD_LEVEL_DEBUG,
    LOGMOD_LEVEL_INFO,
    LOGMOD_LEVEL_WARN,
    LOGMOD_LEVEL_ERROR,
    LOGMOD_LEVEL_FATAL,
    __LOGMOD_LEVEL_MAX,
    /** user defined log level must start with this value */
    LOGMOD_LEVEL_CUSTOM = __LOGMOD_LEVEL_MAX
};

typedef enum {
    LOGMOD_OK = 0,
    LOGMOD_BAD_PARAMETER = -1,
    LOGMOD_ERRNO = -2
} logmod_err;

struct logmod_logger_options {
    FILE *logfile;
    char *buffer;
    int quiet;
    int color;
};

struct logmod_label {
    const char *const name;
    const unsigned color;
    const int output; /* 0 = stdout, 1 = stderr */
};

/* forward declaration */
struct logmod_logger;
struct tm;
/**/

typedef void (*logmod_lock)(const struct logmod_logger *logger,
                            int should_lock);

typedef logmod_err (*logmod_callback)(const struct logmod_logger *logger,
                                      const struct logmod_label *const level,
                                      const char *fmt,
                                      va_list args);

enum logmod_colors {
    LOGMOD_COLOR_BLACK = 0,
    LOGMOD_COLOR_RED = 1,
    LOGMOD_COLOR_GREEN = 2,
    LOGMOD_COLOR_YELLOW = 3,
    LOGMOD_COLOR_BLUE = 4,
    LOGMOD_COLOR_MAGENTA = 5,
    LOGMOD_COLOR_CYAN = 6,
    LOGMOD_COLOR_WHITE = 7
};

enum logmod_styles {
    LOGMOD_STYLE_REGULAR = 0,
    LOGMOD_STYLE_BOLD = 1,
    LOGMOD_STYLE_UNDERLINE = 4,
    LOGMOD_STYLE_STRIKETHROUGH = 9
};

enum logmod_visibility {
    LOGMOD_VISIBILITY_FOREGROUND = 30,
    LOGMOD_VISIBILITY_BACKGROUND = 40,
    LOGMOD_VISIBILITY_INTENSITY = 90,
    LOGMOD_VISIBILITY_BACKGROUND_INTENSITY = 100
};

#define __LOGMOD_LOGGER_ATTRS(_qualifier)                                     \
    _qualifier unsigned line;                                                 \
    const char *_qualifier filename;                                          \
    _qualifier unsigned level;                                                \
    const char *context_id;                                                   \
    _qualifier struct logmod_logger_options options;                          \
    const long *counter;                                                      \
    _qualifier logmod_callback callback;                                      \
    _qualifier void *user_data;                                               \
    const struct logmod_label *_qualifier custom_labels;                      \
    _qualifier size_t num_custom_labels;                                      \
    _qualifier struct {                                                       \
        _qualifier char *content;                                             \
        _qualifier size_t length;                                             \
    } buf

#define __BLANK
struct logmod_mut_logger {
    __LOGMOD_LOGGER_ATTRS(__BLANK);
};
#undef __BLANK

struct logmod_logger {
    __LOGMOD_LOGGER_ATTRS(const);
};

#undef __LOGMOD_LOGGER_ATTRS

struct logmod {
    const char *application_id;
    const struct logmod_logger *loggers;
    const size_t length;
    const size_t real_length;
    long counter;
    logmod_lock lock;
};

logmod_err logmod_init(struct logmod *logmod,
                       const char *const application_id,
                       struct logmod_logger table[],
                       unsigned length);

logmod_err logmod_cleanup(struct logmod *logmod);

logmod_err logmod_set_lock(struct logmod *logmod, logmod_lock lock);

logmod_err logmod_logger_set_data(struct logmod_logger *logger,
                                  void *user_data);

logmod_err logmod_logger_set_callback(
    struct logmod_logger *logger,
    const struct logmod_label *const custom_labels,
    const size_t num_custom_labels,
    logmod_callback callback);

logmod_err logmod_logger_set_options(struct logmod_logger *logger,
                                     struct logmod_logger_options options);

logmod_err logmod_logger_set_quiet(struct logmod_logger *logger, int quiet);

logmod_err logmod_logger_set_color(struct logmod_logger *logger, int color);

logmod_err logmod_logger_set_logfile(struct logmod_logger *logger,
                                     FILE *logfile);

struct logmod_logger *logmod_get_logger(struct logmod *logmod,
                                        const char *const context_id);

const struct logmod_label *const logmod_logger_get_label(
    const struct logmod_logger *logger, const unsigned level);

long logmod_logger_get_level(const struct logmod_logger *logger,
                             const char *const label);

long logmod_logger_get_counter(const struct logmod_logger *logger);

#define logmod_nlog(_level, _logger, _parenthesized_params, num_params)       \
    _logmod_log(_logger, __LINE__, __FILE__, LOGMOD_LEVEL_##_level,           \
                LOGMOD_SPREAD_TUPLE_##num_params _parenthesized_params)

#if __STDC_VERSION__ && __STDC_VERSION__ >= 199901L

#define _logmod_log_permissive(_level, _line, _file, _logger, _fmt, ...)      \
    _logmod_log(_logger, _line, _file, _level, _fmt "%s", __VA_ARGS__)
#define logmod_log(_level, _logger, ...)                                      \
    _logmod_log_permissive(_logger, __LINE__, __FILE__,                       \
                           LOGMOD_LEVEL_##_level, __VA_ARGS__, "")
#else
#define logmod_log logmod_nlog
#endif /* __STDC_VERSION__ */

logmod_err _logmod_log(const struct logmod_logger *logger,
                       const unsigned line,
                       const char *const filename,
                       const unsigned level,
                       const char *fmt,
                       ...) LOGMOD_PRINTF_LIKE(5, 6);

#define logmod_encode(_logger, _buf, _color, _style, _visibility)             \
    _logmod_encode(_logger, _buf, LOGMOD_COLOR_##_color,                      \
                   LOGMOD_STYLE_##_style, LOGMOD_VISIBILITY_##_visibility)

const char *_logmod_encode(const struct logmod_logger *logger,
                           const char *buf,
                           enum logmod_colors color,
                           enum logmod_styles style,
                           enum logmod_visibility visibility);

#define LOGMOD_SPREAD_TUPLE_0(_fmt)                 _fmt
#define LOGMOD_SPREAD_TUPLE_1(_fmt, _1)             _fmt, _1
#define LOGMOD_SPREAD_TUPLE_2(_fmt, _1, _2)         _fmt, _1, _2
#define LOGMOD_SPREAD_TUPLE_3(_fmt, _1, _2, _3)     _fmt, _1, _2, _3
#define LOGMOD_SPREAD_TUPLE_4(_fmt, _1, _2, _3, _4) _fmt, _2, _3, _4
#define LOGMOD_SPREAD_TUPLE_5(_fmt, _1, _2, _3, _4, _5)                       \
    _fmt, _1, _2, _3, _4, _5
#define LOGMOD_SPREAD_TUPLE_6(_fmt, _1, _2, _3, _4, _5, _6)                   \
    _fmt, _1, _2, _3, _4, _6
#define LOGMOD_SPREAD_TUPLE_7(_fmt, _1, _2, _3, _4, _5, _6, _7)               \
    _fmt, _1, _2, _3, _4, _5, _6, _7
#define LOGMOD_SPREAD_TUPLE_8(_fmt, _1, _2, _3, _4, _5, _6, _7, _8)           \
    _fmt, _1, _2, _3, _4, _5, _6, _7, _8
#define LOGMOD_SPREAD_TUPLE_9(_fmt, _1, _2, _3, _4, _5, _6, _7, _8, _9)       \
    _fmt, _1, _2, _3, _4, _5, _6, _7, _8, _9
#define LOGMOD_SPREAD_TUPLE_10(_fmt, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10) \
    _fmt, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10
#define LOGMOD_SPREAD_TUPLE_11(_fmt, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
                               _11)                                           \
    _fmt, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11
#define LOGMOD_SPREAD_TUPLE_12(_fmt, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
                               _11, _12)                                      \
    _fmt, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12
#define LOGMOD_SPREAD_TUPLE_13(_fmt, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
                               _11, _12, _13)                                 \
    _fmt, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13
#define LOGMOD_SPREAD_TUPLE_14(_fmt, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
                               _11, _12, _13, _14)                            \
    _fmt, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14
#define LOGMOD_SPREAD_TUPLE_15(_fmt, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
                               _11, _12, _13, _14, _15)                       \
    _fmt, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15
#define LOGMOD_SPREAD_TUPLE_16(_fmt, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, \
                               _11, _12, _13, _14, _15, _16)                  \
    _fmt, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LOGMOD_H */
