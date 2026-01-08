#ifndef ISH_H
#define ISH_H

#include <time.h>
#include <sys/types.h>

#define MAX_BUFFER 5
#define MAX_ARGS 4
#define MAX_CHILDREN 128
//representa um comando, com seus nome, args e exec
typedef struct {
    char *name;               // args[0]
    char *args[MAX_ARGS + 1]; // argv[0-3] + NULL
} Command;

typedef struct {
    int has_pipe;
    Command *cmd1;
    Command *cmd2;
} CommandLine;

typedef struct {
    CommandLine buffer[MAX_BUFFER]; //filas de comandos
    int count; //qtd no buffer
    time_t last_ctrl_c; // timestamp do ultimo ctrl-c
    int has_children;  // flag de filhos vivos
    int interactive; // rodar o script de testes
    pid_t children[MAX_CHILDREN];  // pids dos filhos vivos
    int child_count; //qtd de filhos
    pid_t zombie_pids[MAX_CHILDREN]; 
    int zombie_status[MAX_CHILDREN];
    int zombie_count;
} IshState;

IshState *construct(void);
void read_line(IshState *);
void handle_sigint_ctrl_c(int, IshState *);
void handle_sigchld(int, IshState *);
void delete_buffer(IshState *);

#endif
