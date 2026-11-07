#!/bin/bash
# affinite.sh : effet de l'affinite (taskset + affinite des IRQ) sur irq_sim.
# Usage : ./affinite.sh [essais]      (defaut : 3)
# Ne pas lancer avec sudo : le script appelle sudo lui-meme.

cd "$(dirname "$0")" || exit 1
NB=${1:-3}
NCPU=$(nproc)
OUT="resultats/$(uname -r)/s4-affinite"
CSV="$OUT/resultats.csv"
SAUVEGARDE=$(mktemp)
mkdir -p "$OUT"

sudo -v || exit 1
make irq_sim > /dev/null || exit 1

if [ "$NCPU" -lt 2 ]; then
    echo "Il faut au moins 2 CPU pour ce test (nproc = $NCPU)."
    exit 1
fi

CPU_A=0          # coeur ou on deplace les interruptions
CPU_B=1          # coeur de mesure isole

# Sauvegarde de l'affinite d'origine de chaque IRQ
for f in /proc/irq/*/smp_affinity_list; do
    echo "$f $(cat "$f" 2>/dev/null)" >> "$SAUVEGARDE"
done

restaurer_irq() {
    while read -r f val; do
        [ -n "$val" ] && echo "$val" | sudo tee "$f" > /dev/null 2>&1
    done < "$SAUVEGARDE"
    rm -f "$SAUVEGARDE"
    pkill stress-ng 2>/dev/null
}
trap restaurer_irq EXIT

deplacer_irq() {      # $1 = coeur vers lequel envoyer toutes les IRQ
    for f in /proc/irq/*/smp_affinity_list; do
        echo "$1" | sudo tee "$f" > /dev/null 2>&1    # echecs ignores (IRQ non modifiables)
    done
}

echo "scenario,charge,essai,p50_us,p99_us,max_us" > "$CSV"

mesurer() {           # $1 = scenario, $2 = charge, reste = commande
    local scen=$1 charge=$2
    shift 2
    for i in $(seq 1 "$NB"); do
        echo "  $scen, charge=$charge, essai $i/$NB"
        sortie=$("$@")
        ligne=$(echo "$sortie" | awk '
            $1=="P50"{p50=$3}
            $1=="P99"{p99=$3}
            $1=="max"{mx=$3}
            END{printf "%s,%s,%s", p50, p99, mx}')
        echo "$scen,$charge,$i,$ligne" >> "$CSV"
    done
}

for charge in aucune cpu; do
    echo "=== Charge : $charge ==="
    if [ "$charge" = "cpu" ]; then
        stress-ng --cpu 8 --timeout 3600s > /dev/null 2>&1 &
        sleep 5
    fi

    mesurer "libre"             "$charge" sudo ./irq_sim fifo 80
    mesurer "taskset_cpu$CPU_A" "$charge" sudo taskset -c $CPU_A ./irq_sim fifo 80
    mesurer "taskset_cpu$CPU_B" "$charge" sudo taskset -c $CPU_B ./irq_sim fifo 80

    deplacer_irq $CPU_A
    mesurer "cpu${CPU_B}_irq_sur_cpu$CPU_A" "$charge" sudo taskset -c $CPU_B ./irq_sim fifo 80
    # on remet les IRQ comme avant pour la charge suivante
    while read -r f val; do
        [ -n "$val" ] && echo "$val" | sudo tee "$f" > /dev/null 2>&1
    done < "$SAUVEGARDE"

    pkill stress-ng 2>/dev/null
    sleep 2
done

echo "Termine. Resultats : $CSV"
