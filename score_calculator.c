#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_USER 100
#define USERNAME_LEN 32
#define CLUE_LEN 128

typedef struct {
    int id;
    char username[USERNAME_LEN];
    float latitude;
    float longitude;
    char clue[CLUE_LEN];
    int value;
} Treasure;

typedef struct {
    char username[USERNAME_LEN];
    int score;
} UserScore;

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <hunt_id>\n", argv[0]);
        return 1;
    }

    char path[256];
    snprintf(path, sizeof(path), "%s/treasures.dat", argv[1]);

    FILE* f = fopen(path, "rb");
    if (!f) {
        perror("fopen");
        return 1;
    }

    Treasure t;
    UserScore users[MAX_USER];
    int user_count = 0;

    while (fread(&t, sizeof(Treasure), 1, f) == 1) {
        int found = 0;
        for (int i = 0; i < user_count; ++i) {
            if (strcmp(users[i].username, t.username) == 0) {
                users[i].score += t.value;
                found = 1;
                break;
            }
        }
        if (!found && user_count < MAX_USER) {
            strncpy(users[user_count].username, t.username, USERNAME_LEN);
            users[user_count].score = t.value;
            user_count++;
        }
    }

    fclose(f);

    printf("%s:\n", argv[1]);
    for (int i = 0; i < user_count; ++i) {
        printf("  %s: %d\n", users[i].username, users[i].score);
    }

    return 0;
}
