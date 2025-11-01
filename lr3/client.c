#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <fcntl.h>

#define SHM_SIZE 4096
#define BUFFER_SIZE 1024

void write_str(int fd, const char *str) {
    write(fd, str, strlen(str));
}

void exit_with_error(char *shm_buf, sem_t *sem) {
    uint32_t *length = (uint32_t *)shm_buf;
    uint32_t *status = (uint32_t *)(shm_buf + sizeof(uint32_t));
    
    sem_wait(sem);
    *length = 0;
    *status = 1; 
    sem_post(sem);
    
    exit(EXIT_FAILURE);
}

void int_to_str(int num, char *str) {
    if (num == 0) {
        str[0] = '0';
        str[1] = '\0';
        return;
    }
    
    int i = 0;
    int is_negative = 0;
    
    if (num < 0) {
        is_negative = 1;
        num = -num;
    }

    while (num > 0) {
        str[i++] = '0' + (num % 10);
        num /= 10;
    }
    
    if (is_negative) {
        str[i++] = '-';
    }
    
    str[i] = '\0';

    int len = i;
    for (int j = 0; j < len / 2; j++) {
        char temp = str[j];
        str[j] = str[len - 1 - j];
        str[len - 1 - j] = temp;
    }
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        write_str(STDERR_FILENO, "Использование: client <shm_name> <sem_name> <filename>\n");
        exit(EXIT_FAILURE);
    }
    
    const char *shm_name = argv[1];
    const char *sem_name = argv[2];
    const char *filename = argv[3];

    int shm_fd = shm_open(shm_name, O_RDWR, 0);
    if (shm_fd == -1) {
        write_str(STDERR_FILENO, "error: failed to open SHM\n");
        exit(EXIT_FAILURE);
    }

    char *shm_buf = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_buf == MAP_FAILED) {
        write_str(STDERR_FILENO, "error: failed to map SHM\n");
        close(shm_fd);
        exit(EXIT_FAILURE);
    }

    sem_t *sem = sem_open(sem_name, O_RDWR);
    if (sem == SEM_FAILED) {
        write_str(STDERR_FILENO, "error: failed to open semaphore\n");
        munmap(shm_buf, SHM_SIZE);
        close(shm_fd);
        exit(EXIT_FAILURE);
    }

    int file_fd = open(filename, O_RDONLY);
    if (file_fd == -1) {
        write_str(STDERR_FILENO, "Ошибка открытия файла\n");
        sem_close(sem);
        munmap(shm_buf, SHM_SIZE);
        close(shm_fd);
        exit(EXIT_FAILURE);
    }

    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    char line[BUFFER_SIZE];
    int line_pos = 0;
    
    while ((bytes_read = read(file_fd, buffer, sizeof(buffer))) > 0) {
        for (int i = 0; i < bytes_read; i++) {
            if (buffer[i] == '\n' || buffer[i] == '\r') {
                if (line_pos > 0) {
                    line[line_pos] = '\0';
                    
                    int numbers[100];
                    int count = 0;
                    char *token = line;
                    
                    while (*token && count < 100) {
                        while (*token == ' ' || *token == '\t') token++;
                        if (!*token) break;
                        
                        int num = 0;
                        int is_negative = 0;
                        
                        if (*token == '-') {
                            is_negative = 1;
                            token++;
                        }
                        
                        while (*token >= '0' && *token <= '9') {
                            num = num * 10 + (*token - '0');
                            token++;
                        }
                        
                        if (is_negative) {
                            num = -num;
                        }
                        
                        numbers[count++] = num;
                        
                        while (*token == ' ' || *token == '\t') token++;
                    }
                    
                    if (count >= 2) {
                        int result = numbers[0];
                        
                        for (int j = 1; j < count; j++) {
                            if (numbers[j] == 0) {
                                exit_with_error(shm_buf, sem);
                            }
                            result /= numbers[j];
                        }

                        char num_str[20];
                        int_to_str(result, num_str);
                        strcat(num_str, "\n");
                        
                        sem_wait(sem);
                        uint32_t *length = (uint32_t *)shm_buf;
                        uint32_t *status = (uint32_t *)(shm_buf + sizeof(uint32_t));
                        char *data = shm_buf + 2 * sizeof(uint32_t);
                        
                        size_t result_len = strlen(num_str);
                        if (result_len < SHM_SIZE - 2 * sizeof(uint32_t)) {
                            *length = result_len;
                            *status = 0;
                            memcpy(data, num_str, result_len);
                        }
                        sem_post(sem);
                        
                        sleep(1);
                    }
                    
                    line_pos = 0;
                }
            } else {
                if (line_pos < sizeof(line) - 1) {
                    line[line_pos++] = buffer[i];
                }
            }
        }
    }
    
    close(file_fd);
    sem_close(sem);
    munmap(shm_buf, SHM_SIZE);
    close(shm_fd);
    
    return 0;
}