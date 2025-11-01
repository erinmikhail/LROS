#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#define BUFFER_SIZE 1024

void write_str(int fd, const char *str) {
    write(fd, str, strlen(str));
}

void exit_with_error() {
    write_str(STDERR_FILENO, "Ошибка: деление на ноль\n");
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
    if (argc != 2) {
        write_str(STDERR_FILENO, "Использование: child <filename>\n");
        exit(EXIT_FAILURE);
    }
    
    int file_fd = open(argv[1], O_RDONLY);
    if (file_fd == -1) {
        write_str(STDERR_FILENO, "Ошибка открытия файла\n");
        exit(EXIT_FAILURE);
    }

    dup2(file_fd, STDIN_FILENO);
    close(file_fd);
    
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    char line[BUFFER_SIZE];
    int line_pos = 0;
    
    while ((bytes_read = read(STDIN_FILENO, buffer, sizeof(buffer))) > 0) {
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
                                exit_with_error();
                            }
                            result /= numbers[j];
                        }
                        
                        char num_str[20];
                        int_to_str(result, num_str);
                        write_str(STDOUT_FILENO, num_str);
                        write_str(STDOUT_FILENO, "\n");
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
    
    return 0;
}