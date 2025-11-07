#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <string.h>
#include <stdio.h>

typedef struct {
    double real;
    double imag;
} Complex;

typedef struct {
    size_t thread_id;
    size_t n_threads;
    size_t start_row;
    size_t end_row;
    size_t n;
    Complex **A;
    Complex **B;
    Complex **C;
} ThreadArgs;

// 1: Последовательный алгоритм
// Особенности: 
// выполняется в одном потоке, нет накладных расходов на создание потоков, эталон для проверки.

Complex complex_create(double real, double imag) {
    Complex c;
    c.real = real;
    c.imag = imag;
    return c;
}

Complex complex_add(Complex a, Complex b) {
    return complex_create(a.real + b.real, a.imag + b.imag);
}

Complex complex_multiply(Complex a, Complex b) {
    return complex_create(a.real * b.real - a.imag * b.imag,
                         a.real * b.imag + a.imag * b.real);
}

Complex** matrix_alloc(size_t rows, size_t cols) {
    Complex **matrix = malloc(rows * sizeof(Complex*));
    for (size_t i = 0; i < rows; i++) {
        matrix[i] = malloc(cols * sizeof(Complex));
    }
    return matrix;
}

void matrix_free(Complex **matrix, size_t rows) {
    for (size_t i = 0; i < rows; i++) {
        free(matrix[i]);
    }
    free(matrix);
}

void matrix_init_random(Complex **matrix, size_t rows, size_t cols) {
    for (size_t i = 0; i < rows; i++) {
        for (size_t j = 0; j < cols; j++) {
            matrix[i][j] = complex_create(
                (double)rand() / RAND_MAX * 10.0,
                (double)rand() / RAND_MAX * 10.0
            );
        }
    }
}

void matrix_multiply_sequential(Complex **A, Complex **B, Complex **C, 
                               size_t n, size_t m, size_t p) {
    for (size_t i = 0; i < n; i++) {
        for (size_t j = 0; j < p; j++) {
            C[i][j] = complex_create(0, 0);
            for (size_t k = 0; k < m; k++) {
                Complex product = complex_multiply(A[i][k], B[k][j]);
                C[i][j] = complex_add(C[i][j], product);
            }
        }
    }
}

// 2: Параллельный алгоритм
// особенности: 
// блочное распределение, каждый поток работает со своей областью памяти

static void *matrix_multiply_worker(void *_args) {
    ThreadArgs *args = (ThreadArgs*)_args;
    
    for (size_t i = args->start_row; i < args->end_row; i++) {
        for (size_t j = 0; j < args->n; j++) {
            args->C[i][j] = complex_create(0, 0);
            for (size_t k = 0; k < args->n; k++) {
                Complex product = complex_multiply(args->A[i][k], args->B[k][j]);
                args->C[i][j] = complex_add(args->C[i][j], product);
            }
        }
    }
    
    printf("Поток #%zu завершил строки [%zu-%zu]\n", 
           args->thread_id, args->start_row, args->end_row - 1);
    return NULL;
}

void matrix_multiply_parallel(Complex **A, Complex **B, Complex **C, 
                             size_t n, size_t max_threads) {
    pthread_t *threads = malloc(max_threads * sizeof(pthread_t));
    ThreadArgs *thread_args = malloc(max_threads * sizeof(ThreadArgs));
    
    size_t rows_per_thread = n / max_threads;
    size_t remaining_rows = n % max_threads;
    
    size_t current_row = 0;
    for (size_t i = 0; i < max_threads; i++) {
        size_t thread_rows = rows_per_thread;
        if (i < remaining_rows) {
            thread_rows++;
        }
        
        thread_args[i] = (ThreadArgs){
            .thread_id = i,
            .n_threads = max_threads,
            .start_row = current_row,
            .end_row = current_row + thread_rows,
            .n = n,
            .A = A,
            .B = B,
            .C = C
        };
        
        pthread_create(&threads[i], NULL, matrix_multiply_worker, &thread_args[i]);
        current_row += thread_rows;
    }
    
    for (size_t i = 0; i < max_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    free(thread_args);
    free(threads);
}

double get_current_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec * 1e-6;
}

bool matrices_equal(Complex **A, Complex **B, size_t n, double tolerance) {
    for (size_t i = 0; i < n; i++) {
        for (size_t j = 0; j < n; j++) {
            double diff_real = A[i][j].real - B[i][j].real;
            double diff_imag = A[i][j].imag - B[i][j].imag;
            if (diff_real * diff_real + diff_imag * diff_imag > tolerance * tolerance) {
                return false;
            }
        }
    }
    return true;
}

int main(int argc, char **argv) {
    if (argc != 4) {
        printf("Использование: %s <размер_матрицы> <макс_потоков> <показать_инфо(0/1)>\n", argv[0]);
        printf("Пример: %s 100 4 1\n", argv[0]);
        return 1;
    }
    
    size_t n = atoi(argv[1]);
    size_t max_threads = atoi(argv[2]);
    int show_info = atoi(argv[3]);
    
    if (n == 0 || max_threads == 0) {
        printf("Ошибка: неверные параметры\n");
        return 1;
    }
    
    size_t available_cores = sysconf(_SC_NPROCESSORS_ONLN);
    if (max_threads > available_cores) {
        max_threads = available_cores;
        printf("Предупреждение: ограничение потоков до %zu (доступные ядра)\n", max_threads);
    }
    
    printf("=== Перемножение комплексных матриц (Linux/POSIX) ===\n");
    printf("Размер матрицы: %zu x %zu\n", n, n);
    printf("Максимальное количество потоков: %zu\n", max_threads);
    printf("Доступные ядра процессора: %zu\n", available_cores);
    printf("\n");
    
    Complex **A = matrix_alloc(n, n);
    Complex **B = matrix_alloc(n, n);
    Complex **C_seq = matrix_alloc(n, n);
    Complex **C_par = matrix_alloc(n, n);
    
    srand(42);
    matrix_init_random(A, n, n);
    matrix_init_random(B, n, n);
    
    printf("= ВЕРСИЯ 1: Последовательный алгоритм =\n");
    printf("Запуск последовательной версии...\n");
    double start_time = get_current_time();
    matrix_multiply_sequential(A, B, C_seq, n, n, n);
    double seq_time = get_current_time() - start_time;
    printf("Время последовательной версии: %.6f секунд\n", seq_time);
    

    printf("\n= ВЕРСИЯ 2: Параллельный алгоритм (POSIX Threads) =\n");
    printf("Запуск параллельных версий с разным количеством потоков:\n");
    printf("Потоки\tВремя(с)\tУскорение\tЭффективность\tКорректность\n");
    printf("------\t--------\t---------\t-------------\t------------\n");
    
    double best_parallel_time = seq_time;
    size_t best_thread_count = 1;
    
    for (size_t threads = 1; threads <= max_threads; threads++) {
        for (size_t i = 0; i < n; i++) {
            for (size_t j = 0; j < n; j++) {
                C_par[i][j] = complex_create(0, 0);
            }
        }
        
        printf("%zu\t", threads);
        fflush(stdout);
        
        start_time = get_current_time();
        matrix_multiply_parallel(A, B, C_par, n, threads);
        double par_time = get_current_time() - start_time;
        
        double speedup = seq_time / par_time;
        double efficiency = speedup / threads * 100;
        
        printf("%.6f\t%.3f\t\t%.1f%%", par_time, speedup, efficiency);
        
        if (matrices_equal(C_seq, C_par, n, 1e-6)) {
            printf("\t✓ корректно\n");
        } else {
            printf("\t✗ ОШИБКА\n");
        }
        
        if (par_time < best_parallel_time) {
            best_parallel_time = par_time;
            best_thread_count = threads;
        }
    }
    
    printf("\n= СРАВНЕНИЕ ДВУХ ВЕРСИЙ АЛГОРИТМА =\n");
    printf("Последовательная версия: %.6f сек\n", seq_time);
    printf("Лучшая параллельная версия (%zu потоков): %.6f сек\n", best_thread_count, best_parallel_time);
    printf("Максимальное ускорение: %.3f раз\n", seq_time / best_parallel_time);
    printf("Эффективность: %.1f%%\n", (seq_time / best_parallel_time) / best_thread_count * 100);
    
    if (show_info) {
        printf("\n= ОСНОВНЫЕ ОТЛИЧИЯ ВЕРСИЙ =\n");
        printf("Версия 1 (Последовательная):\n");
        printf("  - 1 поток выполнения\n");
        
        printf("Версия 2 (Параллельная - POSIX):\n");
        printf("  - Многопоточное выполнение (pthreads)\n");
        printf("  - Блочное распределение работы\n");
    }

    matrix_free(A, n);
    matrix_free(B, n);
    matrix_free(C_seq, n);
    matrix_free(C_par, n);
    
    return 0;
}