//construcao do estado inicial
#include "ish.h"
#include <stdlib.h>

IshState *construct(void) {
    IshState *state = calloc(1, sizeof(*state));
    return state;
}
