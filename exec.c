#include "exec.h"
#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

static void add_child(IshState *lasy, pid_t pid) {
    if (pid <= 0) {
        return;
    }
    if (lasy->child_count < MAX_CHILDREN) {
        lasy->children[lasy->child_count++] = pid;
    }
    lasy->has_children = 1;
}

static void remove_child(IshState *lasy, pid_t pid) {
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
}

static void report_child_status(pid_t pid, int status) {
    if (WIFEXITED(status)) {
        printf("Processo %d terminou com status %d\n", pid, WEXITSTATUS(status));
        return;
    }
    if (WIFSIGNALED(status)) {
        printf("Processo %d terminou por sinal %d\n", pid, WTERMSIG(status));
        return;
    }
    if (WIFSTOPPED(status)) {
        printf("Processo %d parou por sinal %d\n", pid, WSTOPSIG(status));
        return;
    }
    printf("Processo %d terminou por causa desconhecida\n", pid);
}

static void reap_zombies(IshState *lasy) {
    int status = 0;
    int found = 0;
    pid_t pid = waitpid(-1, &status, WNOHANG);
    while (pid > 0) {
        report_child_status(pid, status);
        remove_child(lasy, pid);
        found = 1;
        pid = waitpid(-1, &status, WNOHANG);
    }
    if (!found) {
        printf("Nao ha processos zumbis.\n");
    }
}

static void exit_shell(IshState *lasy) {
    pid_t groups[MAX_CHILDREN];
    int group_count = 0;

    for (int i = 0; i < lasy->child_count; i++) {
        pid_t pid = lasy->children[i];
        pid_t pgid = getpgid(pid);
        if (pgid < 0) {
            kill(pid, SIGTERM);
            continue;
        }
        int exists = 0;
        for (int g = 0; g < group_count; g++) {
            if (groups[g] == pgid) {
                exists = 1;
                break;
            }
        }
        if (!exists && group_count < MAX_CHILDREN) {
            groups[group_count++] = pgid;
            // mata o grupo inteiro para cobrir filhos indiretos
            kill(-pgid, SIGTERM);
        }
    }
    int status = 0;
    pid_t pid = waitpid(-1, &status, 0);
    while (pid > 0) {
        report_child_status(pid, status);
        pid = waitpid(-1, &status, 0);
    }
    lasy->child_count = 0;
    lasy->has_children = 0;
    exit(0);
}

static pid_t fork_exec(Command *cmd, pid_t group_id, int in_fd, int out_fd) {
    pid_t pid = fork();
    if (pid == 0) {
        if (group_id < 0) {
            setpgid(0, 0);
        } else {
            setpgid(0, group_id);
        }
        signal(SIGINT, SIG_IGN);

        if (in_fd >= 0) {
            dup2(in_fd, STDIN_FILENO);
        }
        if (out_fd >= 0) {
            dup2(out_fd, STDOUT_FILENO);
        }
        if (in_fd > STDERR_FILENO) {
            close(in_fd);
        }
        if (out_fd > STDERR_FILENO) {
            close(out_fd);
        }

        execvp(cmd->name, cmd->args);
        perror("execvp");
        _exit(127);
    }
    return pid;
}

void execute_command(CommandLine cmd, IshState *lasy, pid_t *group_id)
{   
    // processos internos
    if (cmd.has_pipe == 0) {
        char *prog = cmd.cmd1->name;
        
        if (strcmp(prog, "cd") == 0) {
            char *path = cmd.cmd1->args[1];
            if (path == NULL) {
                printf("cd: caminho ausente\n");
                return;
            }
            if (cmd.cmd1->args[2] != NULL) {
                printf("cd: muitos argumentos\n");
                return;
            }
            if (chdir(path) != 0) {
                perror("cd");
            }
            return;
        }
        if (strcmp(prog, "exit") == 0) {
            if (cmd.cmd1->args[1] != NULL) {
                printf("exit: nao aceita argumentos\n");
                return;
            }
            exit_shell(lasy);
            return;
        }
        if (strcmp(prog, "wait") == 0) {
            if (cmd.cmd1->args[1] != NULL) {
                printf("wait: nao aceita argumentos\n");
                return;
            }
            reap_zombies(lasy);
            return;
        }
    }

    // processos externos
    if (cmd.has_pipe == 1) {
        int fd[2];
        if (pipe(fd) != 0) {
            perror("pipe");
            return;
        }

        pid_t pid1 = fork_exec(cmd.cmd1, *group_id, -1, fd[1]);
        if (pid1 < 0) {
            perror("fork");
            close(fd[0]);
            close(fd[1]);
            return;
        }
        if (*group_id < 0) {
            *group_id = pid1;
        }

        pid_t pid2 = fork_exec(cmd.cmd2, *group_id, fd[0], -1);
        if (pid2 < 0) {
            perror("fork");
            close(fd[0]);
            close(fd[1]);
            return;
        }

        close(fd[0]);
        close(fd[1]);

        add_child(lasy, pid1);
        add_child(lasy, pid2);
        return;
    }

    pid_t pid = fork_exec(cmd.cmd1, *group_id, -1, -1);
    if (pid < 0) {
        perror("fork");
        return;
    }
    if (*group_id < 0) {
        *group_id = pid;
    }
    add_child(lasy, pid);
}

void execute_buffer(IshState *lasy)
{
    pid_t group_id = -1;
    for (int i = 0; i < lasy->count; i++) {
        execute_command(lasy->buffer[i], lasy, &group_id);
    }
}
