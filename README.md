# LogMod

LogMod is a simple logging module for C applications. It provides functionality to initialize a logging context, retrieve loggers, and log messages with different severity levels.

## Features

- Initialize logging context with application ID and logger table.
- Retrieve or create loggers by context ID.
- Log messages with different severity levels (TRACE, DEBUG, INFO, WARN, ERROR, FATAL).
- Optionally log messages to a file.

## Installation

To use LogMod in your project, include the `logmod.h` header file and link against the `logmod.c` source file.

## Usage

### Initialization

To initialize the logging context, use the `logmod_init` function:

```c
#include "logmod.h"

struct logmod logmod;
struct logmod_logger table[5];

logmod_err code = logmod_init(&logmod, "APPLICATION_ID", table, 5);
if (code != LOGMOD_OK) {
    // Handle error
}
```

### Retrieving Loggers

To retrieve or create a logger by context ID, use the `logmod_logger_get` function:

```c
struct logmod_logger *LOGMOD_LOGGER = logmod_logger_get(&logmod, "CONTEXT_ID");
if (LOGMOD_LOGGER == NULL) {
    // Handle error
}
```

The logger is stored in the `LOGMOD_LOGGER` variable, which is a macro created to ensure ANSI C compatibility.

### Logging Messages

To log messages with different severity levels, use provided macros:

```c
LOGMOD_TRACE((LOGMOD_LOGGER, "This is a trace message"));
LOGMOD_DEBUG((LOGMOD_LOGGER, "This is a debug message"));
LOGMOD_INFO((LOGMOD_LOGGER, "This is an info message"));
LOGMOD_WARN((LOGMOD_LOGGER, "This is a warning message"));
LOGMOD_ERROR((LOGMOD_LOGGER, "This is an error message"));
LOGMOD_FATAL((LOGMOD_LOGGER, "This is a fatal message"));
```

#### Using `printf` Formatters

LogMod supports formatted logging messages similar to `printf`. Here are some examples:

```c
int value = 42;
const char *name = "example";

LOGMOD_INFO((LOGMOD_LOGGER, "Integer value: %d", value));
LOGMOD_DEBUG((LOGMOD_LOGGER, "String value: %s", name));
LOGMOD_WARN((LOGMOD_LOGGER, "Combined values: %s = %d", name, value));
```

These macros allow you to include variable data in your log messages, making them more informative and useful for debugging.

### Cleanup

To clean up the logging context, use the `logmod_cleanup` function:

```c
logmod_cleanup(&logmod);
```

## API Reference

### `logmod_init`

```c
logmod_err logmod_init(struct logmod *logmod, const char *const app_id, struct logmod_logger table[], unsigned length);
```

Initializes the logging context with the specified application ID and logger table.
- `logmod`: Pointer to the logging context structure.
- `app_id`: Application ID string.
- `table`: Logger table array.
- `length`: Length of the logger table array.

Returns `LOGMOD_OK` on success, or an error code on failure.

### `logmod_cleanup`

```c
logmod_err logmod_cleanup(struct logmod *logmod);
```

Cleans up the logging context.
- `logmod`: Pointer to the logging context structure.
Returns `LOGMOD_OK` on success

### `logmod_logger_get`

```c
struct logmod_logger *logmod_logger_get(struct logmod *logmod, const char *const context_id);
```

Retrieves or creates a logger by context ID.
- `logmod`: Pointer to the logging context structure.
- `context_id`: Context ID string.
Returns a pointer to the logger, or `NULL` on failure.
**Note:** The variable MUST be called `LOGMOD_LOGGER` (which is a macro created to ensure ANSI C compatibility).

### `logmod_logger_set_lock`

```c
logmod_err logmod_logger_set_lock(struct logmod_logger *logger, void (*lock)(int should_lock));
```

Sets the lock function for the specified logger.
- `logger`: Pointer to the logger structure.
- `lock`: Lock function pointer.

### `logmod_logger_set_options`

```c
logmod_err logmod_logger_set_options(struct logmod_logger *logger, struct logmod_logger_options options);
```

Sets the options for the logger.
- `logger`: Pointer to the logger structure.
- `options`: Logger options structure.

### `logmod_logger_set_quiet`

```c
logmod_err logmod_logger_set_quiet(struct logmod_logger *logger, int quiet);
```

Sets the quiet mode for the logger.
- `logger`: Pointer to the logger structure.
- `quiet`: Quiet mode flag.
Returns `LOGMOD_OK` on success.

### `logmod_logger_set_logfile`

```c
logmod_err logmod_logger_set_logfile(struct logmod_logger *logger, FILE *logfile);
```

Sets the logfile for the logger.
- `logger`: Pointer to the logger structure.
- `logfile`: Logfile pointer.
Returns `LOGMOD_OK` on success.

### `_logmod_log`

```c
logmod_err _logmod_log(struct logmod_logger *logger, const char *fmt, ...);
```

Logs a message with the specified logger and format string.
- `logger`: Pointer to the logger structure.
- `fmt`: Format string for the message.
Returns `LOGMOD_OK` on success, or an error code on failure.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
