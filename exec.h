#if !defined(EXEC_C)
#define EXEC_C

#include "ish.h"

void execute_command(CommandLine cmd, IshState *lasy, pid_t *group_id);
void execute_buffer(IshState *lasy);

#endif // EXEC_C
