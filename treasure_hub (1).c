// treasure hub

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <dirent.h>

pid_t monitor_pid = -1;
int monitor_running = 0;
char command_file[] = "monitor_cmd.txt";

void handle_sigchld(int sig) {
    int status;
    pid_t pid = waitpid(monitor_pid, &status, WNOHANG);
    if (pid > 0) {
        printf("[hub] monitorul s-a terminat cu status %d.\n", status);
        monitor_running = 0;
        monitor_pid = -1;
    }
}

void send_signal_to_monitor(int signal) {
    if (monitor_running) {
        kill(monitor_pid, signal);
    } else {
        printf("[eroare] monitorul nu ruleaza.\n");
    }
}

void write_command(const char *cmd) {
    int fd = open(command_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd >= 0) {
        dprintf(fd, "%s\n", cmd);
        close(fd);
    } else {
        perror("nu pot scrie in fisierul de comenzi");
    }
}

void execute_calculate_score() {
    DIR *dir = opendir(".");
    if (!dir) {
        perror("Nu pot deschide directorul curent");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir))) {
        if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            char treasures_path[256];
            snprintf(treasures_path, sizeof(treasures_path), "%s/treasures.dat", entry->d_name);
            if (access(treasures_path, F_OK) != 0) continue;

            int pipefd[2];
            if (pipe(pipefd) == -1) {
                perror("pipe");
                continue;
            }

            pid_t pid = fork();
            if (pid == 0) {
                close(pipefd[0]);
                dup2(pipefd[1], STDOUT_FILENO);
                execlp("./calculate_score", "calculate_score", entry->d_name, NULL);
                perror("exec calculate_score");
                exit(1);
            } else if (pid > 0) {
                close(pipefd[1]);
                char buffer[256];
                ssize_t bytes_read = read(pipefd[0], buffer, sizeof(buffer) - 1);
                if (bytes_read > 0) {
                    buffer[bytes_read] = '\0';
                    printf("[score] %s", buffer);
                }
                close(pipefd[0]);
                waitpid(pid, NULL, 0);
            } else {
                perror("fork");
            }
        }
    }

    closedir(dir);
}

int main() {
    struct sigaction sa;
    sa.sa_handler = handle_sigchld;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &sa, NULL);

    char input[256];

    while (1) {
        printf(">> ");
        fflush(stdout);
        if (!fgets(input, sizeof(input), stdin)) break;
        input[strcspn(input, "\n")] = 0;

        if (strcmp(input, "start_monitor") == 0) {
            if (monitor_running) {
                printf("[eroare] monitorul deja ruleaza.\n");
                continue;
            }

            monitor_pid = fork();
            if (monitor_pid == 0) {
                execlp("./monitor", "monitor", NULL);
                perror("Eroare la exec monitor");
                exit(1);
            } else if (monitor_pid > 0) {
                monitor_running = 1;
                printf("[hub] monitorul a fost pornit cu PID %d.\n", monitor_pid);
            } else {
                perror("Eroare la fork pentru monitor");
            }

        } else if (strcmp(input, "stop_monitor") == 0 ||
                   strcmp(input, "list_hunts") == 0 ||
                   strncmp(input, "list_treasures", 14) == 0 ||
                   strncmp(input, "view_treasure", 13) == 0) {
            if (monitor_running) {
                write_command(input);
                send_signal_to_monitor(SIGUSR1);
            } else {
                printf("[eroare] monitorul nu este activ.\n");
            }

        } else if (strcmp(input, "calculate_score") == 0) {
            execute_calculate_score();

        } else if (strcmp(input, "exit") == 0) {
            if (monitor_running) {
                printf("[eroare] monitorul inca ruleaza.\n");
            } else {
                break;
            }

        } else {
            printf("[eroare] comanda necunoscuta.\n");
        }
    }

    if (monitor_running) {
        kill(monitor_pid, SIGKILL);
        waitpid(monitor_pid, NULL, 0);
    }

    return 0;
}
