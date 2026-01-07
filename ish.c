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
    x->child_count = 0;
    return x;
}

static int is_internal(const char *name) {
    return (strcmp(name, "cd") == 0 ||
            strcmp(name, "wait") == 0 ||
            strcmp(name, "exit") == 0);
}

static void free_command(Command *cmd) {
    if (cmd == NULL) {
        return;
    }
    for (int i = 0; i < MAX_ARGS; i++) {
        if (cmd->args[i] != NULL) {
            free(cmd->args[i]);
            cmd->args[i] = NULL;
        }
    }
    cmd->name = NULL;
    free(cmd);
}

static char *trim_spaces(char *s) {
    while (*s == ' ' || *s == '\t') {
        s++;
    }
    if (*s == '\0') {
        return s;
    }
    char *end = s + strlen(s) - 1;
    while (end > s && (*end == ' ' || *end == '\t')) {
        *end = '\0';
        end--;
    }
    return s;
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
    // ignora o restante dos tokens acima do limite
    
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
    char *clean_line = trim_spaces(line_buffer);
    if (clean_line[0] == '\0') {
        free(line_buffer);
        return;
    }
    
    // verifica se existe o pipe #
    char *pipe = strchr(clean_line, '#');

    if(pipe !=  NULL){
        if (strchr(pipe + 1, '#') != NULL) {
            free(line_buffer);
            return;
        }
        c.has_pipe = 1;

        *pipe = '\0'; 
        
        char *parte1 = trim_spaces(clean_line);
        char *parte2 = trim_spaces(pipe + 1);

        c.cmd1 = (Command*)malloc(sizeof(Command));
        c.cmd2 = (Command*)malloc(sizeof(Command));

        memset(c.cmd1, 0, sizeof(Command));
        memset(c.cmd2, 0, sizeof(Command));

        break_line(parte1, c.cmd1);
        break_line(parte2, c.cmd2);
    }
    else{
        c.has_pipe = 0;
        c.cmd1 = (Command*)malloc(sizeof(Command));
        c.cmd2 = NULL;

        memset(c.cmd1, 0, sizeof(Command));
        break_line(clean_line, c.cmd1);
    }

    free(line_buffer);

    if (c.cmd1 == NULL || c.cmd1->name == NULL) {
        free_command(c.cmd1);
        free_command(c.cmd2);
        return;
    }
    if (c.has_pipe == 1 && (c.cmd2 == NULL || c.cmd2->name == NULL)) {
        free_command(c.cmd1);
        free_command(c.cmd2);
        return;
    }
    if (c.has_pipe == 1 && is_internal(c.cmd1->name)) {
        free_command(c.cmd1);
        free_command(c.cmd2);
        return;
    }
    if (c.has_pipe == 1 && is_internal(c.cmd2->name)) {
        free_command(c.cmd1);
        free_command(c.cmd2);
        return;
    }

    lasy->buffer[lasy->count] = c;
    lasy->count++;
}

void handle_sigint_ctrl_c(int sig, IshState *lasy)
{
    time_t now = time(NULL);
    double diff = difftime(now, lasy->last_ctrl_c);
    int in_window = (lasy->last_ctrl_c != 0 && diff <= 3.0);

    if (lasy->count > 0) {
        execute_buffer(lasy);
        delete_buffer(lasy);
        lasy->count = 0; // Reseta buffer
        lasy->last_ctrl_c = 0;
    }
    else if (lasy->has_children) {
        if (in_window) {
            printf("\nOk... você venceu! Adeus!\n");
            exit(0);
        }
        printf("\nNão posso morrer... sou lenta mas sou mãe de família!!!\n");
        lasy->last_ctrl_c = now;
    } 
    else {
        printf("\nOk... você venceu! Adeus!\n");
        exit(0);
    } 
    
    printf("\nlsh> ");
    fflush(stdout);

}

void handle_sigchld(int sig, IshState *lasy)
{
    int status = 0;
    pid_t pid = waitpid(-1, &status, WNOHANG);
    while (pid > 0) {
        for (int i = 0; i < lasy->child_count; i++) {
            if (lasy->children[i] == pid) {
                lasy->children[i] = lasy->children[lasy->child_count - 1];
                lasy->child_count--;
                break;
            }
        }
        if (lasy->child_count == 0) {
            lasy->has_children = 0;
        }
        pid = waitpid(-1, &status, WNOHANG);
    }
}

void delete_buffer(IshState * lasy)
{   
    for(int i = 0; i < lasy->count; i++){
        if(lasy->buffer[i].has_pipe == 1){
            free_command(lasy->buffer[i].cmd1);
            free_command(lasy->buffer[i].cmd2);
        }
        else{
            free_command(lasy->buffer[i].cmd1);
        }
        lasy->buffer[i].cmd1 = NULL;
        lasy->buffer[i].cmd2 = NULL;
        lasy->buffer[i].has_pipe = 0;
    }
}
