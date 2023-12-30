#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <limits.h>

#define MAX_BYTES 255

int current_processes_count = 0;

// Функция выполняет поиск заданной комбинации байт в файле и выводит информацию о процессе поиска.
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

    // Вывод информации о процессе поиска в файле с номером текущего процесса
    printf("PID: %d, File: %s, Total Bytes Read: %zu, Occurrences: %d, Number of running processes: %d\n", getpid(), filename, totalBytesRead, occurrences, current_processes_count);

    fclose(file);
}

// Функция открывает директорию, обходит все файлы в ней, и для каждого файла запускает отдельный процесс поиска.
void search_in_directory(const char *dirname, const char *search_string, int max_processes) {
    DIR *dir = opendir(dirname);
    if (dir == NULL) {
        perror("Error opening directory");
        exit(EXIT_FAILURE);
    }

    struct dirent *entry;
    int total_processes_count = 0;

    // Функция поиска в директории
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) // Если это обычный файл
        {
            char path[PATH_MAX];

            if (snprintf(path, sizeof(path), "%s/%s", dirname, entry->d_name) >= sizeof(path)) {
                fprintf(stderr, "Path buffer overflow\n");
                closedir(dir);
                exit(EXIT_FAILURE);
            }

            // Проверка, чтобы не превысить максимальное количество одновременно работающих процессов
            while (current_processes_count >= max_processes) {
                int status;
                pid_t finished_pid = wait(&status);
                if (WIFEXITED(status)) {
                    current_processes_count--;
                } 
            }

            // Создание нового процесса
            pid_t child_pid = fork();

            if (child_pid == -1) {
                perror("Error creating child process");
                exit(EXIT_FAILURE);
            } else if (child_pid == 0) {
                // Дочерний процесс вызывает "search_in_file" для выполнения поиска в конкретном файле.
                search_in_file(path, search_string);
                exit(EXIT_SUCCESS);
            } else {
                // Родительский процесс
                current_processes_count++;
                total_processes_count++;
           
            }
        }
    }

    closedir(dir);

    // Ожидание завершения всех дочерних процессов.
    while (current_processes_count > 0) {
        int status;
        pid_t finished_pid = wait(&status);
        if (WIFEXITED(status)) {
            current_processes_count--;
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
