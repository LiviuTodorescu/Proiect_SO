#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

#define MAX_ENTRIES 1000
/*
<<<<<READ ME >>>>>
./p /home/kali/Desktop/tema_so/dir1/dir2 /home/kali/Desktop/tema_so/dir1/dir3 /home/kali/Desktop/tema_so/dir1 /home/kali/Desktop/tema_so/malitios_files -o /home/kali/Desktop/tema_so/output_dir
structura de directoare pe care am testat acest cod arata in felul urmator
    dir 1
        dir2
            dir4
        dir3
        dir5
            dir6
    
-pentru ca programul sa functioneze scriptul script.sh trebuie sa fie in acelasi director cu main.c
-snapshotu ul este salvat in director de output sub forma de fisier binar, dar continutul acestuia este afisat si in terminal
-fisiere malitioase am bagat in directoare random pentru a testa functionalitatea
./p <dir1> <dir2> <dir3> <directorul pentru fisiere malitioase> -o <director outoput>

<<disclaimer>>
nu sunt vinovat daca in urma rularii programului se pot intampla diferite analomalii precum furtuni geomagnetice, cadere de corpuri ceresti, cutremur, lumini ciudate pe cer


sper sa mearga bine :)
*/
// Structura pentru stocarea metadatelor fiecarei intrari (fisier/director)
struct Metadata {
    char name[256];
    time_t last_modified;
    mode_t mode;
};

void update_snapshot(const char *output_dir, int num_directories, const char *directories[]) {

    DIR *output = opendir(output_dir);
    if (output == NULL) {
        perror("opendir");
        exit(EXIT_FAILURE);
    }

    // Vector pentru a stoca metadatele fiecarui fisier
    struct Metadata metadata[MAX_ENTRIES];
    int num_entries = 0;

    // Vector pentru a stoca calea catre directorul fiecarui fisier
    char entry_directories[MAX_ENTRIES][512];

    // Parcurg fiecare vector
    for (int i = 0; i < num_directories; i++) {
        // Deschidem directorul
        DIR *dir = opendir(directories[i]);
        if (dir == NULL) {
            perror("opendir");
            continue;
        }

        // Parcurg fiecare fisier
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL && num_entries < MAX_ENTRIES) {
            // Ignoram intrarile implicite "." si ".."
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;

            // Construim calea absoluta pentru fiecare fisier
            char entry_path[512];
            snprintf(entry_path, sizeof(entry_path), "%s/%s", directories[i], entry->d_name);

            // Obținem informații despre fiecare fisier
            struct stat entry_stat;
            if (lstat(entry_path, &entry_stat) == -1) {
                perror("lstat");
                continue;
            }

            // Stocam metadatele fiecarui fisier
            strcpy(metadata[num_entries].name, entry->d_name);
            metadata[num_entries].last_modified = entry_stat.st_mtime;
            metadata[num_entries].mode = entry_stat.st_mode;

            // Stocam calea catre directorul fiecarui fisier
            strcpy(entry_directories[num_entries], directories[i]);

            num_entries++;
        }

        // Inchidem directorul
        closedir(dir);
    }

    // Inchidem directorul de iesire
    closedir(output);

    // Construim calea catre fisierul snapshot.bin In directorul de iesire
    char snapshot_file_path[512];
    snprintf(snapshot_file_path, sizeof(snapshot_file_path), "%s/snapshot.bin", output_dir);

    // Deschidem fisierul snapshot.bin pentru a scrie metadatele
    int fd = open(snapshot_file_path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    // Scriem metadatele si calea catre directorul fiecarui fisier In fisierul snapshot.bin
    for (int i = 0; i < num_entries; i++) {
        write(fd, &metadata[i], sizeof(struct Metadata));
        write(fd, entry_directories[i], strlen(entry_directories[i]) + 1);
    }

    // Inchidem fisierul
    close(fd);

    // Afisam conținutul snapshot-ului In terminal
    printf("Snapshot Contents for directories:\n");
    for (int i = 0; i < num_entries; i++) {
        printf("Name: %s\n", metadata[i].name);
        printf("Directory: %s\n", entry_directories[i]);
        printf("Last Modified: %s", ctime(&metadata[i].last_modified));
        printf("Mode: %o\n\n", metadata[i].mode);
    }
}




// Funcție pentru mutarea unui fisier malitios In directorul specificat
void move_malicious_file(const char *file_path, const char *malicious_dir) {
    // Construim calea noua pentru fisierul malitios
    char new_path[512];
    snprintf(new_path, sizeof(new_path), "%s/%s", malicious_dir, strrchr(file_path, '/') + 1);

    // Mutam fisierul In directorul de fisiere malitioase
    if (rename(file_path, new_path) == -1) {
        perror("rename");
        return;
    }

    printf("Moved malicious file %s to %s\n", file_path, new_path);
}

void process_directory(const char *dir_path, const char *output_dir, const char *malicious_dir) {
    DIR *dir = opendir(dir_path);
    if (dir == NULL) {
        perror("opendir");
        return;
    }

    struct dirent *entry;
    int malicious_count = 0; // Contor pentru numarul de fisiere malitioase
    while ((entry = readdir(dir)) != NULL) {
        // Ignoram intrarile implicite "." si ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        // Construim calea absoluta pentru elementul curent
        char element_path[512];
        snprintf(element_path, sizeof(element_path), "%s/%s", dir_path, entry->d_name);

        // Verificam daca elementul este un director sau un fisier
        struct stat st;
        if (lstat(element_path, &st) == -1) {
            perror("lstat");
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            // Daca este director, procesam recursiv directorul
            process_directory(element_path, output_dir, malicious_dir);
        } else {
            // Daca este fisier, verificam daca este malitios si mutam, daca este cazul
            int pipefd[2];
            if (pipe(pipefd) == -1) {
                perror("pipe");
                continue;
            }

            pid_t pid = fork();
            if (pid == -1) {
                perror("fork");
                close(pipefd[0]);
                close(pipefd[1]);
                continue;
            }

            if (pid == 0) {
                // Procesul fiu
                close(pipefd[0]); // Inchidem capatul de citire al pipe-ului In procesul fiu

                char command[2000];
                snprintf(command, sizeof(command), "sudo ./script.sh %s", element_path);
                int script_result = system(command);

                // Verificam rezultatul Intoarcerii scriptului
                if (WIFEXITED(script_result) && WEXITSTATUS(script_result) == 1) {
                    // Fisier malitios
                    write(pipefd[1], "Malicious", 10);
                } else {
                    // Fisier sigur
                    write(pipefd[1], "Safe", 5);
                }

                close(pipefd[1]); // Inchidem capatul de scriere al pipe-ului si terminam procesul fiu
                exit(EXIT_SUCCESS);
            } else {
                // Procesul parinte
                close(pipefd[1]); // Inchidem capatul de scriere al pipe-ului In procesul parinte

                char buffer[512];
                int status;
                waitpid(pid, &status, 0); // Asteptam finalizarea procesului fiu si preluam statusul
                read(pipefd[0], buffer, sizeof(buffer));

                if (strcmp(buffer, "Malicious") == 0) {
                    // Fisier malitios
                    move_malicious_file(element_path, malicious_dir);
                    malicious_count++; // Incrementam contorul de fisiere malitioase
                }

                // Afisam statusul procesului fiu si numarul de fisiere malitioase
                printf("Procesul copil %d s-a incheiat cu PID-ul %d si cu %d fisiere malitioase\n", pid, pid, malicious_count);

                close(pipefd[0]); // Inchidem capatul de citire al pipe-ului
            }
        }
    }

    closedir(dir);
}

int main(int argc, char *argv[]) {
    // Verificam daca argumentele sunt suficiente
    if (argc < 4 || strcmp(argv[argc - 2], "-o") != 0) {
        fprintf(stderr, "Usage: %s <directory1> [<directory2> ...] <malicious_directory> -o <output_directory>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *output_dir = argv[argc - 1]; // Directorul de iesire este ultimul argument
    const char *malicious_dir = argv[argc - 3]; // Directorul malitios este penultimul argument Inainte de "-o"

    // Calculam numarul de directoare care trebuie procesate
    int num_directories = argc - 4; // Excludem numele programului, directorul malitios, "-o" si directorul de iesire

    // Alocam memorie pentru a stoca directoarele care trebuie procesate
    const char **directories = malloc(num_directories * sizeof(char*));
    if (directories == NULL) {
        perror("malloc");
        return EXIT_FAILURE;
    }

    // Copiem argumentele (directoarele) In vectorul 'directories'
    for (int i = 1; i <= num_directories; i++) {
        directories[i - 1] = argv[i];
    }

    // Iteram prin toate directoarele primite ca argumente si aplicam process_directory pentru fiecare
    for (int i = 0; i < num_directories; i++) {
        printf("Processing directory: %s\n", directories[i]);
        process_directory(directories[i], output_dir, malicious_dir);
    }

    // Actualizam snapshot-ul pentru toate directoarele
    update_snapshot(output_dir, num_directories, directories);

    // Eliberam memoria alocata pentru vectorul 'directories'
    free(directories);

    return EXIT_SUCCESS;
}
