#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>

#define SHM_SIZE 1024

int shm_id;
void *shm_ptr;
pid_t child_pid;

void signal_handler(int sig) {
    if (sig == SIGUSR1) {
        int sum;
        memcpy(&sum, shm_ptr, sizeof(int));
        printf("Program 1 (parent): Sum is: %d\n", sum);
    }
}

void create_shared_memory() {
    shm_id = shmget(IPC_PRIVATE, SHM_SIZE, IPC_CREAT | 0666);
    shm_ptr = shmat(shm_id, NULL, 0);
    if (shm_ptr == (void *) -1) {
        perror("shmat");
        exit(1);
    }
}

void destroy_shared_memory() {
    shmdt(shm_ptr);
    shmctl(shm_id, IPC_RMID, NULL);
}

void run_child_process() {
    char shm_id_str[10];
    sprintf(shm_id_str, "%d", shm_id);
    execlp("./program2", "program2", shm_id_str, NULL);
    perror("execlp");
    exit(1);
}

void handle_parent_process() {
    while (1) {
        int n, i, input;
        printf("Program 1 (parent): Enter count of numbers to sum (0 - exit): ");
        scanf("%d", &n);
        if (n <= 0) {
            break;
        }
        for (i = 0; i < n; i++) {
            printf("Program 1 (parent): Input num: ");
            scanf("%d", &input);
            memcpy(shm_ptr + i * sizeof(int), &input, sizeof(int));
        }
        int end_marker = 0;
        memcpy(shm_ptr + n * sizeof(int), &end_marker, sizeof(int));
        kill(child_pid, SIGUSR1);
        pause();
    }
    kill(child_pid, SIGTERM);
    waitpid(child_pid, NULL, 0);
}

int main(int argc, char *argv[]) {
    create_shared_memory();
    signal(SIGUSR1, signal_handler);
    child_pid = fork();
    if (child_pid == 0) {
        run_child_process();
    } else {
        handle_parent_process();
        destroy_shared_memory();
    }
    return 0;
}
