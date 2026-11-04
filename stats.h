#ifndef STATS_H
#define STATS_H

#include <stdint.h>
#include <stddef.h>

/* Resultats statistiques, tous en nanosecondes (sauf mean) */
typedef struct {
    int64_t min, max;
    int64_t p50, p90, p99, p999;
    double  mean;
} stats_t;

/* Calcule les statistiques de data[0..n-1].
   Retourne 0 si OK, -1 en cas d'erreur. data n'est pas modifie. */
int stats_compute(const int64_t *data, size_t n, stats_t *out);

#endif
