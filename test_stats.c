#include <stdio.h>
#include <assert.h>
#include "stats.h"

int main(void)
{
    int64_t d[1000];
    for (int i = 0; i < 1000; i++) d[i] = i + 1;   /* 1, 2, ..., 1000 */

    stats_t s;
    assert(stats_compute(d, 1000, &s) == 0);
    assert(s.min  == 1);
    assert(s.max  == 1000);
    assert(s.p50  == 500);
    assert(s.p90  == 900);
    assert(s.p99  == 990);
    assert(s.p999 == 999);
    assert(stats_compute(d, 0, &s) == -1);          /* cas d'erreur */

    printf("Tous les tests passent.\n");
    return 0;
}
