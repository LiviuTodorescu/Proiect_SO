#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#define MAX_ENTRIES 1000
#define MAX_ARGS 10

struct Metadata {
    char name[256];
    time_t last_modified;
    //mode_t mode;
};


void update_snapshot(const char *dir_path) {
    DIR *dir = opendir(dir_path);
    if (dir == NULL) {
        perror("opendir");
        exit(EXIT_FAILURE);
    }

    struct Metadata metadata[MAX_ENTRIES];
    int num_entries = 0;

    // Afisam snapshotul inainte de a-l salva
    printf("Snapshot Contents for directory %s:\n", dir_path);
    while (num_entries < MAX_ENTRIES) {
        struct dirent *entry = readdir(dir);
        if (entry == NULL)
            break;

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

        // Stocam metadatele intrarii
        strcpy(metadata[num_entries].name, entry->d_name);
        metadata[num_entries].last_modified = entry_stat.st_mtime;
      //  metadata[num_entries].mode = entry_stat.st_mode;

        // Afisam metadatele intrarii
        printf("Name: %s\n", metadata[num_entries].name);
        printf("Last Modified: %s\n\n", ctime(&metadata[num_entries].last_modified)); 
       // printf("Mode: %o\n\n", metadata[num_entries].mode);

        num_entries++;
    }


    closedir(dir);

    // Deschidem fișierul snapshot.bin pentru a scrie
    char snapshot_file_path[512];
    snprintf(snapshot_file_path, sizeof(snapshot_file_path), "%s/snapshot.bin", dir_path);
    int fd = open(snapshot_file_path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    // Scriem metadatele în fișierul snapshot.bin
    if (write(fd, metadata, num_entries * sizeof(struct Metadata)) == -1) {
        perror("write");
        close(fd);
        exit(EXIT_FAILURE);
    }

    close(fd);
}


int main(int argc, char *argv[]) {
    // Verificăm dacă argumentele sunt suficiente
    if (argc < 2 || argc > MAX_ARGS + 1) {
        fprintf(stderr, "Usage: %s <directory1> <directory2> ... <directory%d>\n", argv[0], MAX_ARGS);
        return EXIT_FAILURE;
    }

    // Parcurgem toate argumentele și creăm un proces copil pentru fiecare director
    for (int i = 1; i < argc; i++) {
        struct stat st;
        if (stat(argv[i], &st) == -1 || !S_ISDIR(st.st_mode)) {
            fprintf(stderr, "Ignoring non-directory argument: %s\n", argv[i]);
            continue;
        }

        printf("Updating snapshot for directory: %s\n", argv[i]);
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            return EXIT_FAILURE;
        } else if (pid == 0) {
            // Suntem în procesul copil
            update_snapshot(argv[i]);
            exit(EXIT_SUCCESS);
        }
    }

    // Așteptăm ca toate procesele copil să se încheie
    int status;
    pid_t pid;
    while ((pid = wait(&status)) != -1);

    return EXIT_SUCCESS;
}
