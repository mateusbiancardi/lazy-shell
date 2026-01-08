#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <termios.h>
#include "ish.h"

static IshState *g_state;

static void on_sigint(int sig) {
    handle_sigint_ctrl_c(sig, g_state);
}

static void on_sigchld(int sig) {
    handle_sigchld(sig, g_state);
}

static void disable_ctrl_echo() {
    struct termios t;
    tcgetattr(STDIN_FILENO, &t);
#ifdef ECHOCTL
    t.c_lflag &= ~ECHOCTL;
#endif
    tcsetattr(STDIN_FILENO, TCSANOW, &t);
}

static void setup_signals() {
    signal(SIGINT, on_sigint);
    signal(SIGCHLD, on_sigchld);
}

static void print_prompt(const IshState *state) {
    if (state->interactive) {
        printf("lsh> ");
        fflush(stdout);
    }
}

// loop principal e leitura.
int main(void) {
    disable_ctrl_echo();
    
    IshState *state = construct();
    if (state == NULL) {
        fprintf(stderr, "Erro inicializar o shell\n");
        return 1;
    }
    
    state-> = (isatty(STDIN_FILENO) && isatty(STDOUT_FILENO));
    g_state = state;
    
    setup_signals();
    
    while (1) {
        print_prompt(state);
        read_line(state);
    }
    
    return 0;
}
