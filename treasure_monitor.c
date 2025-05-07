#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#define BUFFER_SIZE 256

void handle_sigusr1(int sig) {
    FILE* f = fopen("/tmp/hub_request.txt", "r");
    if (!f) {
        perror("fopen request");
        return;
    }

    char command[BUFFER_SIZE];
    if (fgets(command, sizeof(command), f)) {
        command[strcspn(command, "\n")] = 0; // remove newline

        if (strcmp(command, "LIST_HUNTS") == 0) {
            printf("[Monitor] Listing hunts...\n");

            // For now, just simulate a response
            system("ls -d */ | grep -E '^Hunt' | xargs -I{} bash -c 'echo -n \"{}: \"; stat -c%s {}/treasures.dat 2>/dev/null || echo 0'");
        } 
        
        if (strncmp(command, "LIST_TREASURES ", 15) == 0) {
            char* hunt_id = command + 15;
            list_treasures(hunt_id);
        }

        else if (strncmp(command, "VIEW_TREASURE ", 14) == 0) {
            char hunt_id[64];
            int treasure_id;
            if (sscanf(command + 14, "%s %d", hunt_id, &treasure_id) == 2) {
                view_treasure(hunt_id, treasure_id);
            } else {
                printf("[Monitor] Invalid VIEW_TREASURE command.\n");
            }
        }

        else if (strcmp(command, "STOP_MONITOR") == 0) {
            printf("[Monitor] Stopping after delay...\n");
            usleep(500000); // Delay for 0.5 seconds
            exit(0);
        }
    
            
        else {
            printf("[Monitor] Unknown request: %s\n", command);
        }
    }

    fclose(f);
    usleep(500000); // Simulate delay
}

void list_treasures(const char* hunt_id) {
    char path[BUFFER_SIZE];
    snprintf(path, sizeof(path), "%s/treasures.dat", hunt_id);

    FILE* f = fopen(path, "rb");
    if (!f) {
        perror("[Monitor] Cannot open treasure file");
        return;
    }

    typedef struct {
        int id;
        char username[32];
        float latitude;
        float longitude;
        char clue[128];
        int value;
    } Treasure;

    Treasure t;
    printf("[Monitor] Treasures in %s:\n", hunt_id);
    while (fread(&t, sizeof(Treasure), 1, f) == 1) {
        printf("  ID: %d | User: %s | (%.4f, %.4f) | Value: %d\n", 
               t.id, t.username, t.latitude, t.longitude, t.value);
        printf("  Clue: %s\n", t.clue);
    }

    fclose(f);
}

void view_treasure(const char* hunt_id, int treasure_id) {
    char path[BUFFER_SIZE];
    snprintf(path, sizeof(path), "%s/treasures.dat", hunt_id);

    FILE* f = fopen(path, "rb");
    if (!f) {
        perror("[Monitor] Cannot open treasure file");
        return;
    }

    typedef struct {
        int id;
        char username[32];
        float latitude;
        float longitude;
        char clue[128];
        int value;
    } Treasure;

    Treasure t;
    int found = 0;

    while (fread(&t, sizeof(Treasure), 1, f) == 1) {
        if (t.id == treasure_id) {
            printf("[Monitor] Treasure found in %s:\n", hunt_id);
            printf("  ID: %d\n", t.id);
            printf("  User: %s\n", t.username);
            printf("  Location: (%.4f, %.4f)\n", t.latitude, t.longitude);
            printf("  Clue: %s\n", t.clue);
            printf("  Value: %d\n", t.value);
            found = 1;
            break;
        }
    }

    if (!found) {
        printf("[Monitor] Treasure ID %d not found in %s.\n", treasure_id, hunt_id);
    }

    fclose(f);
}

int main() {
    struct sigaction sa;
    sa.sa_handler = handle_sigusr1;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGUSR1, &sa, NULL);

    printf("[Monitor] Ready (PID %d). Waiting for commands...\n", getpid());

    while (1) pause();
    return 0;
}
