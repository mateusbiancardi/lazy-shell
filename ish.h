#if !defined(IO_H_)
#define IO_H_

#include <time.h>
#include <sys/types.h>

#define MAX_BUFFER 5
#define MAX_ARGS 3
#define MAX_CHILDREN 128

typedef struct {
    char *name;                     // referencia do executavel
    char *args[MAX_ARGS + 2];       // programa + ate 3 args + NULL
} Command;

typedef struct {
    int has_pipe;
    Command *cmd1;
    Command *cmd2;
} CommandLine;

typedef struct {
    CommandLine buffer[MAX_BUFFER]; // armazena todos os comandos
    int count;                      // quantidade de comandos
    time_t last_ctrl_c;             // variavel de tempo pro Ctrl-c
    int has_children;               // se tem algum filho rodando
    int interactive;                // modo interativo
    pid_t children[MAX_CHILDREN];   // pids dos filhos vivos
    int child_count;                // quantidade de filhos vivos
    pid_t zombie_pids[MAX_CHILDREN];
    int zombie_status[MAX_CHILDREN];
    int zombie_count;
} IshState;

IshState *construct();
void read_line(IshState *);
void handle_sigint_ctrl_c(int, IshState *);
void handle_sigchld(int, IshState *);
void delete_buffer(IshState *);

#endif // IO_H_
