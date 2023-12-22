#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <limits.h>

#define MAX_BYTES 255

// Функция поиска заданной комбинации в файле
void search_in_file(const char *filename, const char *search_string) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    char buffer[MAX_BYTES];
    size_t totalBytesRead = 0;
    int occurrences = 0;

    while (1) {
        size_t bytesRead = fread(buffer, 1, sizeof(buffer), file);
        if (bytesRead == 0) {
            break;
        }

        for (size_t i = 0; i < bytesRead; ++i) {
            if (memcmp(buffer + i, search_string, strlen(search_string)) == 0) {
                occurrences++;
            }
        }

        totalBytesRead += bytesRead;
    }
    
    // Вывод информации о процессе поиска в файле
    printf("PID: %d, File: %s, Total Bytes Read: %zu, Occurrences: %d\n", getpid(), filename, totalBytesRead, occurrences);

    fclose(file);
}

// Функция поиска в директории
void search_in_directory(const char *dirname, const char *search_string, int max_processes) {
    DIR *dir = opendir(dirname);
    if (dir == NULL) {
        perror("Error opening directory");
        exit(EXIT_FAILURE);
    }

    struct dirent *entry;
    int running_processes = 0;

    // Функция поиска в директории
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) // Если это обычный файл
        {
            char path[PATH_MAX];

            //Использование `snprintf` для создания полного пути файла, объединяя текущий каталог (`dirname`) с именем файла (`entry->d_name`). 
            //Если размер пути превышает заданный максимум (`PATH_MAX`), выводится сообщение об ошибке, и программа завершается.
            if (snprintf(path, sizeof(path), "%s/%s", dirname, entry->d_name) >= sizeof(path)) {
                fprintf(stderr, "Path buffer overflow\n");
                closedir(dir);
                exit(EXIT_FAILURE);
            }

            pid_t child_pid = fork();

            if (child_pid == -1) {
                perror("Error creating child process");
                exit(EXIT_FAILURE);
            } else if (child_pid == 0) {
                // Child process
                search_in_file(path, search_string);
                exit(EXIT_SUCCESS);
            } else {
                // Parent process
                running_processes++;
                if (running_processes >= max_processes) {
                    int status;
                    wait(&status); 
                    if (WIFEXITED(status)) {
                        running_processes--;
                    }
                }
            }
        }
    }

    closedir(dir);

    //Ожидание завершения всех дочерних процессов. Если один из дочерних процессов завершается, 
    //уменьшается количество выполняющихся дочерних процессов. Это гарантирует, 
    //что родительский процесс подождет завершения всех дочерних задач перед завершением.
    while (running_processes > 0) {
        int status;
        wait(&status);
        if (WIFEXITED(status)) {
            running_processes--;
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <directory_name> <search_string> <max_processes>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *dirname = argv[1];
    char *search_string = argv[2];
    int max_processes = atoi(argv[3]);

    if (max_processes <= 0) {
        fprintf(stderr, "Invalid value for max_processes\n");
        exit(EXIT_FAILURE);
    }

    if (opendir(dirname) == NULL) {
        perror("Error opening directory");
        exit(EXIT_FAILURE);
    }

    search_in_directory(dirname, search_string, max_processes);

    return 0;
}
