# Rapport de campagne - banc de test temps reel

Genere le 2026-11-18T10:32:16. Valeurs en microsecondes (us).
Un lancement est suspect (pause de la VM) si son maximum depasse 50 ms.

## Jitter, sans charge

| Politique | Noyau | Lancements | Suspects | P50 | P90 | P99 | max |
|---|---|---:|---:|---:|---:|---:|---:|
| SCHED_OTHER | standard | 3 | 2 | 597.6 | 1171.5 | 3052.3 | 6298.7 |
| SCHED_OTHER | PREEMPT_RT | 3 | 1 | 649.5 | 1225.8 | 1597.2 | 4329.1 |
| SCHED_FIFO | standard | 3 | 0 | 646.2 | 1391.9 | 3982.1 | 9846.8 |
| SCHED_FIFO | PREEMPT_RT | 3 | 2 | 513.9 | 1144.7 | 1506.1 | 1846.2 |
| SCHED_RR | standard | 3 | 0 | 714.9 | 3773.4 | 8154.4 | 11670.7 |
| SCHED_RR | PREEMPT_RT | 3 | 2 | 543.6 | 1199.7 | 2780.9 | 4878.0 |
| SCHED_DEADLINE | standard | 3 | 2 | 729.0 | 7255.5 | 13654.2 | 22056.7 |
| SCHED_DEADLINE | PREEMPT_RT | 3 | 2 | 383.0 | 957.5 | 2223.8 | 4364.7 |

## Jitter, charge CPU (stress-ng --cpu 2)

| Politique | Noyau | Lancements | Suspects | P50 | P90 | P99 | max |
|---|---|---:|---:|---:|---:|---:|---:|
| SCHED_OTHER | standard | 3 | 1 | 959.0 | 2459.1 | 5031.5 | 7636.6 |
| SCHED_OTHER | PREEMPT_RT | 3 | 2 | 885.9 | 2156.2 | 6464.0 | 9344.3 |
| SCHED_FIFO | standard | 3 | 2 | 1157.9 | 3246.7 | 6918.0 | 7894.8 |
| SCHED_FIFO | PREEMPT_RT | 3 | 1 | 716.0 | 2144.3 | 5522.2 | 18959.5 |
| SCHED_RR | standard | 3 | 1 | 997.1 | 3139.9 | 6451.4 | 8194.5 |
| SCHED_RR | PREEMPT_RT | 3 | 2 | 672.9 | 1355.2 | 2009.9 | 2984.9 |
| SCHED_DEADLINE | standard | 3 | 1 | 372.1 | 1263.0 | 4864.5 | 8878.0 |
| SCHED_DEADLINE | PREEMPT_RT | 3 | 2 | 499.2 | 1842.7 | 5057.8 | 9908.0 |

## Jitter, charge E/S (stress-ng --io --hdd)

| Politique | Noyau | Lancements | Suspects | P50 | P90 | P99 | max |
|---|---|---:|---:|---:|---:|---:|---:|
| SCHED_OTHER | standard | 3 | 1 | 1157.5 | 6849.6 | 22790.5 | 37265.2 |
| SCHED_OTHER | PREEMPT_RT | 3 | 3 | 975.1 | 46617.9 | 172965.2 | 258330.7 |
| SCHED_FIFO | standard | 3 | 2 | 1167.9 | 5383.4 | 13032.3 | 20394.0 |
| SCHED_FIFO | PREEMPT_RT | 3 | 3 | 892.6 | 16881.9 | 142080.7 | 235046.9 |
| SCHED_RR | standard | 3 | 2 | 1290.6 | 7391.9 | 17504.3 | 38222.2 |
| SCHED_RR | PREEMPT_RT | 3 | 3 | 1043.7 | 36589.9 | 150149.7 | 239416.4 |
| SCHED_DEADLINE | standard | 3 | 0 | 1016.6 | 5880.8 | 14333.0 | 30578.1 |
| SCHED_DEADLINE | PREEMPT_RT | 3 | 3 | 918.0 | 6279.7 | 73327.6 | 199644.6 |

## Latence d'interruption simulee, sans charge

| Politique | Noyau | Lancements | Suspects | P50 | P90 | P99 | max |
|---|---|---:|---:|---:|---:|---:|---:|
| SCHED_OTHER | standard | 3 | 0 | 626.8 | 1415.2 | 3106.1 | 11724.8 |
| SCHED_OTHER | PREEMPT_RT | 3 | 0 | 42.7 | 757.9 | 1582.2 | 4947.7 |
| SCHED_FIFO | standard | 3 | 0 | 73.6 | 1435.8 | 5828.9 | 10695.2 |
| SCHED_FIFO | PREEMPT_RT | 3 | 0 | 60.0 | 532.8 | 1111.5 | 4152.2 |
| SCHED_RR | standard | 3 | 0 | 739.7 | 1233.4 | 2391.9 | 9798.2 |
| SCHED_RR | PREEMPT_RT | 3 | 0 | 95.8 | 1081.0 | 1590.9 | 3923.0 |

## Latence d'interruption simulee, charge CPU

| Politique | Noyau | Lancements | Suspects | P50 | P90 | P99 | max |
|---|---|---:|---:|---:|---:|---:|---:|
| SCHED_OTHER | standard | 3 | 0 | 25.4 | 3099.9 | 10338.8 | 12999.2 |
| SCHED_OTHER | PREEMPT_RT | 3 | 1 | 23.9 | 325.4 | 755.8 | 5417.8 |
| SCHED_FIFO | standard | 3 | 0 | 167.4 | 320.3 | 537.8 | 2821.1 |
| SCHED_FIFO | PREEMPT_RT | 3 | 0 | 30.3 | 165.4 | 453.7 | 2837.6 |
| SCHED_RR | standard | 3 | 0 | 165.0 | 371.6 | 676.5 | 3046.5 |
| SCHED_RR | PREEMPT_RT | 3 | 0 | 223.4 | 444.8 | 1074.1 | 1765.9 |

## Effet de mlockall (mediane par essai)

| Noyau | Charge | Placement | mlock | P50 | P99 | max | fautes mineures |
|---|---|---|---|---:|---:|---:|---:|
| standard | aucune | libre | non | 33.4 | 854.6 | 1920.6 | 3.0 |
| standard | aucune | libre | oui | 314.7 | 1013.3 | 2593.0 | 0.0 |
| standard | aucune | cpu1 | non | 20.9 | 67.6 | 189.5 | 2.0 |
| standard | aucune | cpu1 | oui | 20.8 | 61.8 | 134.5 | 0.0 |
| standard | vm | libre | non | 146.5 | 839.6 | 5787.4 | 3.0 |
| standard | vm | libre | oui | 157.2 | 604.1 | 1828.1 | 0.0 |
| standard | vm | cpu1 | non | 9.0 | 42.5 | 409.7 | 2.0 |
| standard | vm | cpu1 | oui | 8.7 | 28.3 | 434.7 | 0.0 |
| PREEMPT_RT | aucune | libre | non | 115.1 | 2052.0 | 3575.7 | 3.0 |
| PREEMPT_RT | aucune | libre | oui | 82.3 | 1547.3 | 2818.9 | 0.0 |
| PREEMPT_RT | aucune | cpu1 | non | 89.8 | 214.0 | 1299.3 | 2.0 |
| PREEMPT_RT | aucune | cpu1 | oui | 89.0 | 210.0 | 1107.0 | 0.0 |
| PREEMPT_RT | vm | libre | non | 13.5 | 318.6 | 767.9 | 2.0 |
| PREEMPT_RT | vm | libre | oui | 25.9 | 549.8 | 2544.9 | 0.0 |
| PREEMPT_RT | vm | cpu1 | non | 20.1 | 51.9 | 3253.5 | 3.0 |
| PREEMPT_RT | vm | cpu1 | oui | 12.6 | 38.9 | 524.7 | 0.0 |

## Affinite (mediane par essai)

| Noyau | Scenario | Charge | P50 | P99 | max |
|---|---|---|---:|---:|---:|
| standard | libre | aucune | 26.5 | 1116.7 | 2067.5 |
| standard | taskset_cpu0 | aucune | 22.7 | 114.3 | 411.6 |
| standard | taskset_cpu1 | aucune | 21.6 | 154.3 | 337.0 |
| standard | cpu1_irq_sur_cpu0 | aucune | 21.6 | 185.1 | 1149.0 |
| PREEMPT_RT | libre | aucune | 46.5 | 559.2 | 1750.3 |
| PREEMPT_RT | taskset_cpu0 | aucune | 39.0 | 245.9 | 1537.0 |
| PREEMPT_RT | taskset_cpu1 | aucune | 34.2 | 256.0 | 853.6 |
| PREEMPT_RT | cpu1_irq_sur_cpu0 | aucune | 39.6 | 301.1 | 1568.9 |
