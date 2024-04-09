#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>

#define MAX_ENTRIES 1000

// Structura pentru stocarea metadatelor fiecărei intrări (fișier/director)
struct Metadata {
    char name[256];
    time_t last_modified;
    mode_t mode;
};

// Funcție pentru actualizarea capturii (snapshot-ului) directorului
void update_snapshot(const char *dir_path) {
    DIR *dir = opendir(dir_path);
    if (dir == NULL) {
        perror("opendir");
        exit(EXIT_FAILURE);
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Ignorăm intrările implicite "." și ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        // Construim calea absolută pentru intrare
        char entry_path[512];
        snprintf(entry_path, sizeof(entry_path), "%s/%s", dir_path, entry->d_name);

        // Obținem informații despre intrare
        struct stat entry_stat;
        if (lstat(entry_path, &entry_stat) == -1) {
            perror("lstat");
            closedir(dir);
            exit(EXIT_FAILURE);
        }

        // Salvăm metadatele în snapshot
        struct Metadata snapshot[MAX_ENTRIES];
        int snapshot_count = 0;
        strcpy(snapshot[snapshot_count].name, entry->d_name);
        snapshot[snapshot_count].last_modified = entry_stat.st_mtime;
        snapshot[snapshot_count].mode = entry_stat.st_mode;

        snapshot_count++;

        // Dacă intrarea este un director, creăm un snapshot în acesta recursiv
        if (S_ISDIR(entry_stat.st_mode)) {
            update_snapshot(entry_path);
        }

        // Scriem captura într-un fișier în directorul curent
        char snapshot_file_path[512];
        snprintf(snapshot_file_path, sizeof(snapshot_file_path), "%s/snapshot.txt", dir_path);
        FILE *fp = fopen(snapshot_file_path, "w");
        if (fp == NULL) {
            perror("fopen");
            exit(EXIT_FAILURE);
        }

        // Scriem captura în fișier
        fprintf(fp, "Snapshot for directory: %s\n", dir_path);
        for (int i = 0; i < snapshot_count; i++) {
            fprintf(fp, "%s - Last modified: %s", snapshot[i].name, ctime(&snapshot[i].last_modified));
        }

        // Închidem fișierul
        fclose(fp);
    }

    closedir(dir);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <directory>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Directorul de monitorizat
    const char *dir_path = argv[1];

    // Apelăm funcția pentru a crea snapshot-ul în fiecare director
    update_snapshot(dir_path);

    return EXIT_SUCCESS;
}
