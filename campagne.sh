#!/bin/bash
cd "$(dirname "$0")" || exit 1
MODE=${1:-sans-charge}
OUT="resultats/$(uname -r)/$MODE"
mkdir -p "$OUT"
sudo -v || exit 1

if [ "$MODE" = "charge" ]; then
    stress-ng --cpu 2 --timeout 600s >/dev/null 2>&1 &
    STRESS=$!
    trap 'kill $STRESS 2>/dev/null' EXIT
    sleep 3
fi

for i in 1 2 3; do
    echo "=== Lancement $i ==="
    ./jitter > "$OUT/other_$i.txt"
    ./percentiles latences_other.csv >> "$OUT/other_$i.txt"
    sudo ./jitter fifo 80 > "$OUT/fifo_$i.txt"
    ./percentiles latences_fifo.csv >> "$OUT/fifo_$i.txt"
    sudo ./jitter rr 80 > "$OUT/rr_$i.txt"
    ./percentiles latences_rr.csv >> "$OUT/rr_$i.txt"
    sudo ./deadline > "$OUT/deadline_$i.txt"
done
echo "Termine. Resultats dans $OUT"
