#!/usr/bin/env python3
"""rapport.py - Rapport automatise (JSON + graphiques) du banc de test temps reel.

Mini-projet n°15 - Banc de test temps reel (latence/jitter).

Lit le dossier resultats/ produit par les scripts de campagne :
    resultats/<noyau>/<mode>/<politique>_<n>.txt   (modes : sans-charge, charge,
                                                    charge-io, irq, irq-charge)
    resultats/<noyau>/s4-mlock/resultats.csv
    resultats/<noyau>/s4-affinite/resultats.csv

et produit dans le dossier de sortie (par defaut : rapport/) :
    rapport.json   toutes les statistiques (mediane, meilleur lancement, pauses)
    rapport.md     tableaux lisibles
    *.png          graphiques (si matplotlib est installe)

Usage :
    python3 rapport.py [dossier_resultats] [-o dossier_sortie] [--sans-pauses]

Un lancement est dit "suspect" (pause de la VM) quand son maximum depasse 100 ms.
"""
import argparse
import csv
import json
import re
import statistics
import sys
from datetime import datetime
from pathlib import Path

SEUIL_PAUSE_US = 100_000          # max > 100 ms : lancement suspect
CLES = ["min", "moy", "P50", "P90", "P99", "P99.9", "max"]
MODES = ["sans-charge", "charge", "charge-io", "irq", "irq-charge"]
POLITIQUES = ["other", "fifo", "rr", "deadline"]
NOMS_POLITIQUES = {"other": "SCHED_OTHER", "fifo": "SCHED_FIFO",
                   "rr": "SCHED_RR", "deadline": "SCHED_DEADLINE"}
NOMS_MODES = {
    "sans-charge": "Jitter, sans charge",
    "charge": "Jitter, charge CPU (stress-ng --cpu 2)",
    "charge-io": "Jitter, charge E/S (stress-ng --io --hdd)",
    "irq": "Latence d'interruption simulee, sans charge",
    "irq-charge": "Latence d'interruption simulee, charge CPU",
}
COULEURS = {"standard": "#4C78A8", "PREEMPT_RT": "#F58518"}

STAT_RE = re.compile(r"^(min|moy|P50|P90|P99\.9|P99|max)\s*:\s*([0-9.]+)\s*us", re.M)
FICHIER_RE = re.compile(r"^(other|fifo|rr|deadline)_(\d+)\.txt$")


# --------------------------------------------------------------------------
# Lecture des resultats
# --------------------------------------------------------------------------
IGNORER = []   # sous-chaines de noms de noyaux a ignorer (rempli dans main)


def noyaux_retenus(racine):
    return sorted(p for p in racine.iterdir()
                  if p.is_dir() and not any(x in p.name for x in IGNORER))


def type_noyau(nom):
    return "PREEMPT_RT" if "-rt" in nom else "standard"


def lire_stats(chemin):
    """Extrait min, moy, P50, P90, P99, P99.9, max (en us) d'un fichier .txt."""
    texte = Path(chemin).read_text(errors="replace")
    return {nom: float(val) for nom, val in STAT_RE.findall(texte)}


def mediane(valeurs):
    return statistics.median(valeurs) if valeurs else None


def stats_resume(lancements):
    return {c: mediane([l[c] for l in lancements if c in l]) for c in CLES}


def resumer_serie(lancements):
    propres = [l for l in lancements if l.get("max", 0) <= SEUIL_PAUSE_US]
    meilleur = min(lancements, key=lambda l: l.get("max", float("inf")))
    return {
        "n_lancements": len(lancements),
        "n_suspects": len(lancements) - len(propres),
        "mediane": stats_resume(lancements),
        "mediane_sans_pauses": stats_resume(propres) if propres else None,
        "meilleur_lancement": {c: meilleur.get(c) for c in CLES},
    }


def collecter_series(racine):
    """Retourne {(noyau, mode, politique): [stats, ...]}."""
    series = {}
    for dossier_noyau in noyaux_retenus(racine):
        for mode in MODES:
            dossier_mode = dossier_noyau / mode
            if not dossier_mode.is_dir():
                continue
            for fichier in sorted(dossier_mode.glob("*.txt")):
                m = FICHIER_RE.match(fichier.name)
                if not m:
                    continue
                stats = lire_stats(fichier)
                if "max" not in stats:
                    print(f"  ! fichier sans statistiques, ignore : {fichier}", file=sys.stderr)
                    continue
                cle = (dossier_noyau.name, mode, m.group(1))
                series.setdefault(cle, []).append(stats)
    return series


def lire_csv_groupe(chemin):
    """Regroupe un resultats.csv par (colonnes non numeriques) et calcule les medianes."""
    with open(chemin, newline="") as f:
        lignes = list(csv.DictReader(f))
    if not lignes:
        return []
    numeriques = {"essai", "p50_us", "p99_us", "max_us",
                  "fautes_mineures", "fautes_majeures"}
    cles_groupe = [c for c in lignes[0].keys() if c not in numeriques]
    groupes = {}
    for ligne in lignes:
        groupes.setdefault(tuple(ligne[c] for c in cles_groupe), []).append(ligne)
    sortie = []
    for cle, ls in groupes.items():
        entree = dict(zip(cles_groupe, cle))
        entree["n"] = len(ls)
        for metrique in ("p50_us", "p99_us", "max_us", "fautes_mineures", "fautes_majeures"):
            vals = []
            for l in ls:
                try:
                    vals.append(float(l[metrique]))
                except (KeyError, ValueError, TypeError):
                    pass
            entree[metrique + "_med"] = mediane(vals)
        sortie.append(entree)
    return sortie


def collecter_csv(racine, sous_dossier):
    resultat = {}
    for dossier_noyau in noyaux_retenus(racine):
        chemin = dossier_noyau / sous_dossier / "resultats.csv"
        if chemin.is_file():
            resultat[dossier_noyau.name] = lire_csv_groupe(chemin)
    return resultat


# --------------------------------------------------------------------------
# Rapport JSON et Markdown
# --------------------------------------------------------------------------
def construire_json(series, mlock, affinite):
    noyaux = sorted({k[0] for k in series} | set(mlock) | set(affinite))
    liste = []
    for (noyau, mode, politique), lancements in sorted(series.items()):
        entree = {"noyau": noyau, "type_noyau": type_noyau(noyau),
                  "mode": mode, "politique": politique}
        entree.update(resumer_serie(lancements))
        liste.append(entree)
    return {
        "genere_le": datetime.now().isoformat(timespec="seconds"),
        "unite": "us",
        "seuil_pause_us": SEUIL_PAUSE_US,
        "noyaux": [{"nom": n, "type": type_noyau(n)} for n in noyaux],
        "series": liste,
        "mlockall": mlock,
        "affinite": affinite,
    }


def fmt(v):
    return "-" if v is None else f"{v:.1f}"


def ecrire_markdown(rapport, chemin, sans_pauses):
    lignes = ["# Rapport de campagne - banc de test temps reel", "",
              f"Genere le {rapport['genere_le']}. Valeurs en microsecondes (us).",
              f"Un lancement est suspect (pause de la VM) si son maximum depasse "
              f"{rapport['seuil_pause_us'] / 1000:.0f} ms.", ""]
    for mode in MODES:
        entrees = [s for s in rapport["series"] if s["mode"] == mode]
        if not entrees:
            continue
        lignes += [f"## {NOMS_MODES[mode]}", "",
                   "| Politique | Noyau | Lancements | Suspects | P50 | P90 | P99 | max |",
                   "|---|---|---:|---:|---:|---:|---:|---:|"]
        ordre = {p: i for i, p in enumerate(POLITIQUES)}
        for s in sorted(entrees, key=lambda e: (ordre[e["politique"]], e["type_noyau"] != "standard")):
            m = s["mediane_sans_pauses"] if (sans_pauses and s["mediane_sans_pauses"]) else s["mediane"]
            lignes.append(f"| {NOMS_POLITIQUES[s['politique']]} | {s['type_noyau']} | "
                          f"{s['n_lancements']} | {s['n_suspects']} | "
                          f"{fmt(m['P50'])} | {fmt(m['P90'])} | {fmt(m['P99'])} | {fmt(m['max'])} |")
        lignes.append("")
    if rapport["mlockall"]:
        lignes += ["## Effet de mlockall (mediane par essai)", "",
                   "| Noyau | Charge | Placement | mlock | P50 | P99 | max | fautes mineures |",
                   "|---|---|---|---|---:|---:|---:|---:|"]
        for noyau, entrees in rapport["mlockall"].items():
            for e in entrees:
                lignes.append(f"| {type_noyau(noyau)} | {e.get('charge', '')} | {e.get('placement', '')} | "
                              f"{e.get('mlock', '')} | {fmt(e['p50_us_med'])} | {fmt(e['p99_us_med'])} | "
                              f"{fmt(e['max_us_med'])} | {fmt(e['fautes_mineures_med'])} |")
        lignes.append("")
    if rapport["affinite"]:
        lignes += ["## Affinite (mediane par essai)", "",
                   "| Noyau | Scenario | Charge | P50 | P99 | max |",
                   "|---|---|---|---:|---:|---:|"]
        for noyau, entrees in rapport["affinite"].items():
            for e in entrees:
                lignes.append(f"| {type_noyau(noyau)} | {e.get('scenario', '')} | {e.get('charge', '')} | "
                              f"{fmt(e['p50_us_med'])} | {fmt(e['p99_us_med'])} | {fmt(e['max_us_med'])} |")
        lignes.append("")
    Path(chemin).write_text("\n".join(lignes), encoding="utf-8")


# --------------------------------------------------------------------------
# Graphiques
# --------------------------------------------------------------------------
def etiqueter(ax, barres, valeurs):
    """Ecrit la valeur (us) au-dessus de chaque barre."""
    for b, v in zip(barres, valeurs):
        if v:
            ax.annotate(f"{v:.0f}", (b.get_x() + b.get_width() / 2, v),
                        ha="center", va="bottom", fontsize=7,
                        xytext=(0, 2), textcoords="offset points")


def graphique_mode(rapport, mode, dossier, plt, sans_pauses):
    entrees = [s for s in rapport["series"] if s["mode"] == mode]
    if not entrees:
        return None
    politiques = [p for p in POLITIQUES if any(s["politique"] == p for s in entrees)]
    types = [t for t in ("standard", "PREEMPT_RT") if any(s["type_noyau"] == t for s in entrees)]
    fig, axes = plt.subplots(1, 3, figsize=(13, 4.4))
    largeur = 0.8 / len(types)
    for ax, (cle, titre) in zip(axes, [("P50", "P50"), ("P99", "P99"), ("max", "Maximum")]):
        for j, t in enumerate(types):
            valeurs = []
            for p in politiques:
                s = next((e for e in entrees if e["politique"] == p and e["type_noyau"] == t), None)
                if s is None:
                    valeurs.append(0)
                    continue
                m = s["mediane_sans_pauses"] if (sans_pauses and s["mediane_sans_pauses"]) else s["mediane"]
                valeurs.append(m[cle] or 0)
            positions = [i + (j - (len(types) - 1) / 2) * largeur for i in range(len(politiques))]
            barres = ax.bar(positions, valeurs, width=largeur * 0.95, label=t, color=COULEURS[t])
            etiqueter(ax, barres, valeurs)
        ax.set_yscale("log")
        ax.set_ylim(bottom=1)   # base fixe a 1 us : les hauteurs restent comparables
        ax.set_xticks(range(len(politiques)))
        ax.set_xticklabels([NOMS_POLITIQUES[p].replace("SCHED_", "") for p in politiques])
        ax.set_title(titre)
        ax.set_ylabel("microsecondes (echelle log)")
        ax.grid(axis="y", alpha=0.3)
    axes[0].legend(title="Noyau")
    note = "mediane des lancements sans pause" if sans_pauses else "mediane de tous les lancements"
    fig.suptitle(f"{NOMS_MODES[mode]} - {note}")
    fig.tight_layout()
    chemin = dossier / f"{mode}.png"
    fig.savefig(chemin, dpi=150)
    plt.close(fig)
    return chemin


def graphique_mlock(rapport, dossier, plt):
    donnees = rapport["mlockall"]
    if not donnees:
        return None
    categories = []
    for entrees in donnees.values():
        for e in entrees:
            cat = (e.get("charge", ""), e.get("placement", ""))
            if cat not in categories:
                categories.append(cat)
    barres = [(t, m) for t in ("standard", "PREEMPT_RT") for m in ("non", "oui")]
    fig, axes = plt.subplots(1, 2, figsize=(13, 4.4))
    largeur = 0.8 / len(barres)
    for ax, (cle, titre, log) in zip(axes, [("p99_us_med", "P99 (us, echelle log)", True),
                                             ("fautes_mineures_med", "Fautes de page mineures", False)]):
        for j, (t, m) in enumerate(barres):
            valeurs = []
            for cat in categories:
                v = 0
                for noyau, entrees in donnees.items():
                    if type_noyau(noyau) != t:
                        continue
                    for e in entrees:
                        if (e.get("charge", ""), e.get("placement", "")) == cat and e.get("mlock") == m:
                            v = e.get(cle) or 0
                valeurs.append(v)
            positions = [i + (j - (len(barres) - 1) / 2) * largeur for i in range(len(categories))]
            ax.bar(positions, valeurs, width=largeur * 0.95, color=COULEURS[t],
                   hatch="//" if m == "oui" else None, edgecolor="white",
                   label=f"{t}, mlock={m}")
        if log:
            ax.set_yscale("log")
        ax.set_xticks(range(len(categories)))
        ax.set_xticklabels([f"{c}\n{p}" for c, p in categories])
        ax.set_title(titre)
        ax.grid(axis="y", alpha=0.3)
    axes[0].legend(fontsize=8)
    fig.suptitle("Effet de mlockall (irq_sim, SCHED_FIFO 80) - mediane des essais")
    fig.tight_layout()
    chemin = dossier / "mlockall.png"
    fig.savefig(chemin, dpi=150)
    plt.close(fig)
    return chemin


def graphique_affinite(rapport, dossier, plt):
    donnees = rapport["affinite"]
    if not donnees:
        return None
    charges, scenarios = [], []
    for entrees in donnees.values():
        for e in entrees:
            if e.get("charge", "") not in charges:
                charges.append(e.get("charge", ""))
            if e.get("scenario", "") not in scenarios:
                scenarios.append(e.get("scenario", ""))
    types = [t for t in ("standard", "PREEMPT_RT")
             if any(type_noyau(n) == t for n in donnees)]
    fig, axes = plt.subplots(1, len(charges), figsize=(6.5 * len(charges), 4.6), squeeze=False)
    largeur = 0.8 / len(types)
    for ax, charge in zip(axes[0], charges):
        for j, t in enumerate(types):
            valeurs = []
            for sc in scenarios:
                v = 0
                for noyau, entrees in donnees.items():
                    if type_noyau(noyau) != t:
                        continue
                    for e in entrees:
                        if e.get("scenario") == sc and e.get("charge") == charge:
                            v = e.get("p99_us_med") or 0
                valeurs.append(v)
            positions = [i + (j - (len(types) - 1) / 2) * largeur for i in range(len(scenarios))]
            ax.bar(positions, valeurs, width=largeur * 0.95, label=t, color=COULEURS[t])
        ax.set_yscale("log")
        ax.set_xticks(range(len(scenarios)))
        ax.set_xticklabels([s.replace("_", "\n") for s in scenarios], fontsize=8)
        ax.set_title(f"charge : {charge}")
        ax.set_ylabel("P99 (us, echelle log)")
        ax.grid(axis="y", alpha=0.3)
    axes[0][0].legend(title="Noyau")
    fig.suptitle("Effet du placement et de l'affinite des IRQ - P99, mediane des essais")
    fig.tight_layout()
    chemin = dossier / "affinite.png"
    fig.savefig(chemin, dpi=150)
    plt.close(fig)
    return chemin


# --------------------------------------------------------------------------
def main():
    ap = argparse.ArgumentParser(description="Rapport automatise JSON + graphiques")
    ap.add_argument("resultats", nargs="?", default="resultats",
                    help="dossier des resultats (defaut : resultats)")
    ap.add_argument("-o", "--sortie", default="rapport",
                    help="dossier de sortie (defaut : rapport)")
    ap.add_argument("--sans-pauses", action="store_true",
                    help="utiliser la mediane des lancements sans pause (max <= 100 ms)")
    ap.add_argument("--tout", action="store_true",
                    help="inclure aussi les noyaux d'une autre machine (ex. Ubuntu '-generic')")
    args = ap.parse_args()
    if not args.tout:
        IGNORER.append("generic")

    racine = Path(args.resultats)
    if not racine.is_dir():
        sys.exit(f"Dossier introuvable : {racine}")
    sortie = Path(args.sortie)
    sortie.mkdir(parents=True, exist_ok=True)

    series = collecter_series(racine)
    mlock = collecter_csv(racine, "s4-mlock")
    affinite = collecter_csv(racine, "s4-affinite")
    if not series and not mlock and not affinite:
        sys.exit(f"Aucun resultat exploitable dans {racine}")

    rapport = construire_json(series, mlock, affinite)
    (sortie / "rapport.json").write_text(
        json.dumps(rapport, indent=2, ensure_ascii=False), encoding="utf-8")
    ecrire_markdown(rapport, sortie / "rapport.md", args.sans_pauses)
    print(f"{len(rapport['series'])} series lues, noyaux : "
          f"{', '.join(n['nom'] for n in rapport['noyaux'])}")
    print(f"Ecrit : {sortie / 'rapport.json'}")
    print(f"Ecrit : {sortie / 'rapport.md'}")

    try:
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
    except ImportError:
        print("matplotlib absent : graphiques non generes "
              "(sudo apt install python3-matplotlib)", file=sys.stderr)
        return

    produits = [graphique_mode(rapport, m, sortie, plt, args.sans_pauses) for m in MODES]
    produits += [graphique_mlock(rapport, sortie, plt), graphique_affinite(rapport, sortie, plt)]
    for chemin in produits:
        if chemin:
            print(f"Ecrit : {chemin}")


if __name__ == "__main__":
    main()
