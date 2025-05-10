#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// calculeaza scor in functie de user

#define NAME_SIZE 100
#define LINE_SIZE 1000
#define MAX_USERS 100

typedef struct {
    char treasure_id[10];
    char username[NAME_SIZE];
    double latitude, longitude;
    char clue[LINE_SIZE];
    int value;
} treasure;

typedef struct {
    char user[NAME_SIZE];
    int total_score;
}usr_score;



int main(int argc, char **argv) {
    if(argc != 2) {
        perror("format incorect\n");
        exit(-1);
    }

    char path[256];
    snprintf(path, sizeof(path), "%s/treasures.dat", argv[1]);

    FILE *f = fopen(path, "rb");
    if(!f) {
        perror("error opening file\n");
        exit(-1);
    }

    usr_score scores[MAX_USERS];
    int usr_count = 0;

    treasure t;
    while(fread(&t, sizeof(treasure), 1, f) == 1) {
        int found = 0;
        for(int i = 0; i < usr_count; i++) {
            if(strcmp(scores[i].user, t.username) == 0) {
                scores[i].total_score += t.value;
                found = 1;
                break;
            }
        }
        if(!found && usr_count < MAX_USERS) {
            strncpy(scores[usr_count].user, t.username, NAME_SIZE);
            scores[usr_count].total_score = t.value;
            usr_count++;
        }
    }

    fclose(f);

    for(int i = 0; i < usr_count; i++) {
        printf("%s: %d\n", scores[i].user, scores[i].total_score);
    }
    return 0;
}