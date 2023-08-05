#include "logmod.h"

int
main(void)
{
    struct logmod *logmod = logmod_init("CONCORD");
    struct logmod_logger *LOGMOD_LOGGER = logmod_logger_get(logmod, "TEST");

    LOGMOD_DEBUG((LOGMOD_LOGGER, "%s %d %f", "oi", 2, 1.7));

    logmod_cleanup(logmod);

    return 0;
}
