#include "ish.h"
#include <stdlib.h>

// inicializa o estado da shell
IshState *construct()
{
    IshState * x = (IshState *) malloc(sizeof(IshState));
    x->count = 0;
    x->last_ctrl_c = 0;
    x->has_children = 0;
    x->child_count = 0;
    x->zombie_count = 0;
    return x;
}
