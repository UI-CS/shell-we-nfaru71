#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>

#define MAX_PROC 8

// هر بچه یک بخش از کل کار رو انجام می‌ده
double simulate_pi(long points, unsigned int seed_plus) {
    long inside = 0;
    for (long i = 0; i < points; i++) {
        double x = (double)rand_r(&seed_plus) / RAND_MAX;
        double y = (double)rand_r(&seed_plus) / RAND_MAX;
        if (x * x + y * y <= 1.0)
            inside++;
    }
    return (4.0 * inside) / points;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <num_processes> <num_points>\n", argv[0]);
        return 1;
    }

    int nproc = atoi(argv[1]);
    if (nproc > MAX_PROC) nproc = MAX_PROC;
    long total_points = atol(argv[2]);
    long per_proc = total_points / nproc;

    int fd[2];
    if (pipe(fd) == -1) {
        perror("pipe failed");
        return 1;
    }

    for (int i = 0; i < nproc; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            close(fd[0]); // فقط می‌نویسه
            unsigned int seed = time(NULL) ^ (getpid() << 8);
            double local_pi = simulate_pi(per_proc, seed + i);
            write(fd[1], &local_pi, sizeof(double));
            close(fd[1]);
            exit(0);
        }
        else if (pid < 0) {
            perror("fork error");
            return 1;
        }
    }

    // فرآیند پدر
    close(fd[1]);
    double sum_pi = 0.0;
    for (int i = 0; i < nproc; i++) {
        double part;
        read(fd[0], &part, sizeof(double));
        sum_pi += part;
    }
    close(fd[0]);

    // منتظر بچه‌ها
    while (wait(NULL) > 0);

    double avg_pi = sum_pi / nproc;
    printf("Estimated PI = %.6f using %d process(es)\n", avg_pi, nproc);
    return 0;
}
