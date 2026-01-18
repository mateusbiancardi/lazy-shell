#if !defined(IO_H_)
#define IO_H_

#include <time.h>
#include <sys/types.h>

// Definições de limites da especificação do trabalho
#define MAX_BUFFER 5        // Quantos comandos são guardados no buffer
#define MAX_ARGS 3          // Limite de argumentos suportados pelo programa
#define MAX_CHILDREN 128    

typedef struct {
    char *name;                     // O nome do programa a ser executado
    char *args[MAX_ARGS + 2];       // Vetor para execução (nome + argumentos + NULL)
} Command;

// Estrutura para a linha completa digitada, lidando com o pipe '#'
typedef struct {
    int has_pipe;
    Command *cmd1;
    Command *cmd2;
} CommandLine;

// Estados da Shell
typedef struct {
    CommandLine buffer[MAX_BUFFER]; // Fila de armazenamento de todos os comandos
    int count;                      // Quantidade de comandos atuais na fila
    
    time_t last_ctrl_c;             // Marca o tempo pro Ctrl-c duplo

    int has_children;               // Se tem algum filho rodando
    int interactive;                // Modo interativo

    pid_t children[MAX_CHILDREN];   // Lista de processos ativos em background
    int child_count;                // Quantos filhos vivos

    // Dados de filhos mortos, para o comando wait
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
