#!/usr/bin/env python3
"""Mediane par groupe d'un CSV de campagne.
Les colonnes avant 'essai' sont les cles du groupe, celles apres sont les mesures."""
import csv
import statistics
import sys
from collections import OrderedDict

with open(sys.argv[1]) as f:
    lignes = list(csv.reader(f))
entete, donnees = lignes[0], lignes[1:]
k = entete.index("essai")

groupes = OrderedDict()
for l in donnees:
    groupes.setdefault(tuple(l[:k]), []).append(l[k + 1:])

print("\t".join(entete[:k] + ["n"] + [c + "_med" for c in entete[k + 1:]]))
for cle, vals in groupes.items():
    meds = ["%.1f" % statistics.median(float(v[j]) for v in vals)
            for j in range(len(vals[0]))]
    print("\t".join(list(cle) + [str(len(vals))] + meds))
