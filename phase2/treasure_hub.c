#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>

pid_t monitor_pid = -1;
int pipefd[2]; // pipe for communication with monitor
int monitor_running = 0;

void start_monitor() {
    if (monitor_running) {
        printf("Monitor is already running.\n");
        return;
    }

    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(1);
    }

    monitor_pid = fork();

    if (monitor_pid == -1) {
        perror("fork");
        exit(1);
    }

    if (monitor_pid == 0) {
        // CHILD: Monitor process
        close(pipefd[1]); // close write end
        dup2(pipefd[0], STDIN_FILENO); // read commands from stdin
        execl("./monitor", "./monitor", NULL);
        perror("execl failed");
        exit(1);
    }

    close(pipefd[0]); // parent closes read end
    monitor_running = 1;
    printf("Monitor started with PID %d\n", monitor_pid);
}

void stop_monitor() {
    if (!monitor_running) {
        printf("No monitor running.\n");
        return;
    }

    kill(monitor_pid, SIGTERM);
    printf("Waiting for monitor to stop...\n");
    waitpid(monitor_pid, NULL, 0); // Wait for child
    monitor_running = 0;
    printf("Monitor has stopped.\n");
}

void send_command(const char *cmd) {
    if (!monitor_running) {
        printf("Monitor is not running.\n");
        return;
    }

    dprintf(pipefd[1], "%s\n", cmd); // write to monitor
}

int main() {
    char command[256];

    while (1) {
        printf("hub> ");
        fflush(stdout);
        if (!fgets(command, sizeof(command), stdin)) break;
        command[strcspn(command, "\n")] = 0;

        if (strcmp(command, "start_monitor") == 0) {
            start_monitor();
        } else if (strcmp(command, "stop_monitor") == 0) {
            stop_monitor();
        } else if (strcmp(command, "list_hunts") == 0) {
            send_command("LIST_HUNTS");
            kill(monitor_pid, SIGUSR1);
        } else if (strcmp(command, "list_treasures") == 0) {
            char hunt[100];
            printf("Hunt name: ");
            fgets(hunt, sizeof(hunt), stdin);
            hunt[strcspn(hunt, "\n")] = 0;

            char msg[200];
            snprintf(msg, sizeof(msg), "LIST_TREASURES %s", hunt);
            send_command(msg);
            kill(monitor_pid, SIGUSR2);
        } else if (strcmp(command, "view_treasure") == 0) {
            char hunt[100], tid[100];
            printf("Hunt name: ");
            fgets(hunt, sizeof(hunt), stdin);
            printf("Treasure ID: ");
            fgets(tid, sizeof(tid), stdin);
            hunt[strcspn(hunt, "\n")] = 0;
            tid[strcspn(tid, "\n")] = 0;

            char msg[300];
            snprintf(msg, sizeof(msg), "VIEW_TREASURE %s %s", hunt, tid);
            send_command(msg);
            kill(monitor_pid, SIGINT);
        } else if (strcmp(command, "exit") == 0) {
            if (monitor_running) {
                printf("Monitor is still running! Use stop_monitor first.\n");
                continue;
            }
            break;
        } else {
            printf("Unknown command\n");
        }
    }

    return 0;
}
