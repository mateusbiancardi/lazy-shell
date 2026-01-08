//trata sinais SIGINT (Ctrl-C) e SIGCHLD (zombies)
#include "ish.h"
#include "exec.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

static int check_double_ctrl_c(IshState *lasy) {
    // calculando se ctrl-c foi apertado 2 vezes com ate 3s de diferenca
    time_t now = time(NULL);
    double diff = difftime(now, lasy->last_ctrl_c);
    return (lasy->last_ctrl_c != 0 && diff <= 3.0);
}

static void handle_buffer_execution(IshState *lasy) {
    execute_buffer(lasy);
    delete_buffer(lasy);
    lasy->count = 0;
    lasy->last_ctrl_c = 0;
}

static void handle_children_alive(IshState *lasy) {
    if (check_double_ctrl_c(lasy)) {
        printf("Ok... você venceu! Adeus!\n");
        exit(0);
    }
    printf("Não posso morrer... sou lenta mas sou mãe de família!!!\n");
    lasy->last_ctrl_c = time(NULL);
}

static void handle_no_children(void) {
    printf("Ok... você venceu! Adeus!\n");
    exit(0);
}

void handle_sigint_ctrl_c(int sig, IshState *lasy) {
    // ctrl-c executa buffer, avisa ou encerra conforme estado
    if (lasy->count > 0) {
        handle_buffer_execution(lasy);
    } else if (lasy->has_children) {
        handle_children_alive(lasy);
    } else {
        handle_no_children();
    }
    
    fflush(stdout);
}

static void store_zombie(IshState *lasy, pid_t pid, int status) {
    if (lasy->zombie_count < MAX_CHILDREN) {
        lasy->zombie_pids[lasy->zombie_count] = pid;
        lasy->zombie_status[lasy->zombie_count] = status;
        lasy->zombie_count++;
    }
}

static void remove_from_children(IshState *lasy, pid_t pid) {
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

void handle_sigchld(int sig, IshState *lasy) {
    // guarda zombies para o comando wait
    int status;
    pid_t pid;
    
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        store_zombie(lasy, pid, status);
        remove_from_children(lasy, pid);
    }
}
