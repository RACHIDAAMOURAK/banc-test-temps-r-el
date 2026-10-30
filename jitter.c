#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <time.h>

#define PERIOD_NS    10000000L     /* periode : 10 ms */
#define N_SAMPLES    1000          /* nombre de mesures */
#define NSEC_PER_SEC 1000000000L

/* Ajoute ns nanosecondes a t en gerant le debordement des secondes */
static void add_ns(struct timespec *t, long ns)
{
    t->tv_nsec += ns;
    while (t->tv_nsec >= NSEC_PER_SEC) {
        t->tv_nsec -= NSEC_PER_SEC;
        t->tv_sec++;
    }
}

/* Retourne a - b en nanosecondes */
static int64_t diff_ns(const struct timespec *a, const struct timespec *b)
{
    return (int64_t)(a->tv_sec - b->tv_sec) * NSEC_PER_SEC
         + (a->tv_nsec - b->tv_nsec);
}

int main(void)
{
    static int64_t lat[N_SAMPLES];
    struct timespec next, now;

    clock_gettime(CLOCK_MONOTONIC, &next);

    for (int i = 0; i < N_SAMPLES; i++) {
        add_ns(&next, PERIOD_NS);
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL);
        clock_gettime(CLOCK_MONOTONIC, &now);
        lat[i] = diff_ns(&now, &next);
    }

    int64_t min = lat[0], max = lat[0], sum = 0;
    for (int i = 0; i < N_SAMPLES; i++) {
        if (lat[i] < min) min = lat[i];
        if (lat[i] > max) max = lat[i];
        sum += lat[i];
    }

    printf("Echantillons : %d (periode %ld us)\n", N_SAMPLES, PERIOD_NS / 1000);
    printf("Latence min  : %.1f us\n", min / 1000.0);
    printf("Latence moy  : %.1f us\n", (sum / (double)N_SAMPLES) / 1000.0);
    printf("Latence max  : %.1f us\n", max / 1000.0);

    FILE *f = fopen("latences.csv", "w");
    if (f) {
        fprintf(f, "echantillon,latence_ns\n");
        for (int i = 0; i < N_SAMPLES; i++)
            fprintf(f, "%d,%lld\n", i, (long long)lat[i]);
        fclose(f);
        printf("Mesures enregistrees dans latences.csv\n");
    }
    return 0;
}
