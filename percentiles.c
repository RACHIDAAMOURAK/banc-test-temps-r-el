#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "stats.h"

int main(int argc, char **argv)
{
    const char *fichier = (argc > 1) ? argv[1] : "latences.csv";
    FILE *f = fopen(fichier, "r");
    if (!f) {
        perror(fichier);
        return 1;
    }

    size_t cap = 1024, n = 0;
    int64_t *data = malloc(cap * sizeof(int64_t));
    if (!data) {
        fclose(f);
        return 1;
    }

    char ligne[128];
    if (!fgets(ligne, sizeof ligne, f)) {        /* saute l'en-tete */
        fprintf(stderr, "Fichier vide : %s\n", fichier);
        fclose(f);
        free(data);
        return 1;
    }

    while (fgets(ligne, sizeof ligne, f)) {
        int idx;
        long long ns;
        if (sscanf(ligne, "%d,%lld", &idx, &ns) == 2) {
            if (n == cap) {
                cap *= 2;
                int64_t *nouveau = realloc(data, cap * sizeof(int64_t));
                if (!nouveau) {
                    free(data);                  /* evite la fuite memoire */
                    fclose(f);
                    return 1;
                }
                data = nouveau;
            }
            data[n++] = ns;
        }
    }
    fclose(f);

    stats_t s;
    if (stats_compute(data, n, &s) != 0) {
        fprintf(stderr, "Pas de donnees dans %s\n", fichier);
        free(data);
        return 1;
    }

    printf("Fichier : %s (%zu echantillons)\n", fichier, n);
    printf("min  : %8.1f us\n", s.min  / 1000.0);
    printf("moy  : %8.1f us\n", s.mean / 1000.0);
    printf("P50  : %8.1f us\n", s.p50  / 1000.0);
    printf("P90  : %8.1f us\n", s.p90  / 1000.0);
    printf("P99  : %8.1f us\n", s.p99  / 1000.0);
    printf("P99.9: %8.1f us\n", s.p999 / 1000.0);
    printf("max  : %8.1f us\n", s.max  / 1000.0);

    free(data);
    return 0;
}
