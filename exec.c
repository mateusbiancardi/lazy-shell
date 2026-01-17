// executa comandos internos e externos, gerencia filhos e grupos de processos
#include "exec.h"
#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>

// adiciona pid a lista de filhos vivos
static void add_child(IshState *lasy, pid_t pid)
{
    if (pid <= 0)
    {
        return;
    }
    if (lasy->child_count < MAX_CHILDREN)
    {
        lasy->children[lasy->child_count++] = pid;
    }
    lasy->has_children = 1;
}

void remove_child(IshState *lasy, pid_t pid)
{
    for (int i = 0; i < lasy->child_count; i++)
    {
        if (lasy->children[i] == pid)
        {
            lasy->children[i] = lasy->children[lasy->child_count - 1];
            lasy->child_count--;
            break;
        }
    }
    if (lasy->child_count == 0)
    {
        lasy->has_children = 0;
    }
}

// impressao do status ao termino de um filho
static void report_child_status(pid_t pid, int status)
{
    if (WIFEXITED(status))
    {
        printf("Processo %d terminou com status %d\n",
               pid, WEXITSTATUS(status));
        return;
    }
    if (WIFSIGNALED(status))
    {
        printf("Processo %d terminou por sinal %d\n",
               pid, WTERMSIG(status));
        return;
    }
    if (WIFSTOPPED(status))
    {
        printf("Processo %d parou por sinal %d\n",
               pid, WSTOPSIG(status));
        return;
    }
    printf("Processo %d terminou por causa desconhecida\n", pid);
}

// comandos internos
static void reap_zombies(IshState *lasy)
{
    if (lasy->zombie_count == 0)
    {
        printf("Não há processos zumbis.\n");
        return;
    }

    // remove todos os zombies e reporta o status
    for (int i = 0; i < lasy->zombie_count; i++)
    {
        report_child_status(lasy->zombie_pids[i],
                            lasy->zombie_status[i]);
        remove_child(lasy, lasy->zombie_pids[i]);
    }
    lasy->zombie_count = 0;
}
// comandos internos
static void collect_process_groups(IshState *lasy, pid_t *groups,
                                   int *group_count)
{
    *group_count = 0;

    for (int i = 0; i < lasy->child_count; i++)
    {
        pid_t pid = lasy->children[i];
        // pega grupo do processo
        pid_t pgid = getpgid(pid);

        // se deu erro, mata o processo
        if (pgid < 0)
        {
            kill(pid, SIGTERM);
            continue;
        }

        // verifica grupo esta na lista
        int exists = 0;
        for (int g = 0; g < *group_count; g++)
        {
            if (groups[g] == pgid)
            {
                exists = 1;
                break;
            }
        }

        // se o grupo não estiver adiciona
        // evita enviar sinal 2x pro mesmo grupo
        if (!exists && *group_count < MAX_CHILDREN)
        {
            groups[(*group_count)++] = pgid;
            // mata todos do grupo
            kill(-pgid, SIGTERM);
        }
    }
}

static void wait_all_children(IshState *lasy)
{
    int status;
    pid_t pid;

    // espera qualquer filho
    while ((pid = waitpid(-1, &status, 0)) > 0)
    {
        // reporta como esse filho terminou
        report_child_status(pid, status);
    }

    lasy->child_count = 0;
    lasy->has_children = 0;
}

static void exit_shell(IshState *lasy)
{
    pid_t groups[MAX_CHILDREN];
    int group_count;

    collect_process_groups(lasy, groups, &group_count);
    wait_all_children(lasy);

    exit(0);
}

static int handle_cd(Command *cmd)
{
    char *path = cmd->args[1];

    if (path == NULL)
    {
        printf("cd: caminho ausente\n");
        return 1;
    }
    if (cmd->args[2] != NULL)
    {
        printf("cd: muitos argumentos\n");
        return 1;
    }
    if (chdir(path) != 0)
    {
        perror("cd");
        return 1;
    }
    return 0;
}

static int handle_wait(Command *cmd, IshState *lasy)
{
    if (cmd->args[1] != NULL)
    {
        printf("wait: não aceita argumentos\n");
        return 1;
    }
    reap_zombies(lasy);
    return 0;
}

static int handle_exit(Command *cmd, IshState *lasy)
{
    if (cmd->args[1] != NULL)
    {
        printf("exit: não aceita argumentos\n");
        return 1;
    }
    exit_shell(lasy);
    return 0;
}

// cria processo filho, configura grupo, redireciona e executa comando
static void setup_child_process(pid_t group_id, int in_fd, int out_fd,
                                int close_fd)
{
    if (group_id < 0)
    {
        setpgid(0, 0);
    }
    else
    {
        setpgid(0, group_id);
    }
    // ignora sinais no filho para nao morrer com ctrl-c
    signal(SIGINT, SIG_IGN);
    // fecha fd não usado
    if (close_fd >= 0)
    {
        close(close_fd);
    }
    // redireciona entrada/saida
    if (in_fd >= 0)
    {
        dup2(in_fd, STDIN_FILENO);
    }
    if (out_fd >= 0)
    {
        dup2(out_fd, STDOUT_FILENO);
    }

    // fecha fds se foram redirecionados
    if (in_fd > STDERR_FILENO)
    {
        close(in_fd);
    }
    if (out_fd > STDERR_FILENO)
    {
        close(out_fd);
    }
}

static pid_t fork_exec(Command *cmd, pid_t group_id, int in_fd,
                       int out_fd, int close_fd)
{
    pid_t pid = fork();

    if (pid == 0)
    { // cria filho e faz exec
        setup_child_process(group_id, in_fd, out_fd, close_fd);
        execvp(cmd->name, cmd->args);
        perror("execvp");
        _exit(127);
    }

    return pid;
}

static int execute_internal_command(CommandLine cmd, IshState *lasy)
{
    char *prog = cmd.cmd1->name;

    if (strcmp(prog, "cd") == 0)
    {
        return handle_cd(cmd.cmd1);
    }
    if (strcmp(prog, "exit") == 0)
    {
        return handle_exit(cmd.cmd1, lasy);
    }
    if (strcmp(prog, "wait") == 0)
    {
        return handle_wait(cmd.cmd1, lasy);
    }

    return -1; // nao e comando interno
}

//cria processos para comandos e escreve no pipe
static void execute_pipe_command(CommandLine cmd, IshState *lasy,
                                 pid_t *group_id)
{
    int fd[2];

    if (pipe(fd) != 0)
    {
        perror("pipe");
        return;
    }

    // cria primeiro processo - escreve no pipe
    pid_t pid1 = fork_exec(cmd.cmd1, *group_id, -1, fd[1], fd[0]);
    if (pid1 < 0)
    {
        perror("fork");
        close(fd[0]);
        close(fd[1]);
        return;
    }

    if (*group_id < 0)
    {
        *group_id = pid1;
    }

    // cria segundo processo - le do pipe
    pid_t pid2 = fork_exec(cmd.cmd2, *group_id, fd[0], -1, fd[1]);
    if (pid2 < 0)
    {
        perror("fork");
        close(fd[0]);
        close(fd[1]);
        return;
    }

    close(fd[0]);
    close(fd[1]);

    add_child(lasy, pid1);
    add_child(lasy, pid2);
}

static void execute_simple_command(CommandLine cmd, IshState *lasy,
                                   pid_t *group_id)
{
    pid_t pid = fork_exec(cmd.cmd1, *group_id, -1, -1, -1);

    if (pid < 0)
    {
        perror("fork");
        return;
    }

    if (*group_id < 0)
    {
        *group_id = pid;
    }

    add_child(lasy, pid);
}

void execute_command(CommandLine cmd, IshState *lasy, pid_t *group_id)
{
    // processos internos
    if (!cmd.has_pipe)
    {
        int result = execute_internal_command(cmd, lasy);
        if (result >= 0)
        {
            return;
        }
    }

    // processos externos
    if (cmd.has_pipe)
    {
        execute_pipe_command(cmd, lasy, group_id);
    }
    else
    {
        execute_simple_command(cmd, lasy, group_id);
    }
}

void execute_buffer(IshState *lasy)
{
    pid_t group_id = -1;

    for (int i = 0; i < lasy->count; i++)
    {
        execute_command(lasy->buffer[i], lasy, &group_id);
    }
}
