#include "ish.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// verifica se o comando eh interno
static int is_internal(const char *name) {
    return (strcmp(name, "cd") == 0 ||
            strcmp(name, "wait") == 0 ||
            strcmp(name, "exit") == 0);
}

// libera memoria de um comando
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

// remove espacos no inicio e fim
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

// preenche comando com tokens da linha
static void break_line(char *line, Command *cmd) {
    int i = 0;
    char *token = strtok(line, " \n");

    while (token != NULL && i < MAX_ARGS) {
        cmd->args[i] = strdup(token);
        i++;
        token = strtok(NULL, " \n");
    }

    cmd->args[i] = NULL;
    if (i > 0) {
        cmd->name = cmd->args[0];
    }
}

// le linha do usuario trata hash e enfileira no buffer
void read_line(IshState *lasy){
    if (lasy->count >= MAX_BUFFER) {
        int ch;
        while ((ch = getchar()) != '\n' && ch != EOF);
        return;
    }

    char *line_buffer = NULL;
    size_t size = 0;
    CommandLine c = lasy->buffer[lasy->count];

    int ret = getline(&line_buffer, &size, stdin);
    if (ret == -1) {
        free(line_buffer);
        if (!lasy->interactive) {
            exit(0);
        }
        return;
    }

    line_buffer[strcspn(line_buffer, "\n")] = 0;
    char *clean_line = trim_spaces(line_buffer);
    if (clean_line[0] == '\0') {
        free(line_buffer);
        return;
    }

    char *pipe = strchr(clean_line, '#');
    if (pipe != NULL) {
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
    } else {
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

// libera a memoria do buffer atual
void delete_buffer(IshState *lasy)
{
    for (int i = 0; i < lasy->count; i++) {
        if (lasy->buffer[i].has_pipe == 1) {
            free_command(lasy->buffer[i].cmd1);
            free_command(lasy->buffer[i].cmd2);
        } else {
            free_command(lasy->buffer[i].cmd1);
        }
        lasy->buffer[i].cmd1 = NULL;
        lasy->buffer[i].cmd2 = NULL;
        lasy->buffer[i].has_pipe = 0;
    }
}
