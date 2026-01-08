//leitura e parsing de comandos
#include "ish.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int is_internal(const char *name) {
    return (strcmp(name, "cd") == 0 ||
            strcmp(name, "wait") == 0 ||
            strcmp(name, "exit") == 0);
}
//gerencia memoria
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
//auxiliar do parsing (para strings)
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

static void tokenize_command(char *line, Command *cmd) {
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

static int validate_command(Command *cmd) {
    return cmd != NULL && cmd->name != NULL;
}

static int validate_pipe_command(CommandLine *c) {
    if (!validate_command(c->cmd1)) {
        return 0;
    }
    
    if (c->has_pipe) {
        if (!validate_command(c->cmd2)) {
            return 0;
        }
        if (is_internal(c->cmd1->name) || is_internal(c->cmd2->name)) {
            return 0;
        }
    }
    
    return 1;
}
//parsing de linhas
static Command *create_command() {
    Command *cmd = (Command *)malloc(sizeof(Command));
    if (cmd != NULL) {
        memset(cmd, 0, sizeof(Command));
    }
    return cmd;
}

static int parse_simple_command(char *line, CommandLine *c) {
    c->has_pipe = 0;
    c->cmd1 = create_command();
    c->cmd2 = NULL;
    
    if (c->cmd1 == NULL) {
        return 0;
    }
    
    tokenize_command(line, c->cmd1);
    return 1;
}

static int parse_pipe_command(char *line, char *pipe_pos, CommandLine *c) {
    c->has_pipe = 1;
    *pipe_pos = '\0';
    
    char *part1 = trim_spaces(line);
    char *part2 = trim_spaces(pipe_pos + 1);
    
    c->cmd1 = create_command();
    c->cmd2 = create_command();
    
    if (c->cmd1 == NULL || c->cmd2 == NULL) {
        free_command(c->cmd1);
        free_command(c->cmd2);
        return 0;
    }
    
    tokenize_command(part1, c->cmd1);
    tokenize_command(part2, c->cmd2);
    
    return 1;
}

static int parse_line(char *line, CommandLine *c) {
    char *pipe = strchr(line, '#');
    
    // verifica se tem mais de um pipe
    if (pipe != NULL && strchr(pipe + 1, '#') != NULL) {
        return 0;
    }
    
    if (pipe != NULL) {
        return parse_pipe_command(line, pipe, c);
    } else {
        return parse_simple_command(line, c);
    }
}

static char *read_input_line() {
    char *line = NULL;
    size_t size = 0;
    
    int ret = getline(&line, &size, stdin);
    if (ret == -1) {
        free(line);
        return NULL;
    }
    
    line[strcspn(line, "\n")] = 0;
    return line;
}

static void discard_overflow_input() {
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF);
}

void read_line(IshState *state) {
    // le, valida e enfileira uma linha; descarta se buffer cheio.
    if (state->count >= MAX_BUFFER) {
        discard_overflow_input();
        return;
    }
    
    char *line = read_input_line();
    if (line == NULL) {
        if (!state->interactive) {
            exit(0);
        }
        return;
    }
    
    char *clean = trim_spaces(line);
    if (clean[0] == '\0') {
        free(line);
        return;
    }
    
    CommandLine c; //faz parsing
    if (!parse_line(clean, &c)) {
        free(line);
        return;
    }
    
    free(line);
    
    if (!validate_pipe_command(&c)) {
        free_command(c.cmd1);
        free_command(c.cmd2);
        return;
    }
    
    state->buffer[state->count] = c;
    state->count++;
}

void delete_buffer(IshState *state) {
    for (int i = 0; i < state->count; i++) {
        free_command(state->buffer[i].cmd1);
        if (state->buffer[i].has_pipe) {
            free_command(state->buffer[i].cmd2);
        }
        state->buffer[i].cmd1 = NULL;
        state->buffer[i].cmd2 = NULL;
        state->buffer[i].has_pipe = 0;
    }
}
