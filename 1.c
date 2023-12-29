#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <time.h>

void displayTimeWithMilliseconds() {
    struct timeval currentTime;
    gettimeofday(&currentTime, NULL);

    struct tm *timeInfo;
    timeInfo = localtime(&currentTime.tv_sec);

    char timeBuffer[26];
    strftime(timeBuffer, 26, "%H:%M:%S", timeInfo);

    printf("%s:%02ld\n", timeBuffer, (currentTime.tv_usec )/10000);
}

int main() {
    pid_t parent_pid = getpid();
    pid_t child1_pid, child2_pid;

    child1_pid = fork(); // Create the first child process

    if (child1_pid < 0) {
        perror("Error creating first child process");
        exit(1);
    } else if (child1_pid == 0) {
        printf("First child process - PID: %d, PPID: %d\n", getpid(), getppid());

        displayTimeWithMilliseconds();
        exit(0);
    } else {
        child2_pid = fork(); // Create a second child process

        if (child2_pid < 0) {
            perror("Error creating second child process");
            exit(1);
        } else if (child2_pid == 0) {
            printf("Second child process - PID: %d, PPID: %d\n", getpid(), getppid());

            displayTimeWithMilliseconds();
            exit(0);
        } else {
            // Parent process
            printf("Parent process - PID: %d\n", parent_pid);
            
            system("ps -x");
            wait(NULL);
            wait(NULL);
            
            exit(0);
        }
    }
}
