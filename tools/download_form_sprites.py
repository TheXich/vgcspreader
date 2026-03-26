"""
Download alternate-form sprites for all Pokémon that have multiple forms but are
missing sprites.  Uses the same Smogon minisprite art style as the base sprites.

Usage (run from project root):
    python tools/download_form_sprites.py

The script:
  1. Checks which form sprites are already present in db/sprites/.
  2. Downloads missing ones from Smogon (primary) or PS live server (fallback).
  3. Names them {dex}-{form_index}.png to match the binary database.
  4. Updates resources.qrc automatically.
"""

import os
import time
import urllib.request
import urllib.error

SPRITES_DIR = "db/sprites"
QRC_FILE = "resources.qrc"

URLS = [
    "https://www.smogon.com/forums/media/minisprites/{name}.png",
    "https://play.pokemonshowdown.com/sprites/gen5/{name}.png",
]

# ---------------------------------------------------------------------------
# Form mapping: (dex_number, form_index) -> smogon/PS sprite name
# form_index matches the binary database (1 = first alternate form, etc.)
# Names use lowercase with hyphens, matching PS sprite naming conventions.
# ---------------------------------------------------------------------------
FORMS = {
    # ── GEN 1 ──────────────────────────────────────────────────────────────
    # 3  Venusaur   (mega, gmax)
    (3,  1): "venusaur-mega",
    (3,  2): "venusaur-gmax",
    # 6  Charizard  (mega-x, mega-y already present; gmax)
    (6,  3): "charizard-gmax",
    # 9  Blastoise  (mega already present; gmax)
    (9,  2): "blastoise-gmax",
    # 12 Butterfree (gmax)
    (12, 1): "butterfree-gmax",
    # 20 Raticate   (alola already present; alola-totem)
    (20, 2): "raticate-totem-alola",
    # 52 Meowth     (alola already present; galar, gmax)
    (52, 2): "meowth-galar",
    (52, 3): "meowth-gmax",
    # 58 Growlithe  (hisui)
    (58, 1): "growlithe-hisui",
    # 59 Arcanine   (hisui)
    (59, 1): "arcanine-hisui",
    # 68 Machamp    (gmax)
    (68, 1): "machamp-gmax",
    # 77 Ponyta     (galar)
    (77, 1): "ponyta-galar",
    # 78 Rapidash   (galar)
    (78, 1): "rapidash-galar",
    # 79 Slowpoke   (galar)
    (79, 1): "slowpoke-galar",
    # 80 Slowbro    (galar, mega)
    (80, 1): "slowbro-galar",
    (80, 2): "slowbro-mega",
    # 83 Farfetch'd (galar)
    (83, 1): "farfetchd-galar",
    # 94 Gengar     (mega already present; gmax)
    (94, 2): "gengar-gmax",
    # 99 Kingler    (gmax)
    (99, 1): "kingler-gmax",
    # 100 Voltorb   (hisui)
    (100, 1): "voltorb-hisui",
    # 101 Electrode (hisui)
    (101, 1): "electrode-hisui",
    # 105 Marowak   (alola already present; alola-totem)
    (105, 2): "marowak-totem-alola",
    # 110 Weezing   (galar)
    (110, 1): "weezing-galar",
    # 122 Mr. Mime  (galar)
    (122, 1): "mr-mime-galar",
    # 128 Tauros    (paldea forms: combat, blaze, aqua)
    (128, 1): "tauros-paldea-combat",
    (128, 2): "tauros-paldea-blaze",
    (128, 3): "tauros-paldea-aqua",
    # 131 Lapras    (gmax)
    (131, 1): "lapras-gmax",
    # 133 Eevee     (starter, gmax)
    (133, 1): "eevee-starter",
    (133, 2): "eevee-gmax",
    # 143 Snorlax   (gmax)
    (143, 1): "snorlax-gmax",
    # 144 Articuno  (galar)
    (144, 1): "articuno-galar",
    # 145 Zapdos    (galar)
    (145, 1): "zapdos-galar",
    # 146 Moltres   (galar)
    (146, 1): "moltres-galar",

    # ── GEN 2 ──────────────────────────────────────────────────────────────
    # 157 Typhlosion  (hisui)
    (157, 1): "typhlosion-hisui",
    # 172 Pichu       (spiky-eared)
    (172, 1): "pichu-spiky-eared",
    # 194 Wooper      (paldea)
    (194, 1): "wooper-paldea",
    # 199 Slowking    (galar)
    (199, 1): "slowking-galar",
    # 211 Qwilfish    (hisui)
    (211, 1): "qwilfish-hisui",
    # 215 Sneasel     (hisui)
    (215, 1): "sneasel-hisui",
    # 222 Corsola     (galar)
    (222, 1): "corsola-galar",

    # ── GEN 3 ──────────────────────────────────────────────────────────────
    # 263 Zigzagoon   (galar)
    (263, 1): "zigzagoon-galar",
    # 264 Linoone     (galar)
    (264, 1): "linoone-galar",
    # 359 Absol       (mega already present; gmax if it exists)
    # 445 Garchomp    (mega already present)
    # 448 Lucario     (mega already present; gmax)
    (448, 2): "lucario-gmax",
    # 460 Abomasnow   (mega already present)

    # ── GEN 4 ──────────────────────────────────────────────────────────────
    # 483 Dialga      (origin — hisui)
    (483, 1): "dialga-origin",
    # 484 Palkia      (origin — hisui)
    (484, 1): "palkia-origin",
    # 646 Kyurem      (black already present as 646-1; white)
    (646, 2): "kyurem-white",

    # ── GEN 5 ──────────────────────────────────────────────────────────────
    # 503 Samurott    (hisui)
    (503, 1): "samurott-hisui",
    # 549 Lilligant   (hisui)
    (549, 1): "lilligant-hisui",
    # 550 Basculin    (blue-striped already present; white-striped)
    (550, 2): "basculin-white-striped",
    # 554 Darumaka    (galar)
    (554, 1): "darumaka-galar",
    # 555 Darmanitan  (galar already present as 555-1; galar-zen, zen)
    (555, 2): "darmanitan-galar-zen",
    (555, 3): "darmanitan-zen",
    # 562 Yamask      (galar)
    (562, 1): "yamask-galar",
    # 569 Garbodor    (gmax)
    (569, 1): "garbodor-gmax",
    # 570 Zorua       (hisui)
    (570, 1): "zorua-hisui",
    # 571 Zoroark     (hisui)
    (571, 1): "zoroark-hisui",
    # 618 Stunfisk    (galar)
    (618, 1): "stunfisk-galar",
    # 628 Braviary    (hisui)
    (628, 1): "braviary-hisui",

    # ── GEN 6 ──────────────────────────────────────────────────────────────
    # 658 Greninja    (battle-bond, ash) [658-1, 658-2 already present]
    (658, 3): "greninja-ash",
    # 668 Pyroar      (female)
    (668, 1): "pyroar-f",
    # 713 Avalugg     (hisui)
    (713, 1): "avalugg-hisui",
    # 724 Decidueye   (hisui)
    (724, 1): "decidueye-hisui",

    # ── GEN 7 ──────────────────────────────────────────────────────────────
    # 745 Lycanroc    (midnight already present as 745-1; dusk)
    (745, 2): "lycanroc-dusk",
    # 807 Zeraora     (gmax)
    (807, 1): "zeraora-gmax",
    # 809 Melmetal    (gmax)
    (809, 1): "melmetal-gmax",

    # ── GEN 8 ──────────────────────────────────────────────────────────────
    # Starter G-Max forms
    (812, 1): "rillaboom-gmax",
    (815, 1): "cinderace-gmax",
    (818, 1): "inteleon-gmax",
    # Other G-Max
    (823, 1): "corviknight-gmax",
    (826, 1): "orbeetle-gmax",
    (834, 1): "drednaw-gmax",
    (839, 1): "coalossal-gmax",
    (841, 1): "flapple-gmax",
    (842, 1): "appletun-gmax",
    (844, 1): "sandaconda-gmax",
    # Cramorant (gulping, gorging)
    (845, 1): "cramorant-gulping",
    (845, 2): "cramorant-gorging",
    # Toxtricity (low-key, gmax variants)
    (849, 1): "toxtricity-low-key",
    (849, 2): "toxtricity-gmax",
    (849, 3): "toxtricity-low-key-gmax",
    # More G-Max
    (851, 1): "centiskorch-gmax",
    # Sinistea / Polteageist (antique)
    (854, 1): "sinistea-antique",
    (855, 1): "polteageist-antique",
    # More G-Max
    (858, 1): "hatterene-gmax",
    (861, 1): "grimmsnarl-gmax",
    (869, 1): "alcremie-gmax",
    # Eiscue (noice face)
    (875, 1): "eiscue-noice",
    # Indeedee (female)
    (876, 1): "indeedee-f",
    # Morpeko (hangry)
    (877, 1): "morpeko-hangry",
    # More G-Max
    (879, 1): "copperajah-gmax",
    (884, 1): "duraludon-gmax",
    # Zacian / Zamazenta crowned formes
    (888, 1): "zacian-crowned",
    (889, 1): "zamazenta-crowned",
    # Eternatus eternamax
    (890, 1): "eternatus-eternamax",
    # Urshifu (rapid-strike and G-Max formes)
    (892, 1): "urshifu-rapidstrike",
    (892, 2): "urshifu-gmax",
    (892, 3): "urshifu-rapid-strike-gmax",
    # Zarude (dada)
    (893, 1): "zarude-dada",
    # Calyrex (ice rider, shadow rider)
    (898, 1): "calyrex-ice",
    (898, 2): "calyrex-shadow",
    # Ursaluna (bloodmoon)
    (901, 1): "ursaluna-bloodmoon",
    # Basculegion (female)
    (902, 1): "basculegion-f",
    # Enamorus (therian)
    (905, 1): "enamorus-therian",

    # ── GEN 9 ──────────────────────────────────────────────────────────────
    # Oinkologne (female)
    (916, 1): "oinkologne-f",
    # Maushold (family of three)
    (925, 1): "maushold-three",
    # Squawkabilly (plumage variants)
    (931, 1): "squawkabilly-blue-plumage",
    (931, 2): "squawkabilly-yellow-plumage",
    (931, 3): "squawkabilly-white-plumage",
    # Palafin (hero)
    (964, 1): "palafin-hero",
    # Tatsugiri (droopy, stretchy)
    (978, 1): "tatsugiri-droopy",
    (978, 2): "tatsugiri-stretchy",
    # Dudunsparce (three-segment)
    (982, 1): "dudunsparce-three-segment",
    # Gimmighoul (roaming)
    (999, 1): "gimmighoul-roaming",
    # Ogerpon (wellspring, hearthflame, cornerstone + tera variants)
    (1017, 1): "ogerpon-wellspring",
    (1017, 2): "ogerpon-hearthflame",
    (1017, 3): "ogerpon-cornerstone",
    (1017, 4): "ogerpon-teal-tera",
    (1017, 5): "ogerpon-wellspring-tera",
    (1017, 6): "ogerpon-hearthflame-tera",
    (1017, 7): "ogerpon-cornerstone-tera",
    # Terapagos (terastal, stellar)
    (1024, 1): "terapagos-terastal",
    (1024, 2): "terapagos-stellar",
}


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def sprite_filename(dex: int, form_idx: int) -> str:
    return f"{dex}-{form_idx}.png"


def download_sprite(name: str, dest_path: str) -> bool:
    for url_template in URLS:
        url = url_template.format(name=name)
        try:
            req = urllib.request.Request(url, headers={"User-Agent": "Mozilla/5.0"})
            with urllib.request.urlopen(req, timeout=15) as resp:
                data = resp.read()
            if len(data) < 50:
                continue  # too small — probably a placeholder
            with open(dest_path, "wb") as f:
                f.write(data)
            return True
        except urllib.error.HTTPError as e:
            if e.code != 404:
                print(f"    HTTP {e.code} for {url}")
        except Exception as e:
            print(f"    Error ({url}): {e}")
    return False


def _sort_key(fname: str):
    """Sort helper: '3-1.png' → (3, 1), '25.png' → (25, 0)"""
    base = fname.replace(".png", "")
    parts = base.split("-")
    try:
        return (int(parts[0]), int(parts[1]) if len(parts) > 1 else 0)
    except ValueError:
        return (999999, 0)


def update_qrc(new_files: list):
    if not new_files:
        return
    with open(QRC_FILE, "r", encoding="utf-8") as f:
        content = f.read()

    marker = "    </qresource>"
    if marker not in content:
        print("  WARNING: Could not find insertion point in resources.qrc")
        return

    new_entries = "\n".join(
        f"        <file>db/sprites/{fname}</file>"
        for fname in sorted(new_files, key=_sort_key)
    )
    content = content.replace(marker, new_entries + "\n" + marker, 1)

    with open(QRC_FILE, "w", encoding="utf-8") as f:
        f.write(content)

    print(f"  Updated resources.qrc with {len(new_files)} new entries.")


def already_in_qrc(fname: str) -> bool:
    with open(QRC_FILE, "r", encoding="utf-8") as f:
        return fname in f.read()


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    if not os.path.isdir(SPRITES_DIR):
        print("ERROR: Run from project root (db/sprites not found).")
        return

    downloaded = []
    skipped   = []
    failed    = []

    total = len(FORMS)
    for i, ((dex, form_idx), name) in enumerate(FORMS.items(), 1):
        fname = sprite_filename(dex, form_idx)
        dest  = os.path.join(SPRITES_DIR, fname)

        if os.path.exists(dest):
            skipped.append(fname)
            continue

        print(f"  [{i}/{total}] #{dex} form {form_idx} ({name})... ", end="", flush=True)
        ok = download_sprite(name, dest)
        if ok:
            print("OK")
            downloaded.append(fname)
        else:
            print("not found")
            failed.append((dex, form_idx, name))

        time.sleep(0.05)

    print(f"\nResult: {len(downloaded)} downloaded, {len(skipped)} already present, "
          f"{len(failed)} not found.")

    if failed:
        print("\nNot found on Smogon/PS (check name or sprite may not exist):")
        for dex, fi, name in failed:
            print(f"  #{dex} form {fi}: '{name}'")

    new_for_qrc = [f for f in downloaded if not already_in_qrc(f)]
    if new_for_qrc:
        update_qrc(new_for_qrc)
        print("\nNext: mingw32-make -f Makefile.Release")
    elif downloaded:
        print("\nAll downloaded files were already in resources.qrc.")


if __name__ == "__main__":
    main()
