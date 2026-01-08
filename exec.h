#ifndef EXEC_H
#define EXEC_H

#include "ish.h"

void exec_command(CommandLine cmd, IshState *state, pid_t *group_id);
void exec_buffer(IshState *state);

#endif