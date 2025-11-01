#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#define BUFFER_SIZE 1024

void write_str(int fd, const char *str) {
    write(fd, str, strlen(str));
}

int main() {
    char filename[256];
    ssize_t bytes_read;

    write_str(STDOUT_FILENO, "Введите имя файла: ");
    bytes_read = read(STDIN_FILENO, filename, sizeof(filename) - 1);
    if (bytes_read <= 0) {
        write_str(STDERR_FILENO, "Ошибка чтения имени файла\n");
        exit(EXIT_FAILURE);
    }

    if (filename[bytes_read - 1] == '\n') {
        filename[bytes_read - 1] = '\0';
    } else {
        filename[bytes_read] = '\0';
    }

    int pipe1[2];
    if (pipe(pipe1) == -1) {
        write_str(STDERR_FILENO, "Ошибка создания pipe\n");
        exit(EXIT_FAILURE);
    }

    pid_t child = fork();
    
    switch (child) {
    case -1: {
        write_str(STDERR_FILENO, "Ошибка создания процесса\n");
        exit(EXIT_FAILURE);
    } break;
    
    case 0: { 
        close(pipe1[0]); 

        dup2(pipe1[1], STDOUT_FILENO);
        close(pipe1[1]);

        char *args[] = {"child", filename, NULL};
        execv("./child", args);

        write_str(STDERR_FILENO, "Ошибка запуска дочерней программы\n");
        exit(EXIT_FAILURE);
    } break;
    
    default: { 
        close(pipe1[1]); 
 
        char buffer[BUFFER_SIZE];
        while ((bytes_read = read(pipe1[0], buffer, sizeof(buffer))) > 0) {
            write(STDOUT_FILENO, buffer, bytes_read);
        }
        
        close(pipe1[0]);
        wait(NULL); 
    } break;
    }
    
    return 0;
}