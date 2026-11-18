#!/bin/bash
cd "$(dirname "$0")" || exit 1
make > /dev/null || exit 1
sudo -v || exit 1
pause() { echo; read -r -p ">>> Entree pour continuer " _; clear; }

clear
echo "=== 1. Noyau utilise ==="
uname -r
uname -v
pause

echo "=== 2. Jitter en SCHED_OTHER (10 s) ==="
./jitter && ./percentiles latences_other.csv
pause

echo "=== 3. Jitter en SCHED_FIFO, priorite 80 (10 s) ==="
sudo ./jitter fifo 80 && ./percentiles latences_fifo.csv
pause

echo "=== 4. Latence d'interruption simulee, FIFO 80 ==="
sudo ./irq_sim fifo 80
pause

echo "=== 5. Avec mlockall ==="
sudo ./irq_sim fifo 80 mlock
pause

echo "=== 6. Resultats de la campagne, FIFO sans charge ==="
for f in resultats/*/sans-charge/fifo_*.txt; do
    printf "%-70s" "$f"
    awk '/^(P50|P99|max) /{printf "%s=%s  ", $1, $3}' "$f"
    echo
done
pause

echo "=== 7. Rapport automatise (JSON + graphes) ==="
python3 rapport.py --sans-pauses --seuil-ms 50
xdg-open rapport/sans-charge.png >/dev/null 2>&1 &
