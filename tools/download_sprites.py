"""
Download Pokemon minisprites from Smogon for Gen 8+ (#808-#1025).
Replaces any existing files with the Smogon style sprites.
Updates resources.qrc automatically.

Usage (run from project root):
    python tools/download_sprites.py
"""

import os
import urllib.request
import urllib.error
import time

SPRITES_DIR = "db/sprites"
QRC_FILE = "resources.qrc"
BASE_URL = "https://www.smogon.com/forums/media/minisprites/{name}.png"

# Complete Pokedex number -> Smogon minisprite name mapping for Gen 8+
POKEMON = {
    808: "meltan",
    809: "melmetal",
    810: "grookey",
    811: "thwackey",
    812: "rillaboom",
    813: "scorbunny",
    814: "raboot",
    815: "cinderace",
    816: "sobble",
    817: "drizzile",
    818: "inteleon",
    819: "skwovet",
    820: "greedent",
    821: "rookidee",
    822: "corvisquire",
    823: "corviknight",
    824: "blipbug",
    825: "dottler",
    826: "orbeetle",
    827: "nickit",
    828: "thievul",
    829: "gossifleur",
    830: "eldegoss",
    831: "wooloo",
    832: "dubwool",
    833: "chewtle",
    834: "drednaw",
    835: "yamper",
    836: "boltund",
    837: "rolycoly",
    838: "carkol",
    839: "coalossal",
    840: "applin",
    841: "flapple",
    842: "appletun",
    843: "silicobra",
    844: "sandaconda",
    845: "cramorant",
    846: "arrokuda",
    847: "barraskewda",
    848: "toxel",
    849: "toxtricity",
    850: "sizzlipede",
    851: "centiskorch",
    852: "clobbopus",
    853: "grapploct",
    854: "sinistea",
    855: "polteageist",
    856: "hatenna",
    857: "hattrem",
    858: "hatterene",
    859: "impidimp",
    860: "morgrem",
    861: "grimmsnarl",
    862: "obstagoon",
    863: "perrserker",
    864: "cursola",
    865: "sirfetchd",
    866: "mr-rime",
    867: "runerigus",
    868: "milcery",
    869: "alcremie",
    870: "falinks",
    871: "pincurchin",
    872: "snom",
    873: "frosmoth",
    874: "stonjourner",
    875: "eiscue",
    876: "indeedee",
    877: "morpeko",
    878: "cufant",
    879: "copperajah",
    880: "dracozolt",
    881: "arctozolt",
    882: "dracovish",
    883: "arctovish",
    884: "duraludon",
    885: "dreepy",
    886: "drakloak",
    887: "dragapult",
    888: "zacian",
    889: "zamazenta",
    890: "eternatus",
    891: "kubfu",
    892: "urshifu",
    893: "zarude",
    894: "regieleki",
    895: "regidrago",
    896: "glastrier",
    897: "spectrier",
    898: "calyrex",
    899: "wyrdeer",
    900: "kleavor",
    901: "ursaluna",
    902: "basculegion",
    903: "sneasler",
    904: "overqwil",
    905: "enamorus",
    906: "sprigatito",
    907: "floragato",
    908: "meowscarada",
    909: "fuecoco",
    910: "crocalor",
    911: "skeledirge",
    912: "quaxly",
    913: "quaxwell",
    914: "quaquaval",
    915: "lechonk",
    916: "oinkologne",
    917: "tarountula",
    918: "spidops",
    919: "nymble",
    920: "lokix",
    921: "pawmi",
    922: "pawmo",
    923: "pawmot",
    924: "tandemaus",
    925: "maushold",
    926: "fidough",
    927: "dachsbun",
    928: "smoliv",
    929: "dolliv",
    930: "arboliva",
    931: "squawkabilly",
    932: "nacli",
    933: "naclstack",
    934: "garganacl",
    935: "charcadet",
    936: "armarouge",
    937: "ceruledge",
    938: "tadbulb",
    939: "bellibolt",
    940: "wattrel",
    941: "kilowattrel",
    942: "maschiff",
    943: "mabosstiff",
    944: "shroodle",
    945: "grafaiai",
    946: "bramblin",
    947: "brambleghast",
    948: "toedscool",
    949: "toedscruel",
    950: "klawf",
    951: "capsakid",
    952: "scovillain",
    953: "rellor",
    954: "rabsca",
    955: "flittle",
    956: "espathra",
    957: "tinkatink",
    958: "tinkatuff",
    959: "tinkaton",
    960: "wiglett",
    961: "wugtrio",
    962: "bombirdier",
    963: "finizen",
    964: "palafin",
    965: "varoom",
    966: "revavroom",
    967: "cyclizar",
    968: "orthworm",
    969: "glimmet",
    970: "glimmora",
    971: "greavard",
    972: "houndstone",
    973: "flamigo",
    974: "cetoddle",
    975: "cetitan",
    976: "veluza",
    977: "dondozo",
    978: "tatsugiri",
    979: "annihilape",
    980: "clodsire",
    981: "farigiraf",
    982: "dudunsparce",
    983: "kingambit",
    984: "great-tusk",
    985: "scream-tail",
    986: "brute-bonnet",
    987: "flutter-mane",
    988: "slither-wing",
    989: "sandy-shocks",
    990: "iron-treads",
    991: "iron-bundle",
    992: "iron-hands",
    993: "iron-jugulis",
    994: "iron-moth",
    995: "iron-thorns",
    996: "frigibax",
    997: "arctibax",
    998: "baxcalibur",
    999: "gimmighoul",
    1000: "gholdengo",
    1001: "wo-chien",
    1002: "chien-pao",
    1003: "ting-lu",
    1004: "chi-yu",
    1005: "roaring-moon",
    1006: "iron-valiant",
    1007: "koraidon",
    1008: "miraidon",
    1009: "walking-wake",
    1010: "iron-leaves",
    1011: "dipplin",
    1012: "poltchageist",
    1013: "sinistcha",
    1014: "okidogi",
    1015: "munkidori",
    1016: "fezandipiti",
    1017: "ogerpon",
    1018: "archaludon",
    1019: "hydrapple",
    1020: "gouging-fire",
    1021: "raging-bolt",
    1022: "iron-boulder",
    1023: "iron-crown",
    1024: "terapagos",
    1025: "pecharunt",
}


def download_sprite(name, dest_path):
    url = BASE_URL.format(name=name)
    try:
        with urllib.request.urlopen(url, timeout=15) as resp:
            data = resp.read()
        if len(data) < 50:
            return False
        with open(dest_path, "wb") as f:
            f.write(data)
        return True
    except urllib.error.HTTPError as e:
        if e.code != 404:
            print(f"    HTTP {e.code} — {url}")
        return False
    except Exception as e:
        print(f"    Error: {e}")
        return False


def is_in_qrc(fname):
    with open(QRC_FILE, "r", encoding="utf-8") as f:
        return fname in f.read()


def update_qrc(new_files):
    if not new_files:
        return
    with open(QRC_FILE, "r", encoding="utf-8") as f:
        content = f.read()

    insert_marker = "    </qresource>"
    if insert_marker not in content:
        print("  WARNING: Could not find insertion point in resources.qrc")
        return

    new_entries = "\n".join(
        f"        <file>db/sprites/{fname}</file>"
        for fname in sorted(new_files, key=lambda x: int(x.replace(".png", "")))
    )
    content = content.replace(insert_marker, new_entries + "\n" + insert_marker, 1)

    with open(QRC_FILE, "w", encoding="utf-8") as f:
        f.write(content)

    print(f"  Updated resources.qrc with {len(new_files)} new entries.")


def main():
    if not os.path.isdir(SPRITES_DIR):
        print("ERROR: Run from project root (db/sprites not found).")
        return

    downloaded = []
    replaced = []
    failed = []

    for num, name in POKEMON.items():
        dest = os.path.join(SPRITES_DIR, f"{num}.png")
        already_exists = os.path.exists(dest)

        print(f"  #{num} ({name})... ", end="", flush=True)
        ok = download_sprite(name, dest)
        if ok:
            if already_exists:
                print("replaced")
                replaced.append(f"{num}.png")
            else:
                print("OK")
                downloaded.append(f"{num}.png")
        else:
            print("not found")
            failed.append((num, name))

        time.sleep(0.03)

    print(f"\nResult: {len(downloaded)} new, {len(replaced)} replaced, {len(failed)} not found.")

    if failed:
        print("Not found on Smogon:")
        for num, name in failed:
            print(f"  #{num} {name}")

    # Only add to QRC entries that didn't exist before
    new_for_qrc = [f for f in downloaded if not is_in_qrc(f)]
    if new_for_qrc:
        update_qrc(new_for_qrc)

    if downloaded or replaced:
        print("\nNext: mingw32-make -f Makefile.Release")


if __name__ == "__main__":
    main()
