#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <termios.h>
#include "ish.h"

IshState *lasy_ptr = NULL;

// Redireciona o sinal do Ctrl-C
void signal_wrapper(int sig) {
    handle_sigint_ctrl_c(sig, lasy_ptr);
}

// Redireciona o sinal de processos filhos para limpar os zombies automaticamente
void sigchld_wrapper(int sig) {
    handle_sigchld(sig, lasy_ptr);
}

// Configura o terminal para não mostrar '^C' no terminal quando apertar as teclas
static void disable_ctrl_echo() {
    struct termios t;
    tcgetattr(STDIN_FILENO, &t);
    #ifdef ECHOCTL
        t.c_lflag &= ~ECHOCTL;
    #endif
    tcsetattr(STDIN_FILENO, TCSANOW, &t);
}

int main(void) {
    disable_ctrl_echo();
    
    // Tratamento do SIGINT (Ctrl c) e SIGCHLD
    signal(SIGINT, signal_wrapper);
    signal(SIGCHLD, sigchld_wrapper);

    IshState *lasy_shell = construct();
    if (lasy_shell == NULL) {
        fprintf(stderr, "Erro ao inicializar a shell.\n");
        return 1;
    }
    
    // Verifica se stdin e stdout são o terminal
    lasy_shell->interactive = (isatty(STDIN_FILENO) && isatty(STDOUT_FILENO));
    
    // Ponteiro para acesso dos handle
    lasy_ptr = lasy_shell;
    
    // Loop infinito que exibe o prompt e fica lendo/armazenando linhas no buffer
    while (1) {
        printf("lsh> ");
        fflush(stdout);
        read_line(lasy_shell);
    }
    
    return 0;
}
