# Banc de test temps réel (latence / jitter)

Mini-projet n°15 — Systèmes Embarqués et Temps Réel
ENSAH — GI-GL, 3ème année cycle d'ingenieur, S5
Encadrant : Pr. Nabil KANNOUF — Année universitaire 2026-2027

## Auteurs
- Fatima Zohra EL HAMDANI
- Rachida Amourak

## Objectif
Mesurer la latence et le jitter d'une tâche périodique sous Linux, avec les
politiques SCHED_FIFO, SCHED_RR et SCHED_DEADLINE, puis comparer le noyau
standard et le noyau PREEMPT_RT.

## Compilation et exécution
make
./jitter

## Programmes

| Programme | Rôle |
|---|---|
| `jitter` | Mesure le retard d'une tâche périodique (10 ms), politiques other, fifo, rr |
| `deadline` | Même mesure avec SCHED_DEADLINE (écart entre deux réveils) |
| `percentiles` | Calcule min, moyenne, P50, P90, P99, P99.9 et max d'un fichier CSV |
| `irq_sim` | Simule la latence d'une interruption (générateur → récepteur) |

## Utilisation

    make                          # compile tout
    make test                     # test unitaire du calcul des percentiles

    ./jitter                      # politique normale (SCHED_OTHER)
    sudo ./jitter fifo 80         # SCHED_FIFO, priorité 80
    sudo ./jitter rr 80           # SCHED_RR, priorité 80
    sudo ./deadline               # SCHED_DEADLINE (runtime 2 ms, période 10 ms)

    ./percentiles latences_fifo.csv

Chaque mesure enregistre ses résultats dans un fichier `latences_*.csv`
(non versionné).

## Latence d'interruption (simulation)

    ./irq_sim                     # récepteur en SCHED_OTHER
    sudo ./irq_sim fifo 80        # récepteur en SCHED_FIFO, priorité 80
    sudo ./irq_sim rr 80          # récepteur en SCHED_RR, priorité 80

Un processus générateur simule le matériel : toutes les 10 ms il envoie
son heure t0 dans un tube. Le récepteur lit t0, note l'heure t1 et
enregistre t1 - t0 (fichier `latences_irq_<politique>.csv`).
Sur une Raspberry Pi, le générateur serait remplacé par un signal GPIO.

Pour forcer les deux processus sur le même cœur (affinité) :

    sudo taskset -c 0 ./irq_sim fifo 80

## Limites

Les mesures sont faites dans une machine virtuelle : les pauses de l'hôte
et le coût des réveils entre cœurs influencent les résultats. Toujours
préciser le noyau (`uname -r`) lors d'une comparaison.

## Mesures S3 : noyau standard et PREEMPT_RT
- Machine : VM Debian 13 (VirtualBox), noyaux `6.12.115+deb13.1-amd64` (PREEMPT_DYNAMIC) et `6.12.115+deb13.1-rt-amd64` (PREEMPT_RT)
- Jitter : `./campagne.sh sans-charge` puis `./campagne.sh charge` (3 lancements par politique : OTHER, FIFO 80, RR 80, DEADLINE)
- Latence d'interruption simulée : `./irq_campagne.sh` (avec charge) ; la série sans charge a été lancée à la main avec `./irq_sim`, `./irq_sim fifo 80` et `./irq_sim rr 80`
- Charge : `stress-ng --cpu 2`
- Résultats bruts : `resultats/<noyau>/{sans-charge,charge,irq,irq-charge}/`
- Limite : les pauses de la VM (hôte Windows) faussent certains lancements, qui sont écartés de l'analyse quand leur max dépasse 100 ms
