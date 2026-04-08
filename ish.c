// construcao do estado inicial
#include "ish.h"
#include <stdlib.h>

IshState *construct()
{
    IshState *state = (IshState *)malloc(sizeof(IshState));
    if (state == NULL)
    {
        return NULL;
    }
    state->count = 0;
    state->last_ctrl_c = 0;
    state->has_children = 0;
    state->interactive = 0;
    state->child_count = 0;
    state->zombie_count = 0;
    return state;
}
