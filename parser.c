// Leitura e transformação do texto digitado em estruturas de comando
#include "ish.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// Verificando se é um comando interno (não gera processo filho)
static int is_internal(const char *name) {
    return (strcmp(name, "cd") == 0 ||
            strcmp(name, "wait") == 0 ||
            strcmp(name, "exit") == 0);
}


//  Libera memória de um comando e seus argumentos
static void free_command(Command *cmd) {
    if (cmd == NULL) {
        return;
    }
    
    for (int i = 0; i < MAX_ARGS + 1; i++) {
        if (cmd->args[i] != NULL) {
            free(cmd->args[i]);
            cmd->args[i] = NULL;
        }
    }
    cmd->name = NULL;
    free(cmd);
}


// Auxiliar do parsing, remove espaços em branco nas strings
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


static void break_line(char *line, Command *cmd) {
    int i = 0;
    
    // Divide a string no primeiro espaco
    char *token = strtok(line, " \n");
    
    while (token != NULL && i < MAX_ARGS + 1) {
        // Aloca memoria nova e copia o token
        cmd->args[i] = strdup(token);
        i++;
        
        // Pega o proximo pedaco da string
        token = strtok(NULL, " \n");
    }
    
    // Garante que a lista de argumentos termine com NULL para o exec funcionar
    cmd->args[i] = NULL;
    
    // Ajusta o nome do comando, pra facilitar uma busca
    if (i > 0) 
        cmd->name = cmd->args[0];
    
}


static int validate_command(Command *cmd) {
    return cmd != NULL && cmd->name != NULL;
}


static int validate_pipe_command(CommandLine *c) {
    // Validação do primeiro comando
    if (!validate_command(c->cmd1)) {
        return 0;
    }
    
    if (c->has_pipe) {
        // Valida o segundo comando
        if (!validate_command(c->cmd2)) {
            return 0;
        }
        // Se um dos dois comandos forem internos, retorna 0
        if (is_internal(c->cmd1->name) || is_internal(c->cmd2->name)) {
            return 0;
        }
    }
    
    return 1;
}


// Aloca memória para um novo comando
static Command *create_command() {
    Command *cmd = (Command *)malloc(sizeof(Command));
    if (cmd != NULL) {
        memset(cmd, 0, sizeof(Command));
    }
    return cmd;
}

// Prepara uma estrutura para um comando simples sem pipe
static int parse_simple_command(char *line, CommandLine *c) {
    c->has_pipe = 0;
    c->cmd1 = create_command();
    c->cmd2 = NULL;
    
    if (c->cmd1 == NULL) {
        return 0;
    }
    
    break_line(line, c->cmd1);
    return 1;
}


static int parse_pipe_command(char *line, char *pipe_pos, CommandLine *c) {
    // Configura a flag de pipe e separa a string original em duas partes
    c->has_pipe = 1;
    *pipe_pos = '\0';   // Corta a string no local do '#'
    
    // Remove espaços desnecessários do primeiro comando e do segundo
    char *part1 = trim_spaces(line);
    char *part2 = trim_spaces(pipe_pos + 1);
    
    c->cmd1 = create_command();
    c->cmd2 = create_command();
    
    if (c->cmd1 == NULL || c->cmd2 == NULL) {
        free_command(c->cmd1);
        free_command(c->cmd2);
        return 0;
    }
    
    // Preenche os argumentos de ambos os comandos separadamente
    break_line(part1, c->cmd1);
    break_line(part2, c->cmd2);
    
    return 1;
}


static int parse_line(char *line, CommandLine *c) {
    char *pipe = strchr(line, '#');
    
    // Verifica se tem mais de um pipe
    if (pipe != NULL && strchr(pipe + 1, '#') != NULL) {
        return 0;
    }
    
    // Comando com pipe e comando sem pipe
    if (pipe != NULL) {
        return parse_pipe_command(line, pipe, c);
    } else {
        return parse_simple_command(line, c);
    }
}

static char *read_input_line() {
    char *line = NULL;
    size_t size = 0;
    
    // Lê uma linha inteira do terminal
    int ret = getline(&line, &size, stdin);
    if (ret == -1) {
        free(line);
        return NULL;
    }
    
    // Remove o \n
    line[strcspn(line, "\n")] = 0;
    return line;
}

// Descarta o que o usuário digitar se o buffer da shell já estiver cheio
static void discard_overflow_input() {
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF);
}

void read_line(IshState *lasy) {
    // Se atingiu o limite de 5 comandos, ignora novas entradas
    if (lasy->count >= MAX_BUFFER) {
        discard_overflow_input();
        return;
    }
    
    // Leitura da linha
    char *line = read_input_line();
    if (line == NULL) {
        if (!lasy->interactive) {
            exit(0);
        }
        return;
    }
    
    // Remove espaços desnecessários
    char *clean = trim_spaces(line);
    if (clean[0] == '\0') {
        free(line);
        return;
    }
    
    // Verifica se existe o pipe # e faz o parsing
    CommandLine c; //faz parsing
    if (!parse_line(clean, &c)) {
        free(line);
        return;
    }
    
    free(line);
    
    // Se não for valido, ignora
    if (!validate_pipe_command(&c)) {
        free_command(c.cmd1);
        free_command(c.cmd2);
        return;
    }
    
    // Se for valido, adiciona ao buffer
    lasy->buffer[lasy->count] = c;
    lasy->count++;
}

void delete_buffer(IshState *lasy) {
    // Limpa todos os comandos processados do buffer para recomeçar o ciclo
    for (int i = 0; i < lasy->count; i++) {
        free_command(lasy->buffer[i].cmd1);
        if (lasy->buffer[i].has_pipe) {
            free_command(lasy->buffer[i].cmd2);
        }
        lasy->buffer[i].cmd1 = NULL;
        lasy->buffer[i].cmd2 = NULL;
        lasy->buffer[i].has_pipe = 0;
    }
}
