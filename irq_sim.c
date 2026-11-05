#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sched.h>
#include <unistd.h>
#include <sys/wait.h>
#include "stats.h"

/*
 * Simulation de latence d'interruption.
 *   ./irq_sim                 -> recepteur en SCHED_OTHER
 *   sudo ./irq_sim fifo 80    -> recepteur en SCHED_FIFO, priorite 80
 *   sudo ./irq_sim rr 80      -> recepteur en SCHED_RR,   priorite 80
 * Un generateur (processus fils) envoie son heure t0 dans un tube
 * toutes les 10 ms ; le recepteur mesure t1 - t0.
 */

#define PERIOD_NS    10000000LL   /* periode des "interruptions" : 10 ms */
#define N_SAMPLES    1000
#define NSEC_PER_SEC 1000000000LL

static int64_t now_ns(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (int64_t)t.tv_sec * NSEC_PER_SEC + t.tv_nsec;
}

/* Processus "materiel" : emet un evenement toutes les 10 ms */
static void generateur(int fd)
{
    struct timespec next;
    clock_gettime(CLOCK_MONOTONIC, &next);

    for (int i = 0; i < N_SAMPLES; i++) {
        next.tv_nsec += PERIOD_NS;
        while (next.tv_nsec >= NSEC_PER_SEC) {
            next.tv_nsec -= NSEC_PER_SEC;
            next.tv_sec++;
        }
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL);

        int64_t t0 = now_ns();                 /* instant de l'evenement */
        if (write(fd, &t0, sizeof t0) != (ssize_t)sizeof t0) {
            perror("write");
            break;
        }
    }
    close(fd);
}

int main(int argc, char *argv[])
{
    static int64_t lat[N_SAMPLES];
    int policy = SCHED_OTHER;
    int prio = 0;
    const char *tag = "other";

    if (argc > 1) {
        if (strcmp(argv[1], "fifo") == 0)       { policy = SCHED_FIFO; tag = "fifo"; }
        else if (strcmp(argv[1], "rr") == 0)    { policy = SCHED_RR;   tag = "rr"; }
        else if (strcmp(argv[1], "other") != 0) {
            fprintf(stderr, "Usage : %s [other|fifo|rr] [priorite 1-99]\n", argv[0]);
            return 1;
        }
    }
    if (policy != SCHED_OTHER)
        prio = (argc > 2) ? atoi(argv[2]) : 80;

    int fd[2];
    if (pipe(fd) == -1) {
        perror("pipe");
        return 1;
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {                            /* fils : le generateur */
        close(fd[0]);
        generateur(fd[1]);
        _exit(0);
    }

    /* pere : le recepteur */
    close(fd[1]);

    if (policy != SCHED_OTHER) {
        struct sched_param sp;
        memset(&sp, 0, sizeof sp);
        sp.sched_priority = prio;
        if (sched_setscheduler(0, policy, &sp) == -1) {
            fprintf(stderr, "sched_setscheduler a echoue : %s (lancer avec sudo)\n",
                    strerror(errno));
            close(fd[0]);
            waitpid(pid, NULL, 0);
            return 1;
        }
    }

    int n = 0;
    while (n < N_SAMPLES) {
        int64_t t0;
        ssize_t r = read(fd[0], &t0, sizeof t0);   /* attend l'evenement */
        if (r != (ssize_t)sizeof t0)
            break;
        lat[n++] = now_ns() - t0;                  /* t1 - t0 */
    }
    close(fd[0]);
    waitpid(pid, NULL, 0);

    stats_t s;
    if (stats_compute(lat, (size_t)n, &s) != 0) {
        fprintf(stderr, "Pas de mesures\n");
        return 1;
    }

    printf("Latence d'interruption simulee : recepteur en %s", tag);
    if (policy != SCHED_OTHER) printf(" (priorite %d)", prio);
    printf("\nEchantillons : %d\n", n);
    printf("min  : %8.1f us\n", s.min  / 1000.0);
    printf("moy  : %8.1f us\n", s.mean / 1000.0);
    printf("P50  : %8.1f us\n", s.p50  / 1000.0);
    printf("P90  : %8.1f us\n", s.p90  / 1000.0);
    printf("P99  : %8.1f us\n", s.p99  / 1000.0);
    printf("P99.9: %8.1f us\n", s.p999 / 1000.0);
    printf("max  : %8.1f us\n", s.max  / 1000.0);

    char nom[64];
    snprintf(nom, sizeof nom, "latences_irq_%s.csv", tag);
    FILE *f = fopen(nom, "w");
    if (f) {
        fprintf(f, "echantillon,latence_ns\n");
        for (int i = 0; i < n; i++)
            fprintf(f, "%d,%lld\n", i, (long long)lat[i]);
        fclose(f);
        printf("Mesures enregistrees dans %s\n", nom);
    }
    return 0;
}
