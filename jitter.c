/*
 * jitter.c - Mesure du retard d'une tache periodique (10 ms)
 * Mini-projet n°15 - Banc de test temps reel (latence/jitter)
 *
 * Semaine 2 : choix de la politique d'ordonnancement
 *   ./jitter                 -> politique normale (SCHED_OTHER)
 *   sudo ./jitter fifo 80    -> SCHED_FIFO, priorite 80
 *   sudo ./jitter rr 80      -> SCHED_RR,   priorite 80
 *
 * Semaine 4 : option mlock (mlockall, memoire verrouillee en RAM)
 *   sudo ./jitter fifo 80 mlock
 *   sudo ./jitter other mlock
 *
 * Principe : la tache doit se reveiller toutes les 10 ms exactement.
 * A chaque tour on mesure de combien elle est en retard sur l'instant prevu.
 */
#define _GNU_SOURCE
#include <errno.h>
#include <sched.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/resource.h>
 
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
 
/* Nom lisible d'une politique d'ordonnancement */
static const char *policy_name(int policy)
{
    switch (policy) {
    case SCHED_OTHER: return "SCHED_OTHER";
    case SCHED_FIFO:  return "SCHED_FIFO";
    case SCHED_RR:    return "SCHED_RR";
    default:          return "inconnue";
    }
}

/* Memoire verrouillee du processus (ligne VmLck de /proc/self/status) */
static long vmlck_kb(void)
{
    FILE *f = fopen("/proc/self/status", "r");
    char ligne[128];
    long kb = -1;
    if (!f)
        return -1;
    while (fgets(ligne, sizeof ligne, f))
        if (sscanf(ligne, "VmLck: %ld kB", &kb) == 1)
            break;
    fclose(f);
    return kb;
}
 
int main(int argc, char *argv[])
{
    static int64_t lat[N_SAMPLES];
    struct timespec next, now;
 
    /* --- Choix de la politique (argument 1) et de la priorite (argument 2) --- */
    int policy = SCHED_OTHER;
    const char *tag = "other";
    int prio = 0;

    /* L'option "mlock" peut etre placee n'importe ou apres le programme */
    int use_mlock = 0;
    for (int k = 1; k < argc; k++)
        if (strcmp(argv[k], "mlock") == 0)
            use_mlock = 1;
 
    if (argc > 1) {
        if (strcmp(argv[1], "fifo") == 0)       { policy = SCHED_FIFO; tag = "fifo"; }
        else if (strcmp(argv[1], "rr") == 0)    { policy = SCHED_RR;   tag = "rr"; }
        else if (strcmp(argv[1], "other") != 0 && strcmp(argv[1], "mlock") != 0) {
            fprintf(stderr, "Usage : %s [other|fifo|rr] [priorite 1-99] [mlock]\n", argv[0]);
            return 1;
        }
    }
    if (policy != SCHED_OTHER) {
        prio = (argc > 2 && strcmp(argv[2], "mlock") != 0) ? atoi(argv[2]) : 80;
        if (prio < 1 || prio > 99) {
            fprintf(stderr, "La priorite doit etre entre 1 et 99.\n");
            return 1;
        }
        struct sched_param sp = { .sched_priority = prio };
        /* 0 = ce processus. Echoue sans sudo (EPERM). */
        if (sched_setscheduler(0, policy, &sp) == -1) {
            fprintf(stderr, "sched_setscheduler a echoue : %s\n", strerror(errno));
            if (errno == EPERM)
                fprintf(stderr, "-> relancez avec sudo.\n");
            return 1;
        }
    }
    printf("Politique    : %s", policy_name(sched_getscheduler(0)));
    if (policy != SCHED_OTHER) printf(" (priorite %d)", prio);
    printf("\n");

    /* --- mlockall : garde toute la memoire du processus en RAM --- */
    if (use_mlock) {
        if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1) {
            fprintf(stderr, "mlockall a echoue : %s\n", strerror(errno));
            if (errno == EPERM || errno == ENOMEM)
                fprintf(stderr, "-> relancez avec sudo.\n");
            return 1;
        }
        printf("mlockall     : OK (%ld kB verrouilles)\n", vmlck_kb());
    }
 
    /* --- Mesure --- */
    struct rusage r0, r1;
    getrusage(RUSAGE_SELF, &r0);

    clock_gettime(CLOCK_MONOTONIC, &next);
 
    for (int i = 0; i < N_SAMPLES; i++) {
        add_ns(&next, PERIOD_NS);
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL);
        clock_gettime(CLOCK_MONOTONIC, &now);
        lat[i] = diff_ns(&now, &next);
    }

    getrusage(RUSAGE_SELF, &r1);
 
    /* --- Statistiques simples : min, moyenne, max --- */
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
    printf("Fautes_mineures : %ld\n", r1.ru_minflt - r0.ru_minflt);
    printf("Fautes_majeures : %ld\n", r1.ru_majflt - r0.ru_majflt);

    /* --- Un fichier CSV par politique --- */
    char name[64];
    snprintf(name, sizeof name, "latences_%s.csv", tag);
    FILE *f = fopen(name, "w");
    if (f) {
        fprintf(f, "echantillon,latence_ns\n");
        for (int i = 0; i < N_SAMPLES; i++)
            fprintf(f, "%d,%lld\n", i, (long long)lat[i]);
        fclose(f);
        printf("Mesures enregistrees dans %s\n", name);
    }
    return 0;
}
