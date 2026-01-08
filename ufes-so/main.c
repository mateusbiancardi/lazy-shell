#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <termios.h>
#include <unistd.h>  
#include "ish.h"

IshState *lasy_ptr = NULL;

void signal_wrapper(int sig) {
    handle_sigint_ctrl_c(sig, lasy_ptr);
}

int main (){

    // Configuraçao pra nao imprimir o ^C no terminal,
    // o terminal para de ecor caracteres de controle
    struct termios t;
    tcgetattr(STDIN_FILENO, &t);      
    t.c_lflag &= ~ECHOCTL;
    tcsetattr(STDIN_FILENO, TCSANOW, &t); 

    // Tratamento do SIGINT (Ctrl c)
    signal(SIGINT, signal_wrapper);
    IshState * lasy_shell = construct();
    lasy_ptr = lasy_shell;
    
    while (1)
    {   
        printf("lsh> ");
        read_line(lasy_shell);
    }

    return 0;
}