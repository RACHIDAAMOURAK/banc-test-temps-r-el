#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "stats.h"

static int cmp_i64(const void *a, const void *b)
{
    int64_t x = *(const int64_t *)a, y = *(const int64_t *)b;
    return (x > y) - (x < y);
}

/* Percentile par rang le plus proche, sur un tableau DEJA trie */
static int64_t percentile(const int64_t *sorted, size_t n, double p)
{
    /* 1e-9 : evite qu'une erreur d'arrondi (ex. 999.0000000000001)
       fasse passer le rang a l'entier superieur */
    size_t rank = (size_t)ceil(p / 100.0 * (double)n - 1e-9);
    if (rank < 1) rank = 1;
    if (rank > n) rank = n;
    return sorted[rank - 1];
}

int stats_compute(const int64_t *data, size_t n, stats_t *out)
{
    if (!data || !out || n == 0)
        return -1;

    int64_t *tmp = malloc(n * sizeof(int64_t));
    if (!tmp)
        return -1;
    memcpy(tmp, data, n * sizeof(int64_t));
    qsort(tmp, n, sizeof(int64_t), cmp_i64);   /* tri obligatoire */

    double sum = 0;
    for (size_t i = 0; i < n; i++)
        sum += (double)tmp[i];

    out->min  = tmp[0];
    out->max  = tmp[n - 1];
    out->mean = sum / (double)n;
    out->p50  = percentile(tmp, n, 50.0);
    out->p90  = percentile(tmp, n, 90.0);
    out->p99  = percentile(tmp, n, 99.0);
    out->p999 = percentile(tmp, n, 99.9);

    free(tmp);
    return 0;
}
