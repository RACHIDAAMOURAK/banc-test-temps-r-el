#!/bin/bash
# mlock_campagne.sh : effet de mlockall sur irq_sim (fifo 80).
# Les mesures "sans" et "avec" mlockall sont ALTERNEES, pour qu'un changement
# d'etat de la VM ne soit pas attribue a mlockall.
# Placement : "libre" (le noyau choisit) ou "cpu1" (taskset : generateur et
# recepteur sur le meme coeur, ce qui supprime le bruit du reveil entre coeurs).
# Usage : ./mlock_campagne.sh [essais]     (defaut : 5)
# Ne pas lancer avec sudo : le script appelle sudo lui-meme.

cd "$(dirname "$0")" || exit 1
NB=${1:-5}
OUT="resultats/$(uname -r)/s4-mlock"
CSV="$OUT/resultats.csv"
mkdir -p "$OUT"

sudo -v || exit 1
make irq_sim > /dev/null || exit 1
trap 'pkill stress-ng 2>/dev/null' EXIT

echo "charge,placement,mlock,essai,p50_us,p99_us,max_us,fautes_mineures,fautes_majeures" > "$CSV"

for charge in aucune vm; do
    echo "=== Charge : $charge ==="
    if [ "$charge" = "vm" ]; then
        stress-ng --vm 2 --vm-bytes 70% --timeout 3600s > /dev/null 2>&1 &
        sleep 5
    fi
    for place in libre cpu1; do
        for i in $(seq 1 "$NB"); do
            if [ $((i % 2)) -eq 1 ]; then ordre="non oui"; else ordre="oui non"; fi
            for ml in $ordre; do
                cmd=(sudo)
                [ "$place" = "cpu1" ] && cmd+=(taskset -c 1)
                cmd+=(./irq_sim fifo 80)
                [ "$ml" = "oui" ] && cmd+=(mlock)
                echo "  $place, mlock=$ml, essai $i/$NB"
                sortie=$("${cmd[@]}")
                ligne=$(echo "$sortie" | awk '
                    $1=="P50"{p50=$3}
                    $1=="P99"{p99=$3}
                    $1=="max"{mx=$3}
                    $1=="Fautes_mineures"{fmin=$3}
                    $1=="Fautes_majeures"{fmaj=$3}
                    END{printf "%s,%s,%s,%s,%s", p50, p99, mx, fmin, fmaj}')
                echo "$charge,$place,$ml,$i,$ligne" >> "$CSV"
            done
        done
    done
    pkill stress-ng 2>/dev/null
    sleep 2
done

echo "Termine. Resultats : $CSV"
