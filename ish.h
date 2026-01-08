#if !defined(IO_H_)
#define IO_H_

#include <time.h>
#include <sys/types.h>

#define MAX_BUFFER 5
#define MAX_ARGS 4
#define MAX_CHILDREN 128

typedef struct {
    char * name;                    // executavel do comando argumento 0
    char * args[MAX_ARGS + 1];      // argumentos 0 a 3 e null final para execvp
}Command;

typedef struct {
    int has_pipe;
    Command *cmd1;
    Command *cmd2;
}CommandLine;

typedef struct {
    CommandLine buffer[MAX_BUFFER]; // buffer de comandos
    int count;                      // quantidade de comandos
    time_t last_ctrl_c;             // marca de tempo do ultimo ctrl-c
    int has_children;               // ha filhos vivos
    int interactive;               // 1 se stdin e terminal
    pid_t children[MAX_CHILDREN];
    int child_count;
    pid_t zombie_pids[MAX_CHILDREN];
    int zombie_status[MAX_CHILDREN];
    int zombie_count;
} IshState;

IshState * construct();
void read_line(IshState *);
void handle_sigint_ctrl_c(int , IshState*);
void handle_sigchld(int, IshState*);
void delete_buffer(IshState *);
#endif
