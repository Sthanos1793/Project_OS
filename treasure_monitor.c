#define _XOPEN_SOURCE 700
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/types.h>

#define BUFFER_SIZE 256

void list_treasures(const char* hunt_id);
void view_treasure(const char* hunt_id, int treasure_id);


void handle_sigusr1(int sig) {
    FILE* f = fopen("/tmp/hub_request.txt", "r");
    if (!f) {
        perror("fopen request");
        return;
    }

    char command[BUFFER_SIZE];
    if (fgets(command, sizeof(command), f)) {
        command[strcspn(command, "\n")] = 0; // remove newline
        printf("[Monitor] Raw command: '%s'\n", command);


        if (strcmp(command, "LIST_HUNTS") == 0) {
            printf("[Monitor] Listing hunts...\n");

            DIR* dir = opendir(".");
            if(!dir){

                perror("[Monitor] opendir");
                return;

            }

            struct dirent* entry;

            while ((entry = readdir(dir)) != NULL){
                
                if (strncmp(entry->d_name, "Hunt", 4) == 0) {

                    struct stat st;
                    if (stat(entry->d_name, &st) == 0 && S_ISDIR(st.st_mode)) {

                        char filepath[256];
                        snprintf(filepath, sizeof(filepath), "%s/treasures.dat", entry->d_name);
                        if(stat(filepath, &st) == 0) {

                            printf("%s: %ld bytes\n", entry->d_name, st.st_size);

                        }

                        else{

                            printf("%s: o bytes (no treasures.dat)\n", entry->d_name);

                        }

                    }  

                }

            }
            
            closedir(dir);
            return;

        } 
        
        if (strncmp(command, "LIST_TREASURES ", 15) == 0) {
            const char* hunt_id = command + 15;
            printf("[Monitor] Received LIST_TREASURES for hunt: %s\n", hunt_id);
            list_treasures(hunt_id);
            return;
        }

        else if (strncmp(command, "VIEW_TREASURE ", 14) == 0) {
            char hunt_id[64];
            int treasure_id;
            if (sscanf(command + 14, "%s %d", hunt_id, &treasure_id) == 2) {
                printf("[Monitor] Received VIEW_TREASURE for hunt: '%s', id: %d\n", hunt_id, treasure_id);
                view_treasure(hunt_id, treasure_id);
            } 
            
            else {
                printf("[Monitor] Invalid VIEW_TREASURE command.\n");
            }
            return; 
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
    usleep(500000); 

    FILE* clear = fopen("/tmp/hub_request.txt", "w");
    if (clear) fclose(clear);
}

void list_treasures(const char* hunt_id) {
    char path[BUFFER_SIZE];
    snprintf(path, sizeof(path), "%s/treasures.dat", hunt_id);

    printf("[Monitor] Trying to open file: %s/treasures.dat\n", hunt_id);

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

    printf("[Monitor] Trying to open file: %s\n", path);

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

    setvbuf(stdout, NULL, _IONBF, 0);

    char cwd[256];
    getcwd(cwd, sizeof(cwd));
    printf("[Monitor] Running in: %s\n", cwd);

    struct sigaction sa;
    sa.sa_handler = handle_sigusr1;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGUSR1, &sa, NULL);

    printf("[Monitor] Ready (PID %d). Waiting for commands...\n", getpid());

    while (1) pause();
    return 0;
}
