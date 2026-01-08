#include "ish.h"
#include "exec.h"
#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <errno.h>
#include <string.h>


IshState *construct()
{   
    IshState * x = (IshState *) malloc(sizeof(IshState));
    x->count = 0;
    x->last_ctrl_c = 0;
    x->has_children = 0;
    return x;
}

void _discard_line(CommandLine c){
    if(c.has_pipe == 1){
        free(c.cmd1);
        free(c.cmd2);
    }
    else
        free(c.cmd1);

}   


void break_line(char *line, Command *cmd) {
    int i = 0;
    
    // divide a string no primeiro espaço
    char *token = strtok(line," \n");

    while (token != NULL && i < MAX_ARGS) {
        // aloca memoria nova e copia o token 
        cmd->args[i] = strdup(token); 
        i++;

        // pega o próximo pedaço da string
        token = strtok(NULL, " \n");
    }
    
    // conferir essa finalizacao dos args, aparentemente é fundamental pro exec funcionar
    cmd->args[i] = NULL; 
    
    // ajusta o nome do comando, pra facilitar uma busca
    if (i > 0) 
        cmd->name = cmd->args[0]; 
    
}


void read_line(IshState * lasy){

    // limpando o conteudo lido quando o buffer está cheio
    if (lasy->count >= MAX_BUFFER) {
        int ch;
        while ((ch = getchar()) != '\n' && ch != EOF); 
        return; 
    }

    char *line_buffer = NULL; 
    size_t size = 0;
    
    CommandLine c = lasy->buffer[lasy->count];
    
    // leitura da linha
    int ret = getline(&line_buffer, &size, stdin);

    if (ret == -1) {
        free(line_buffer);
        return; 
    }

    // remove o \n
    line_buffer[strcspn(line_buffer, "\n")] = 0;
    
    // verifica se existe o pipe #
    char *pipe = strchr(line_buffer, '#');

    if(pipe !=  NULL){
        c.has_pipe = 1;

        *pipe = '\0'; 
        
        char *parte1 = line_buffer;
        char *parte2 = pipe + 1;

        c.cmd1 = (Command*)malloc(sizeof(Command));
        c.cmd2 = (Command*)malloc(sizeof(Command));

        break_line(parte1, c.cmd1);
        break_line(parte2, c.cmd2);
    }
    else{
        c.has_pipe = 0;
        c.cmd1 = (Command*)malloc(sizeof(Command));
        c.cmd2 = NULL;

        break_line(line_buffer, c.cmd1);
    }

    lasy->buffer[lasy->count] = c;
    lasy->count++;
}

void handle_sigint_ctrl_c(int sig, IshState *lasy)
{
    time_t now = time(NULL);
    double diff = difftime(now, lasy->last_ctrl_c);

    // calculando se ctrl-c foi apertado 2 vezes com até 3s de diferença
    if (lasy->last_ctrl_c != 0 && diff <= 3.0) {
        printf("\nOk... você venceu! Adeus!\n");
        exit(0);
    }
    lasy->last_ctrl_c = now;

    if (lasy->count > 0) {
        execute_buffer(lasy);
        delete_buffer(lasy);
        lasy->count = 0; // Reseta buffer
    }
    else if (lasy->has_children) {
        printf("\nNão posso morrer... sou lenta mas sou mãe de família!!!\n");
    } 
    
    printf("\nlsh> ");
    fflush(stdout);

}

void delete_buffer(IshState * lasy)
{   
    for(int i = 5; i > 0; i--){
        if(lasy->buffer[i-1].has_pipe == 1){
            free(lasy->buffer[i-1].cmd1);
            free(lasy->buffer[i-1].cmd2);
        }
        else{
            free(lasy->buffer[i-1].cmd1);
        }
    }
}
