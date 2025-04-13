#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#define MAX_NAME 64
#define MAX_CLUE 256

typedef struct {
    int id;
    char username[MAX_NAME];
    double latitude;
    double longitude;
    char clue[MAX_CLUE];
    int value;
} Treasure;

void usage(const char* prog) {
    printf("Usage:\n");
    printf("  %s add <hunt_id>\n", prog);
    printf("  %s list <hunt_id>\n", prog);
    printf("  %s view <hunt_id> <id>\n", prog);
    printf("  %s remove_treasure <hunt_id> <id>\n", prog);
    printf("  %s remove_hunt <hunt_id>\n", prog);
}

// Returnează calea completă către fișierul treasures.dat 
char* get_hunt_path(const char* hunt_id) {
    static char path[128];
    snprintf(path, sizeof(path), "./%s/treasures.dat", hunt_id);
    return path;
}

// Returnează calea către directorul
char* get_hunt_dir(const char* hunt_id) {
    static char dir[128];
    snprintf(dir, sizeof(dir), "./%s", hunt_id);
    return dir;
}

// Returnează calea către fișierul de log
char* get_log_path(const char* hunt_id) {
    static char path[128];
    snprintf(path, sizeof(path), "./%s/logged_hunt", hunt_id);
    return path;
}

// Scrie o acțiune în fișierul de log și creează un symlink către acesta
void log_action(const char* hunt_id, const char* action) {
    int fd = open(get_log_path(hunt_id), O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd >= 0) {
        dprintf(fd, "%s\n", action);
        close(fd);
        char symlink_name[128];
        snprintf(symlink_name, sizeof(symlink_name), "logged_hunt-%s", hunt_id);
        symlink(get_log_path(hunt_id), symlink_name);
    }
}

void add_treasure(const char* hunt_id) {
    mkdir(get_hunt_dir(hunt_id), 0755);
    int fd = open(get_hunt_path(hunt_id), O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0) { perror("open"); return; }

    Treasure t;
    printf("ID: "); scanf("%d", &t.id);
    printf("Username: "); scanf("%s", t.username);
    printf("Latitude: "); scanf("%lf", &t.latitude);
    printf("Longitude: "); scanf("%lf", &t.longitude);
    printf("Clue: "); getchar(); fgets(t.clue, MAX_CLUE, stdin);
    t.clue[strcspn(t.clue, "\n")] = 0;
    printf("Value: "); scanf("%d", &t.value);

    // Scrie comoara în fișierul binar
    write(fd, &t, sizeof(Treasure));
    close(fd);

    log_action(hunt_id, "add_treasure");
}

void list_treasures(const char* hunt_id) {
    struct stat st;
    if (stat(get_hunt_path(hunt_id), &st) == -1) {
        perror("stat"); return;
    }

    printf("Hunt: %s\nSize: %ld bytes\nLast modified: %s\n", hunt_id, st.st_size, ctime(&st.st_mtime));

    int fd = open(get_hunt_path(hunt_id), O_RDONLY);
    if (fd < 0) { perror("open"); return; }

    Treasure t;
    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        printf("ID: %d | User: %s | Lat: %.6lf | Long: %.6lf | Value: %d\n",
               t.id, t.username, t.latitude, t.longitude, t.value);
    }
    close(fd);
    log_action(hunt_id, "list_treasures");
}

void view_treasure(const char* hunt_id, int id) {
    int fd = open(get_hunt_path(hunt_id), O_RDONLY);
    if (fd < 0) { perror("open"); return; }

    Treasure t;
    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        if (t.id == id) {
            printf("Found: ID %d\nUser: %s\nLat: %.6lf\nLong: %.6lf\nClue: %s\nValue: %d\n",
                   t.id, t.username, t.latitude, t.longitude, t.clue, t.value);
            close(fd);
            log_action(hunt_id, "view_treasure");
            return;
        }
    }
    printf("Treasure not found.\n");
    close(fd);
}

void remove_treasure(const char* hunt_id, int id) {
    int fd = open(get_hunt_path(hunt_id), O_RDONLY);
    if (fd < 0) { perror("open"); return; }

    int temp_fd = open("temp.dat", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (temp_fd < 0) { perror("temp open"); close(fd); return; }

    Treasure t;
    int found = 0;
    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        if (t.id != id) {
            write(temp_fd, &t, sizeof(Treasure));
        } else {
            found = 1;
        }
    }
    close(fd);
    close(temp_fd);

    if (!found) {
        printf("Treasure with ID %d not found.\n", id);
        unlink("temp.dat");
        return;
    }

    // Înlocuiește fișierul original cu cel temporar
    if (rename("temp.dat", get_hunt_path(hunt_id)) != 0) {
        perror("rename");
    } else {
        printf("Treasure %d removed successfully.\n", id);
        log_action(hunt_id, "remove_treasure");
    }
}

void remove_hunt(const char* hunt_id) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "rm -rf ./%s", hunt_id);
    system(cmd);
    log_action(hunt_id, "remove_hunt");
}

// Analizează argumentele și apelează funcția corespunzătoare
int main(int argc, char* argv[]) {
    if (argc < 3) { usage(argv[0]); return 1; }

    if (strcmp(argv[1], "add") == 0) {
        add_treasure(argv[2]);
    } else if (strcmp(argv[1], "list") == 0) {
        list_treasures(argv[2]);
    } else if (strcmp(argv[1], "view") == 0 && argc == 4) {
        view_treasure(argv[2], atoi(argv[3]));
    } else if (strcmp(argv[1], "remove_treasure") == 0 && argc == 4) {
        remove_treasure(argv[2], atoi(argv[3]));
    } else if (strcmp(argv[1], "remove_hunt") == 0) {
        remove_hunt(argv[2]);
    } else {
        usage(argv[0]);
    }

    return 0;
}
