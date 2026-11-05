#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sched.h>
#include <unistd.h>
#include <sys/syscall.h>
#include "stats.h"

#ifndef SCHED_DEADLINE
#define SCHED_DEADLINE 6
#endif

#define PERIOD_NS    10000000LL   /* periode  : 10 ms */
#define RUNTIME_NS    2000000LL   /* runtime  :  2 ms */
#define DEADLINE_NS  10000000LL   /* deadline : 10 ms */
#define N_SAMPLES    1000
#define NSEC_PER_SEC 1000000000LL

/* Structure attendue par le noyau (pas definie dans la libc) */
/* Depuis la glibc 2.41, elle est deja definie dans <sched.h>. */
#if !defined(__GLIBC__) || __GLIBC__ < 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ < 41)
struct sched_attr {
    uint32_t size;
    uint32_t sched_policy;
    uint64_t sched_flags;
    int32_t  sched_nice;
    uint32_t sched_priority;
    uint64_t sched_runtime;
    uint64_t sched_deadline;
    uint64_t sched_period;
};
#endif
static int sched_setattr_sys(pid_t pid, const struct sched_attr *attr)
{
    return (int)syscall(SYS_sched_setattr, pid, attr, 0);
}

static int64_t now_ns(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (int64_t)t.tv_sec * NSEC_PER_SEC + t.tv_nsec;
}

int main(void)
{
    static int64_t lat[N_SAMPLES];

    struct sched_attr attr;
    memset(&attr, 0, sizeof attr);
    attr.size           = sizeof attr;
    attr.sched_policy   = SCHED_DEADLINE;
    attr.sched_runtime  = RUNTIME_NS;
    attr.sched_deadline = DEADLINE_NS;
    attr.sched_period   = PERIOD_NS;

    if (sched_setattr_sys(0, &attr) != 0) {
        fprintf(stderr, "sched_setattr a echoue : %s\n", strerror(errno));
        fprintf(stderr, "(EPERM : lance avec sudo ; EBUSY : le noyau refuse ces parametres)\n");
        return 1;
    }

    sched_yield();                 /* se cale sur le debut d'une periode */
    int64_t prev = now_ns();

    for (int i = 0; i < N_SAMPLES; i++) {
        sched_yield();             /* rend la main jusqu'a la prochaine periode */
        int64_t t = now_ns();
        int64_t ecart = (t - prev) - PERIOD_NS;   /* ecart a la periode */
        lat[i] = ecart < 0 ? -ecart : ecart;      /* valeur absolue : jitter */
        prev = t;
    }

    stats_t s;
    if (stats_compute(lat, N_SAMPLES, &s) != 0) {
        fprintf(stderr, "Erreur de calcul des statistiques\n");
        return 1;
    }

    printf("SCHED_DEADLINE : runtime 2 ms, deadline 10 ms, periode 10 ms\n");
    printf("(jitter = |intervalle entre deux reveils - periode|)\n");
    printf("min  : %8.1f us\n", s.min  / 1000.0);
    printf("moy  : %8.1f us\n", s.mean / 1000.0);
    printf("P50  : %8.1f us\n", s.p50  / 1000.0);
    printf("P90  : %8.1f us\n", s.p90  / 1000.0);
    printf("P99  : %8.1f us\n", s.p99  / 1000.0);
    printf("P99.9: %8.1f us\n", s.p999 / 1000.0);
    printf("max  : %8.1f us\n", s.max  / 1000.0);

    FILE *f = fopen("latences_deadline.csv", "w");
    if (f) {
        fprintf(f, "echantillon,latence_ns\n");
        for (int i = 0; i < N_SAMPLES; i++)
            fprintf(f, "%d,%lld\n", i, (long long)lat[i]);
        fclose(f);
        printf("Mesures enregistrees dans latences_deadline.csv\n");
    }
    return 0;
}
