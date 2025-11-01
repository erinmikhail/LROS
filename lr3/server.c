#include <fcntl.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <wait.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <time.h>

#define SHM_SIZE 4096
#define MAX_FILENAME 256

void generate_unique_names(char *shm_name, char *sem_name, size_t size) {
    static int counter = 0;

    char pid_str[20];
    char counter_str[20];
    
    int pid = getpid();
    char *ptr = pid_str;
    if (pid == 0) {
        *ptr++ = '0';
    } else {
        int temp = pid;
        char *start = ptr;
        while (temp > 0) {
            *ptr++ = '0' + (temp % 10);
            temp /= 10;
        }
        *ptr = '\0';
        int len = ptr - start;
        for (int i = 0; i < len / 2; i++) {
            char tmp = start[i];
            start[i] = start[len - 1 - i];
            start[len - 1 - i] = tmp;
        }
        ptr = start + len;
    }
    *ptr = '\0';
    
    ptr = counter_str;
    if (counter == 0) {
        *ptr++ = '0';
    } else {
        int temp = counter;
        char *start = ptr;
        while (temp > 0) {
            *ptr++ = '0' + (temp % 10);
            temp /= 10;
        }
        *ptr = '\0';
        int len = ptr - start;
        for (int i = 0; i < len / 2; i++) {
            char tmp = start[i];
            start[i] = start[len - 1 - i];
            start[len - 1 - i] = tmp;
        }
        ptr = start + len;
    }
    *ptr = '\0';

    strcpy(shm_name, "div-shm-");
    strcat(shm_name, pid_str);
    strcat(shm_name, "-");
    strcat(shm_name, counter_str);
    
    strcpy(sem_name, "div-sem-");
    strcat(sem_name, pid_str);
    strcat(sem_name, "-");
    strcat(sem_name, counter_str);
    
    counter++;
}

void write_str(int fd, const char *str) {
    write(fd, str, strlen(str));
}

int main() {
    char shm_name[64], sem_name[64];
    generate_unique_names(shm_name, sem_name, sizeof(shm_name));

    char filename[MAX_FILENAME];
    write_str(STDOUT_FILENO, "Введите имя файла: ");
    ssize_t bytes_read = read(STDIN_FILENO, filename, sizeof(filename) - 1);
    if (bytes_read <= 0) {
        write_str(STDERR_FILENO, "Ошибка чтения имени файла\n");
        exit(EXIT_FAILURE);
    }

    if (bytes_read > 0 && filename[bytes_read - 1] == '\n') {
        filename[bytes_read - 1] = '\0';
    } else {
        filename[bytes_read] = '\0';
    }

    int shm_fd = shm_open(shm_name, O_RDWR | O_CREAT | O_EXCL, 0600);
    if (shm_fd == -1) {
        write_str(STDERR_FILENO, "ошибка: не удалось создать SHM\n");
        exit(EXIT_FAILURE);
    }

    if (ftruncate(shm_fd, SHM_SIZE) == -1) {
        write_str(STDERR_FILENO, "ошибка: не удалось изменить размер SHM\n");
        shm_unlink(shm_name);
        exit(EXIT_FAILURE);
    }

    char *shm_buf = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_buf == MAP_FAILED) {
        write_str(STDERR_FILENO, "ошибка: не удалось сопоставить SHM\n");
        shm_unlink(shm_name);
        exit(EXIT_FAILURE);
    }

    sem_t *sem = sem_open(sem_name, O_CREAT | O_EXCL, 0600, 1);
    if (sem == SEM_FAILED) {
        write_str(STDERR_FILENO, "ошибка создания семафора\n");
        munmap(shm_buf, SHM_SIZE);
        shm_unlink(shm_name);
        exit(EXIT_FAILURE);
    }

    uint32_t *length = (uint32_t *)shm_buf;
    uint32_t *status = (uint32_t *)(shm_buf + sizeof(uint32_t));
    *length = 0;
    *status = 0;

    pid_t client = fork();
    if (client == 0) {
        char *args[] = {"client", shm_name, sem_name, filename, NULL};
        execv("./client", args);
        
        write_str(STDERR_FILENO, "ошибка в клиенте\n");
        _exit(EXIT_FAILURE);
    } else if (client == -1) {
        write_str(STDERR_FILENO, "ошибка\n");
        sem_close(sem);
        sem_unlink(sem_name);
        munmap(shm_buf, SHM_SIZE);
        shm_unlink(shm_name);
        exit(EXIT_FAILURE);
    }

    bool running = true;
    int iteration = 0;
    
    while (running && iteration < 50) {
        sem_wait(sem);
        
        uint32_t *length = (uint32_t *)shm_buf;
        uint32_t *status = (uint32_t *)(shm_buf + sizeof(uint32_t));
        char *data = shm_buf + 2 * sizeof(uint32_t);
        
        if (*status == 1) { 
            write_str(STDERR_FILENO, "Ошибка: деление на ноль\n");
            running = false;
        } else if (*length > 0) {
            write(STDOUT_FILENO, data, *length);
            *length = 0;
            *status = 0;
        }
        
        sem_post(sem);
        usleep(100000);
        iteration++;
    }

    waitpid(client, NULL, 0);
    
    sem_close(sem);
    sem_unlink(sem_name);
    munmap(shm_buf, SHM_SIZE);
    shm_unlink(shm_name);
    close(shm_fd);
    
    return 0;
}