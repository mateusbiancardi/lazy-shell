#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <termios.h>
#include "ish.h"

IshState *lasy_ptr = NULL;

void signal_wrapper(int sig) {
    handle_sigint_ctrl_c(sig, lasy_ptr);
}

void sigchld_wrapper(int sig) {
    handle_sigchld(sig, lasy_ptr);
}

int main (){

    // desativa echoctl para nao mostrar ctrl-c
    struct termios t;
    tcgetattr(STDIN_FILENO, &t);      
    t.c_lflag &= ~ECHOCTL;            
    tcsetattr(STDIN_FILENO, TCSANOW, &t); 

    // sigint executa o buffer e sigchld atualiza estado dos filhos
    signal(SIGINT, signal_wrapper);
    signal(SIGCHLD, sigchld_wrapper);
    IshState * lasy_shell = construct();
    lasy_shell->interactive = (isatty(STDIN_FILENO) && isatty(STDOUT_FILENO));
    lasy_ptr = lasy_shell;
    
    while (1)
    {
        read_line(lasy_shell);
    }

    return 0;
}
