#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

#define CMD_MAX 128
#define _XOPEN_SOURCE 700

pid_t monitor_pid = -1;
int monitor_running = 0;

// Handle SIGCHLD when monitor exits
void handle_sigchld(int sig) {
    int status;
    pid_t pid = waitpid(monitor_pid, &status, WNOHANG);
    if (pid == monitor_pid) {
        printf("Monitor process exited with status %d\n", WEXITSTATUS(status));
        monitor_running = 0;
        monitor_pid = -1;
    }
}

void start_monitor() {
    if (monitor_running) {
        printf("Monitor is already running.\n");
        return;
    }

    monitor_pid = fork();
    if (monitor_pid == -1) {
        perror("fork");
        return;
    }

    if (monitor_pid == 0) {
        // Child: monitor process
        execl("./treasure_monitor", "./treasure_monitor", NULL);
        perror("execl");
        exit(1);
    }

    printf("Monitor started with PID %d\n", monitor_pid);
    monitor_running = 1;
}

void send_request_to_monitor(const char* command) {
    if (!monitor_running) {
        printf("Error: Monitor is not running.\n");
        return;
    }

    FILE* f = fopen("/tmp/hub_request.txt", "w");
    if (!f) {
        perror("fopen");
        return;
    }

    fprintf(f, "%s\n", command);
    fclose(f);

    if (kill(monitor_pid, SIGUSR1) == -1) {
        perror("kill");
    }
}


int main() {
    
    struct sigaction sa;
    sa.sa_handler = handle_sigchld;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &sa, NULL);

    char command[CMD_MAX];

    while (1) {
        printf("treasure_hub> ");
        fflush(stdout);

        if (!fgets(command, sizeof(command), stdin)) break;
        command[strcspn(command, "\n")] = 0; // strip newline

        if (strcmp(command, "start_monitor") == 0) {
            start_monitor();
        } 
        
        else if (strcmp(command, "list_hunts") == 0) {
            send_request_to_monitor("LIST_HUNTS");
        }
        
        else if (strncmp(command, "list_treasures ", 15) == 0) {
            char* hunt_id = command + 15;
            if (strlen(hunt_id) == 0) {
                printf("Usage: list_treasures <hunt_id>\n");
            } else {
                char request[CMD_MAX];
                snprintf(request, sizeof(request), "LIST_TREASURES %s", hunt_id);
                send_request_to_monitor(request);
            }
        }
        
        else if (strncmp(command, "view_treasure ", 14) == 0) {
            char hunt_id[64];
            int treasure_id;
            if (sscanf(command + 14, "%s %d", hunt_id, &treasure_id) != 2) {
                printf("Usage: view_treasure <hunt_id> <id>\n");
            } else {
                char request[CMD_MAX];
                snprintf(request, sizeof(request), "VIEW_TREASURE %s %d", hunt_id, treasure_id);
                send_request_to_monitor(request);
            }
        }
        
        else if (strcmp(command, "stop_monitor") == 0) {
            if (!monitor_running) {
                printf("Monitor is not running.\n");
            } else {
                send_request_to_monitor("STOP_MONITOR");
                printf("Requested monitor to stop. Waiting for termination...\n");
            }
        }
        
        
        else if (strcmp(command, "exit") == 0) {
            if (monitor_running) {
                printf("Error: Monitor is still running. Use stop_monitor first.\n");
            } else {
                printf("Exiting...\n");
                break;
            }
        } 

        
        
            else {
            printf("Unknown or unavailable command: %s\n", command);
        }
    }

    return 0;
}
