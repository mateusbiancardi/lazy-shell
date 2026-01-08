//trata sinais SIGINT (Ctrl-C) e SIGCHLD (zombies)
#include "ish.h"
#include "exec.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

static int check_double_ctrl_c(IshState *state) {
    time_t now = time(NULL);
    double diff = difftime(now, state->last_ctrl_c);
    return (state->last_ctrl_c != 0 && diff <= 3.0);
}

static void handle_buffer_execution(IshState *state) {
    exec_buffer(state);
    delete_buffer(state);
    state->count = 0;
    state->last_ctrl_c = 0;
}

static void handle_children_alive(IshState *state) {
    if (check_double_ctrl_c(state)) {
        printf("Ok... você venceu! Adeus!\n");
        exit(0);
    }
    printf("Não posso morrer... sou lenta mas sou mãe de família!!!\n");
    state->last_ctrl_c = time(NULL);
}

static void handle_no_children(void) {
    printf("Ok... você venceu! Adeus!\n");
    exit(0);
}

void handle_sigint_ctrl_c(int sig, IshState *state) {
    // ctrl-c executa buffer, avisa ou encerra conforme estado
    if (state->count > 0) {
        handle_buffer_execution(state);
    } else if (state->has_children) {
        handle_children_alive(state);
    } else {
        handle_no_children();
    }
    
    fflush(stdout);
}

static void store_zombie(IshState *state, pid_t pid, int status) {
    if (state->zombie_count < MAX_CHILDREN) {
        state->zombie_pids[state->zombie_count] = pid;
        state->zombie_status[state->zombie_count] = status;
        state->zombie_count++;
    }
}

static void remove_from_children(IshState *state, pid_t pid) {
    for (int i = 0; i < state->child_count; i++) {
        if (state->children[i] == pid) {
            state->children[i] = state->children[state->child_count - 1];
            state->child_count--;
            break;
        }
    }
    
    if (state->child_count == 0) {
        state->has_children = 0;
    }
}

void handle_sigchld(int sig, IshState *state) {
    // guarda zombies para o comando wait
    int status;
    pid_t pid;
    
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        store_zombie(state, pid, status);
        remove_from_children(state, pid);
    }
}
