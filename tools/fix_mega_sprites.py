"""
Fix oversized Mega/Champions sprites by downloading correct 40x30 minisprites
from Smogon (primary) or PS gen5 (fallback, resized to 40x30).
Sprites not found anywhere are resized from the existing Serebii 120x120.

Usage (run from project root):
    python tools/fix_mega_sprites.py
"""

import os
import io
import time
import urllib.request
import urllib.error

from PIL import Image

SPRITES_DIR = "db/sprites"
TARGET_SIZE = (40, 30)
SMOGON_URL = "https://www.smogon.com/forums/media/minisprites/{name}.png"
PS_URL     = "https://play.pokemonshowdown.com/sprites/gen5/{name}.png"

# (dex-form) -> list of PS/Smogon names to try in order
SPRITE_NAMES = {
    "26-2":  [],              # not on Smogon/PS yet — resize from existing
    "26-3":  [],              # not on Smogon/PS yet — resize from existing
    "36-1":  ["clefable-mega"],
    "71-1":  ["victreebel-mega"],
    "121-1": ["starmie-mega"],
    "149-1": ["dragonite-mega"],
    "154-1": ["meganium-mega"],
    "160-1": ["feraligatr-mega"],
    "227-1": ["skarmory-mega"],
    "358-1": ["chimecho-mega"],
    "359-2": ["absol-mega-z"],
    "398-1": ["staraptor-mega"],
    "445-2": ["garchomp-mega-z"],
    "448-2": ["lucario-gmax"],
    "478-1": ["froslass-mega"],
    "485-1": ["heatran-mega"],
    "491-1": ["darkrai-mega"],
    "500-1": ["emboar-mega"],
    "530-1": ["excadrill-mega"],
    "545-1": ["scolipede-mega"],
    "560-1": ["scrafty-mega"],
    "604-1": ["eelektross-mega"],
    "609-1": ["chandelure-mega"],
    "623-1": ["golurk-mega"],
    "652-1": ["chesnaught-mega"],
    "655-1": ["delphox-mega"],
    "670-2": ["floette-mega"],
    "678-2": ["meowstic-f-mega"],
    "687-1": ["malamar-mega"],
    "689-1": ["barbaracle-mega"],
    "691-1": ["dragalge-mega"],
    "701-1": ["hawlucha-mega"],
    "740-1": ["crabominable-mega"],
    "768-1": ["golisopod-mega"],
    "780-1": ["drampa-mega"],
    "801-3": ["magearna-original-mega"],
    "807-1": ["zeraora-mega"],
    "870-1": ["falinks-mega"],
    "892-1": ["urshifu-rapidstrike"],   # only on PS gen5 (96x96) — will resize
    "952-1": ["scovillain-mega"],
    "970-1": ["glimmora-mega"],
    "978-4": ["tatsugiri-curly-mega"],
}


def fetch(url):
    req = urllib.request.Request(url, headers={"User-Agent": "Mozilla/5.0"})
    with urllib.request.urlopen(req, timeout=15) as r:
        return r.read()


def try_download(names):
    """Try Smogon first, then PS gen5 for each name. Returns (data, source_url) or None."""
    for name in names:
        for url_tpl in [SMOGON_URL, PS_URL]:
            url = url_tpl.format(name=name)
            try:
                data = fetch(url)
                if len(data) < 50:
                    continue
                return data, url
            except urllib.error.HTTPError as e:
                if e.code != 404:
                    print(f"    HTTP {e.code}: {url}")
            except Exception as e:
                print(f"    Error: {url}: {e}")
    return None


def save_resized(data_or_path, dest):
    """Load image from bytes or path, resize to 40x30, save as PNG."""
    if isinstance(data_or_path, bytes):
        img = Image.open(io.BytesIO(data_or_path))
    else:
        img = Image.open(data_or_path)

    if img.size != TARGET_SIZE:
        img = img.resize(TARGET_SIZE, Image.LANCZOS)

    img.save(dest, format="PNG")


def main():
    ok = []
    resized_only = []
    failed = []

    for key, names in SPRITE_NAMES.items():
        fname = f"{key}.png"
        dest  = os.path.join(SPRITES_DIR, fname)

        if not os.path.exists(dest):
            print(f"  SKIP {fname}: file not found in db/sprites/")
            failed.append(key)
            continue

        # Check current size
        current = Image.open(dest)
        if current.size == TARGET_SIZE:
            print(f"  OK   {fname}: already {TARGET_SIZE}")
            ok.append(key)
            continue

        print(f"  FIX  {fname}: {current.size} -> {TARGET_SIZE}", end=" ... ")

        if names:
            result = try_download(names)
            if result:
                data, url = result
                save_resized(data, dest)
                src = "Smogon" if "smogon" in url else "PS"
                print(f"downloaded from {src} ({names[0]})")
                ok.append(key)
                time.sleep(0.1)
                continue

        # Fallback: resize the existing file
        save_resized(dest, dest)
        print("resized from existing (not on Smogon/PS)")
        resized_only.append(key)

    print(f"\nDone: {len(ok)} fixed from Smogon/PS, "
          f"{len(resized_only)} resized from existing, "
          f"{len(failed)} not found at all.")
    if resized_only:
        print(f"Resized from Serebii: {resized_only}")


if __name__ == "__main__":
    main()
