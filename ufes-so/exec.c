#include "exec.h"
#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>
#include <string.h>

void execute_command(CommandLine cmd)
{   
    // processos internos
    if (cmd.has_pipe == 0) {
        char *prog = cmd.cmd1->name;
        
        if (strcmp(prog, "cd") == 0) {
            printf("\nRotina tratamento instrução cd...");
            return;
        }
        if (strcmp(prog, "exit") == 0) {
            printf("\nRotina tratamento instrução exit...");
            return;
        }
        if (strcmp(prog, "wait") == 0) {
            printf("\nRotina tratamento instrução wait...");
            return;
        }
    }

    // processos externos
    // nao entendi essa parte
    // os programas que vão ser abertos serao passados pela argv????
}

void execute_buffer(IshState *lasy)
{
    for (int i = 0; i < lasy->count; i++) {
        execute_command(lasy->buffer[i]);
    }
}
