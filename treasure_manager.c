#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <errno.h>
#include <dirent.h>


#define USERNAME_MAX 32
#define CLUE_MAX 128
#define BUFFER_SIZE 256

typedef struct {
    int id;
    char username[USERNAME_MAX];
    float latitude;
    float longitude;
    char clue[CLUE_MAX];
    int value;
} Treasure;

void log_operation(const char* hunt_dir, const char* message) {
    char path[BUFFER_SIZE];
    snprintf(path, sizeof(path), "%s/logged_hunt", hunt_dir);

    int fd = open(path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd == -1) {
        perror("log open");
        return;
    }

    time_t now = time(NULL);
    char log_entry[BUFFER_SIZE];
    snprintf(log_entry, sizeof(log_entry), "[%s] %s\n", ctime(&now), message);
    write(fd, log_entry, strlen(log_entry));
    close(fd);
}

void create_symlink(const char* hunt_id) {
    char target[BUFFER_SIZE];
    char linkname[BUFFER_SIZE];

    snprintf(target, sizeof(target), "%s/logged_hunt", hunt_id);
    snprintf(linkname, sizeof(linkname), "logged_hunt-%s", hunt_id);

    // Remove if exists
    unlink(linkname);
    if (symlink(target, linkname) == -1) {
        perror("symlink");
    }
}

void add_treasure(const char* hunt_id) {
    char dir_path[BUFFER_SIZE];
    snprintf(dir_path, sizeof(dir_path), "%s", hunt_id);
    mkdir(dir_path, 0755);

    char file_path[BUFFER_SIZE];
    snprintf(file_path, sizeof(file_path), "%s/treasures.dat", hunt_id);

    int fd = open(file_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd == -1) {
        perror("open treasure file");
        return;
    }

    Treasure t;
    printf("Enter treasure ID (int): ");
    scanf("%d", &t.id);
    printf("Enter username: ");
    scanf("%s", t.username);
    printf("Enter latitude (float): ");
    scanf("%f", &t.latitude);
    printf("Enter longitude (float): ");
    scanf("%f", &t.longitude);
    getchar(); // consume newline
    printf("Enter clue: ");
    fgets(t.clue, CLUE_MAX, stdin);
    t.clue[strcspn(t.clue, "\n")] = 0; // remove newline
    printf("Enter value (int): ");
    scanf("%d", &t.value);

    write(fd, &t, sizeof(Treasure));
    close(fd);

    // Log operation
    char log_msg[BUFFER_SIZE];
    snprintf(log_msg, sizeof(log_msg), "Added treasure ID %d", t.id);
    log_operation(hunt_id, log_msg);

    // Create symlink
    create_symlink(hunt_id);
}

void list_treasures(const char* hunt_id) {
    char file_path[BUFFER_SIZE];
    snprintf(file_path, sizeof(file_path), "%s/treasures.dat", hunt_id);

    struct stat st;
    if (stat(file_path, &st) == -1) {
        perror("stat");
        return;
    }

    printf("Hunt: %s\n", hunt_id);
    printf("File size: %ld bytes\n", st.st_size);
    printf("Last modified: %s", ctime(&st.st_mtime));

    int fd = open(file_path, O_RDONLY);
    if (fd == -1) {
        perror("open");
        return;
    }

    Treasure t;
    printf("\nTreasure List:\n");
    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        printf("ID: %d | User: %s | (%.4f, %.4f) | Value: %d\n", 
               t.id, t.username, t.latitude, t.longitude, t.value);
        printf("Clue: %s\n\n", t.clue);
    }

    close(fd);

    // Log operation
    char log_msg[BUFFER_SIZE];
    snprintf(log_msg, sizeof(log_msg), "Listed all treasures");
    log_operation(hunt_id, log_msg);
}

void view_treasure(const char* hunt_id, int treasure_id) {
    char file_path[BUFFER_SIZE];
    snprintf(file_path, sizeof(file_path), "%s/treasures.dat", hunt_id);

    int fd = open(file_path, O_RDONLY);
    if (fd == -1) {
        perror("open");
        return;
    }

    Treasure t;
    int found = 0;

    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        if (t.id == treasure_id) {
            printf("Treasure ID: %d\n", t.id);
            printf("User: %s\n", t.username);
            printf("Coordinates: (%.4f, %.4f)\n", t.latitude, t.longitude);
            printf("Clue: %s\n", t.clue);
            printf("Value: %d\n", t.value);
            found = 1;
            break;
        }
    }

    close(fd);

    if (!found) {
        printf("Treasure with ID %d not found in hunt %s.\n", treasure_id, hunt_id);
    } else {
        char log_msg[BUFFER_SIZE];
        snprintf(log_msg, sizeof(log_msg), "Viewed treasure ID %d", treasure_id);
        log_operation(hunt_id, log_msg);
    }
}

void remove_treasure(const char* hunt_id, int treasure_id) {
    char file_path[BUFFER_SIZE];
    snprintf(file_path, sizeof(file_path), "%s/treasures.dat", hunt_id);

    int fd = open(file_path, O_RDONLY);
    if (fd == -1) {
        perror("open for reading");
        return;
    }

    // Read all treasures and keep only those we want to keep
    Treasure* buffer = NULL;
    int count = 0, found = 0;

    Treasure t;
    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        if (t.id == treasure_id) {
            found = 1;
            continue; // skip the one to remove
        }
        buffer = realloc(buffer, (count + 1) * sizeof(Treasure));
        buffer[count++] = t;
    }

    close(fd);

    if (!found) {
        printf("Treasure with ID %d not found in hunt %s.\n", treasure_id, hunt_id);
        free(buffer);
        return;
    }

    // Overwrite the file with remaining treasures
    fd = open(file_path, O_WRONLY | O_TRUNC);
    if (fd == -1) {
        perror("open for writing");
        free(buffer);
        return;
    }

    for (int i = 0; i < count; i++) {
        write(fd, &buffer[i], sizeof(Treasure));
    }

    close(fd);
    free(buffer);

    printf("Treasure ID %d removed from hunt %s.\n", treasure_id, hunt_id);

    char log_msg[BUFFER_SIZE];
    snprintf(log_msg, sizeof(log_msg), "Removed treasure ID %d", treasure_id);
    log_operation(hunt_id, log_msg);
}

void remove_hunt(const char* hunt_id) {
    char dir_path[BUFFER_SIZE];
    snprintf(dir_path, sizeof(dir_path), "%s", hunt_id);

    DIR* dir = opendir(dir_path);
    if (!dir) {
        perror("opendir");
        return;
    }

    struct dirent* entry;
    char file_path[BUFFER_SIZE];

    // Remove all files in the directory
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        snprintf(file_path, sizeof(file_path), "%s/%s", dir_path, entry->d_name);
        if (unlink(file_path) == -1) {
            perror("unlink file");
        }
    }

    closedir(dir);

    // Remove directory itself
    if (rmdir(dir_path) == -1) {
        perror("rmdir");
        return;
    }

    // Remove symlink
    char linkname[BUFFER_SIZE];
    snprintf(linkname, sizeof(linkname), "logged_hunt-%s", hunt_id);
    unlink(linkname); // okay even if it fails silently

    printf("Hunt '%s' and all its data have been removed.\n", hunt_id);
}


int main(int argc, char* argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <operation> <hunt_id> [id]\n", argv[0]);
        return 1;
    }

    const char* command = argv[1];
    const char* hunt_id = argv[2];

    if (strcmp(command, "add") == 0) {
        add_treasure(hunt_id);
    } 

    else if (strcmp(command, "list") == 0) {
        list_treasures(hunt_id);
    }
    
    else if (strcmp(command, "view") == 0 && argc == 4) {
        int treasure_id = atoi(argv[3]);
        view_treasure(hunt_id, treasure_id);
   }

    else if (strcmp(command, "remove_treasure") == 0 && argc == 4) {
        int treasure_id = atoi(argv[3]);
        remove_treasure(hunt_id, treasure_id);
    }

    else if (strcmp(command, "remove_hunt") == 0) {
        remove_hunt(hunt_id);
    }
    
    else {
        printf("Operation not implemented yet: %s\n", command);
    }

    return 0;
}
