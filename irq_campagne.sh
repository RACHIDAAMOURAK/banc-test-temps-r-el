#!/bin/bash
cd "$(dirname "$0")" || exit 1
OUT="resultats/$(uname -r)/irq-charge"
mkdir -p "$OUT"
sudo -v || exit 1
stress-ng --cpu 2 --timeout 600s >/dev/null 2>&1 &
STRESS=$!
trap 'kill $STRESS 2>/dev/null' EXIT
sleep 3
for i in 1 2 3; do
    echo "=== Lancement $i ==="
    ./irq_sim > "$OUT/other_$i.txt"
    sudo ./irq_sim fifo 80 > "$OUT/fifo_$i.txt"
    sudo ./irq_sim rr 80 > "$OUT/rr_$i.txt"
done
echo "Termine. Resultats dans $OUT"
