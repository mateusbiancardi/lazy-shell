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

    // Disable ECHOCTL so ^C is not printed.
    struct termios t;
    tcgetattr(STDIN_FILENO, &t);      
    t.c_lflag &= ~ECHOCTL;            
    tcsetattr(STDIN_FILENO, TCSANOW, &t); 

    // SIGINT runs the lazy buffer; SIGCHLD keeps child state updated.
    signal(SIGINT, signal_wrapper);
    signal(SIGCHLD, sigchld_wrapper);
    IshState * lasy_shell = construct();
    lasy_ptr = lasy_shell;
    
    while (1)
    {   
        printf("lsh> ");
        read_line(lasy_shell);
    }

    return 0;
}
