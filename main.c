#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>

#define MAX_ENTRIES 1000
#define MAX_ARGS 10
// Structura pentru stocarea metadatelor 
struct Metadata {
    char name[256];
    time_t last_modified;
   // mode_t mode;
};


void update_snapshot(const char *dir_path) {
    DIR *dir = opendir(dir_path);
    if (dir == NULL) {
        perror("opendir");
        exit(EXIT_FAILURE);
    }


    char snapshot_file_path[512];
    snprintf(snapshot_file_path, sizeof(snapshot_file_path), "%s/snapshot.txt", dir_path);
    FILE *fp = fopen(snapshot_file_path, "w");
    if (fp == NULL) {
        perror("fopen");
        closedir(dir);
        exit(EXIT_FAILURE);
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Ignorăm intrările implicite "." și ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char entry_path[512];
        snprintf(entry_path, sizeof(entry_path), "%s/%s", dir_path, entry->d_name);
                

        struct stat entry_stat;
        if (lstat(entry_path, &entry_stat) == -1) {
            perror("lstat");
            fclose(fp);
            closedir(dir);
            exit(EXIT_FAILURE);
        }

        fprintf(fp, "%s - Last modified: %s", entry_path ,ctime(&entry_stat.st_mtime));


        if (S_ISDIR(entry_stat.st_mode)) {
            update_snapshot(entry_path);
        }
    }


    fclose(fp);
    closedir(dir);
}

int main(int argc, char *argv[]) {
    if (argc < 2 || argc > MAX_ARGS + 1) {
        fprintf(stderr, "Usage: %s <directory1> <directory2> ... <directory%d>\n", argv[0], MAX_ARGS);
        return EXIT_FAILURE;
    }

    // Parcurgem toate argumentele și apelăm funcția update_snapshot pentru directoarele valide
    for (int i = 1; i < argc; i++) {
        struct stat st;
        if (stat(argv[i], &st) == -1 || !S_ISDIR(st.st_mode)) {
            fprintf(stderr, "argumentul nu este director: %s\n", argv[i]);
            continue;
        }
        printf("Updatez snapshot ul pentru director: %s\n", argv[i]);
        update_snapshot(argv[i]);
    }

    return EXIT_SUCCESS;
}
