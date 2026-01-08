#include "ish.h"
#include "exec.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

// trata ctrl-c da shell
void handle_sigint_ctrl_c(int sig, IshState *lasy)
{
    time_t now = time(NULL);
    double diff = difftime(now, lasy->last_ctrl_c);
    int in_window = (lasy->last_ctrl_c != 0 && diff <= 3.0);

    if (lasy->count > 0) {
        execute_buffer(lasy);
        delete_buffer(lasy);
        lasy->count = 0;
        lasy->last_ctrl_c = 0;
    }
    else if (lasy->has_children) {
        if (in_window) {
            printf("Ok... você venceu! Adeus!\n");
            exit(0);
        }
        printf("Não posso morrer... sou lenta mas sou mãe de família!!!\n");
        lasy->last_ctrl_c = now;
    }
    else {
        printf("Ok... você venceu! Adeus!\n");
        exit(0);
    }

    fflush(stdout);
}

// guarda status de filhos mortos para o comando wait
void handle_sigchld(int sig, IshState *lasy)
{
    int status = 0;
    pid_t pid = waitpid(-1, &status, WNOHANG);
    while (pid > 0) {
        if (lasy->zombie_count < MAX_CHILDREN) {
            lasy->zombie_pids[lasy->zombie_count] = pid;
            lasy->zombie_status[lasy->zombie_count] = status;
            lasy->zombie_count++;
        }
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
