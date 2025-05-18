
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <hunt_id>\n", argv[0]);
        return 1;
    }

    char path[128];
    snprintf(path, sizeof(path), "./%s/treasures.dat", argv[1]);
    FILE* f = fopen(path, "rb");
    if (!f) {
        perror("fopen");
        return 1;
    }

    Treasure t;
    int total = 0;
    while (fread(&t, sizeof(Treasure), 1, f) == 1) {
        total += t.value;
    }

    fclose(f);
    printf("%s: %d\n", argv[1], total);
    return 0;
}
