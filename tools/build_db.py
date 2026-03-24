#!/usr/bin/env python3
"""
build_db.py — Regenerate vgcspreader binary databases from pokemon-showdown source data.

Usage
-----
    python build_db.py --showdown <path-to-pokemon-showdown> --project <path-to-vgcspreader>

Requirements
------------
    pip install regex          # (optional) for better Unicode handling; stdlib re is sufficient

The script reads:
  <showdown>/data/pokedex.ts
  <showdown>/data/moves.ts
  <showdown>/data/abilities.ts
  <showdown>/data/items.ts

And writes into <project>/db/:
  personal_species.bin   — 84 bytes/entry, indexed by National Dex number
  personal_moves.bin     — 8 bytes/entry, ordered by the Moves enum in moves.hpp
  personal_items.bin     — 6 bytes/entry, ordered by the Items enum in items.hpp
  species.txt            — one display name per line, in Dex-number order (for GUI)
  abilities.txt          — one name per line, in enum order
  moves.txt              — one name per line, in enum order
  items.txt              — one name per line, in enum order
  types.txt              — one name per line, in enum order (unchanged)

Binary format (species, 84 bytes each)
---------------------------------------
  [0-5]   uint8  × 6   base stats (HP, ATK, DEF, SpATK, SpDEF, SPE)
  [6-7]   uint8  × 2   type1, type2 (Type enum index; monotype → same value twice)
  [8-13]  uint16 × 3   ability0, ability1, abilityH (Ability enum index, LE)
  [14-27] reserved / zero
  [28-29] uint16        first-alternate-form flat index (0 if no alternates)
  [30-31] reserved / zero
  [32]    uint8         total number of forms (1 for no alternates)
  [33-83] reserved / zero

Binary format (moves, 8 bytes each)
-------------------------------------
  [0-1]  uint16  base power (LE, 0 for status moves)
  [2]    uint8   type (Type enum index)
  [3]    uint8   category (0=Physical, 1=Special, 2=Status)
  [4]    uint8   is_spread (1 if hits both opponents)
  [5-6]  uint16  Z-move base power (LE, 0 if none)
  [7]    uint8   is_signature_z (1 for named signature Z-moves)

Binary format (items, 6 bytes each)
-------------------------------------
  [0]  uint8  is_removable (can be Knocked Off / Flung)
  [1]  uint8  is_reducing_berry (halves SE damage of its type)
  [2]  uint8  reducing_berry_type (Type enum index, 0 if not a reducing berry)
  [3]  uint8  is_restoring_berry (restores HP when below threshold)
  [4]  uint8  restoring_activation_percent (threshold %, 0 if not a restoring berry)
  [5]  uint8  restoring_hp_percent (% of max HP restored, 0 if not applicable)
"""

import argparse
import os
import re
import struct
import sys
from pathlib import Path
from typing import Dict, List, Optional, Tuple


# ──────────────────────────────────────────────────────────────────────────────
# 1. C++ header parsers — derive enum→index mappings from the project headers
# ──────────────────────────────────────────────────────────────────────────────

def parse_cpp_enum(header_path: Path) -> Dict[str, int]:
    """Return {EnumName: int_value} for every member in the first C++ enum found."""
    text = header_path.read_text(encoding="utf-8")
    # Grab the body between the first '{' and '};'
    m = re.search(r'enum\s+\w+\s*\{([^}]*)\}', text, re.DOTALL)
    if not m:
        raise ValueError(f"No enum found in {header_path}")
    body = m.group(1)
    result: Dict[str, int] = {}
    idx = 0
    for token in re.split(r'[,\n]', body):
        token = token.strip()
        if not token or token.startswith('//'):
            continue
        # Strip trailing inline comments
        token = re.sub(r'//.*', '', token).strip()
        if not token:
            continue
        if '=' in token:
            name, val = token.split('=', 1)
            idx = int(val.strip())
            result[name.strip()] = idx
        else:
            result[token] = idx
        idx += 1
    return result


# ──────────────────────────────────────────────────────────────────────────────
# 2. Showdown TypeScript parsers
# ──────────────────────────────────────────────────────────────────────────────

def extract_ts_entries(ts_text: str) -> Dict[str, str]:
    """
    Parse a Showdown TypeScript data file and return {id_key: raw_object_text}.

    Uses indentation-level parsing rather than brace tracking to correctly handle
    function bodies, template literals, and other complex TypeScript syntax that
    appears in entries like 'thief' or 'spectralthief'.

    Showdown's data files follow a consistent structure:
      export const X = {
        entryid: {          ← exactly one leading tab
          field: value,     ← exactly two leading tabs
          ...
        },
        ...
      };

    Top-level entries always start with exactly ONE tab and end at the next line
    that also starts with exactly one tab (or the closing '};').
    """
    entries: Dict[str, str] = {}
    lines = ts_text.splitlines()

    # Regex for a top-level key line: <TAB>word: {
    top_level = re.compile(r'^\t([\w$]+)\s*:\s*\{')

    current_key: Optional[str] = None
    current_lines: List[str] = []

    for line in lines:
        m = top_level.match(line)
        if m:
            # Save previous entry
            if current_key is not None:
                entries[current_key] = '\n'.join(current_lines)
            current_key = m.group(1)
            current_lines = [line]
        elif current_key is not None:
            # Inside an entry — check for the closing line '\t},' or '\t}'
            stripped = line.rstrip()
            if stripped in ('\t},', '\t}'):
                current_lines.append(line)
                entries[current_key] = '\n'.join(current_lines)
                current_key = None
                current_lines = []
            else:
                current_lines.append(line)

    # Handle last entry without trailing comma
    if current_key is not None:
        entries[current_key] = '\n'.join(current_lines)

    return entries


def get_field_str(obj: str, field: str) -> Optional[str]:
    """Extract a simple string field value: field: "value"  or  field: 'value'"""
    m = re.search(rf'\b{field}\s*:\s*["\']([^"\']*)["\']', obj)
    return m.group(1) if m else None


def get_field_int(obj: str, field: str) -> Optional[int]:
    """Extract a simple integer field value: field: 123"""
    m = re.search(rf'\b{field}\s*:\s*(-?\d+)', obj)
    return int(m.group(1)) if m else None


def get_field_bool(obj: str, field: str) -> bool:
    """Extract a boolean field: field: true"""
    m = re.search(rf'\b{field}\s*:\s*(true|false)\b', obj)
    return m.group(1) == 'true' if m else False


def get_types(obj: str) -> List[str]:
    """Extract types: ["T1", "T2"] or ["T1"]"""
    m = re.search(r'\btypes\s*:\s*\[([^\]]*)\]', obj)
    if not m:
        return []
    return [t.strip().strip('"\'') for t in m.group(1).split(',') if t.strip()]


def get_base_stats(obj: str) -> Optional[Dict[str, int]]:
    """Extract baseStats: {hp: N, atk: N, def: N, spa: N, spd: N, spe: N}"""
    m = re.search(r'\bbaseStats\s*:\s*\{([^}]*)\}', obj)
    if not m:
        return None
    stats = {}
    for pair in re.finditer(r'(\w+)\s*:\s*(\d+)', m.group(1)):
        stats[pair.group(1)] = int(pair.group(2))
    return stats


def get_abilities_map(obj: str) -> Dict[str, str]:
    """Extract abilities: {0: "A1", 1: "A2", H: "AH"} etc."""
    m = re.search(r'\babilities\s*:\s*\{([^}]*)\}', obj)
    if not m:
        return {}
    result = {}
    for pair in re.finditer(r'["\']?(\w+)["\']?\s*:\s*["\']([^"\']+)["\']', m.group(1)):
        result[pair.group(1)] = pair.group(2)
    return result


def get_z_move_info(obj: str) -> Tuple[int, bool]:
    """Return (z_base_power, is_signature_z) from a move entry."""
    # Signature Z: has 'isZ' field pointing to a Z-Crystal ID
    is_sig = bool(re.search(r'\bisZ\s*:', obj))
    # zMove: {basePower: N}
    m = re.search(r'\bzMove\s*:\s*\{[^}]*\bbasePower\s*:\s*(\d+)', obj)
    if m:
        return int(m.group(1)), is_sig
    # If it IS a signature Z, its own base power is the Z base power
    if is_sig:
        bp = get_field_int(obj, 'basePower') or 0
        return bp, True
    return 0, False


def get_target(obj: str) -> bool:
    """Return True if the move hits multiple targets (spread)."""
    t = get_field_str(obj, 'target') or ''
    return t in ('allAdjacent', 'allAdjacentFoes')


# ──────────────────────────────────────────────────────────────────────────────
# 3. Name normalisation helpers
# ──────────────────────────────────────────────────────────────────────────────

def showdown_name_to_cpp(name: str) -> str:
    """
    Convert a Showdown display name to the likely C++ enum identifier.
    e.g. "Primordial Sea" → "Primordial_Sea", "RKS System" → "RKS_System"
    """
    # Replace hyphens and spaces with underscores
    result = re.sub(r'[ \-]', '_', name)
    # Remove apostrophes and other punctuation (e.g. "Let's Snuggle Forever")
    result = re.sub(r"[''\'`]", '', result)
    # Collapse multiple underscores
    result = re.sub(r'_+', '_', result)
    return result


def build_name_index(enum_dict: Dict[str, int]) -> Dict[str, int]:
    """
    Build a lookup that accepts both exact enum names AND lowercased/normalised variants.
    """
    out: Dict[str, int] = {}
    for name, idx in enum_dict.items():
        out[name] = idx
        out[name.lower()] = idx
        out[name.replace('_', '').lower()] = idx
    return out


def resolve_ability(name: str, ability_lookup: Dict[str, int]) -> int:
    """Return ability enum value for a Showdown ability display name, or 0 (No_Ability)."""
    cpp = showdown_name_to_cpp(name)
    for key in (cpp, name, cpp.lower(), name.lower(), name.replace(' ', '').lower()):
        if key in ability_lookup:
            return ability_lookup[key]
    return 0


def resolve_type(name: str, type_lookup: Dict[str, int]) -> int:
    """Return Type enum value for a type name string, or 18 (Typeless)."""
    for key in (name, name.lower(), name.capitalize()):
        if key in type_lookup:
            return type_lookup[key]
    return 18  # Typeless


def resolve_move(name: str, move_lookup: Dict[str, int]) -> Optional[int]:
    """Return Moves enum value, or None if not found."""
    cpp = showdown_name_to_cpp(name)
    for key in (cpp, name, cpp.lower(), name.lower(), name.replace(' ', '').lower()):
        if key in move_lookup:
            return move_lookup[key]
    return None


# ──────────────────────────────────────────────────────────────────────────────
# 4. Binary builders
# ──────────────────────────────────────────────────────────────────────────────

SPECIES_ENTRY_SIZE = 84
MOVE_ENTRY_SIZE = 8
ITEM_ENTRY_SIZE = 6

# Maximum National Dex number to include (Gen 9 = 1025, plus some room)
MAX_DEX = 1025


def build_species_binary(
    showdown_pokedex: Dict[str, str],
    type_lookup: Dict[str, int],
    ability_lookup: Dict[str, int],
) -> Tuple[bytes, List[str]]:
    """
    Build personal_species.bin and a parallel list of display names (by dex index).
    Returns (binary_bytes, display_names_by_dex_index).
    """
    # ── Pass 1: group entries by National Dex number ──────────────────────────
    # base forms: no 'baseSpecies' field (or baseSpecies == name)
    base_forms: Dict[int, str] = {}         # dex_num → entry_id
    alt_forms: Dict[int, List[str]] = {}    # dex_num → [entry_id, ...]
    display_names: Dict[int, str] = {}      # dex_num → display name

    for entry_id, obj in showdown_pokedex.items():
        num = get_field_int(obj, 'num')
        if num is None or num <= 0 or num > MAX_DEX:
            continue
        # Skip cosmetic formes that don't affect battle stats in the standard sense
        non_std = get_field_str(obj, 'isNonstandard')
        if non_std in ('Past', 'LGPE', 'Unobtainable'):
            continue

        base_species = get_field_str(obj, 'baseSpecies')
        name_display = get_field_str(obj, 'name') or entry_id

        if base_species is None:
            # This is a base form
            base_forms[num] = entry_id
            display_names[num] = name_display
        else:
            # Alternate form
            if num not in alt_forms:
                alt_forms[num] = []
            alt_forms[num].append(entry_id)

    # ── Pass 2: allocate flat indices for alternate forms ─────────────────────
    # Main entries occupy flat indices 0 .. MAX_DEX
    # Alternate forms are appended after index MAX_DEX
    next_form_flat_index = MAX_DEX + 1
    # Map (dex_num, form_index) → flat_index
    # form_index 0 is the base form (already at flat index = dex_num)
    form_flat: Dict[Tuple[int, int], int] = {}
    alt_form_data: Dict[int, str] = {}  # flat_index → entry_id

    for dex_num, ids in sorted(alt_forms.items()):
        for i, entry_id in enumerate(ids):
            form_flat[(dex_num, i + 1)] = next_form_flat_index
            alt_form_data[next_form_flat_index] = entry_id
            next_form_flat_index += 1

    total_entries = next_form_flat_index
    data = bytearray(total_entries * SPECIES_ENTRY_SIZE)

    def write_entry(flat_idx: int, obj: str, dex_num: int, form_idx: int) -> None:
        off = flat_idx * SPECIES_ENTRY_SIZE
        stats = get_base_stats(obj)
        if stats:
            for stat_off, key in enumerate(('hp', 'atk', 'def', 'spa', 'spd', 'spe')):
                data[off + stat_off] = min(255, stats.get(key, 0))

        types = get_types(obj)
        t1 = resolve_type(types[0], type_lookup) if len(types) > 0 else 18
        t2 = resolve_type(types[1], type_lookup) if len(types) > 1 else t1
        data[off + 6] = t1
        data[off + 7] = t2

        ab = get_abilities_map(obj)
        a0 = resolve_ability(ab.get('0', ''), ability_lookup)
        a1 = resolve_ability(ab.get('1', ''), ability_lookup)
        ah = resolve_ability(ab.get('H', ''), ability_lookup)
        # Each ability = uint16 LE at offsets 8, 10, 12
        struct.pack_into('<H', data, off + 8, a0)
        struct.pack_into('<H', data, off + 10, a1)
        struct.pack_into('<H', data, off + 12, ah)

        # form_offset (uint16 LE at offset 28): flat index of first alternate form (0 if none)
        if form_idx == 0 and dex_num in alt_forms:
            first_alt_flat = form_flat.get((dex_num, 1), 0)
            struct.pack_into('<H', data, off + 28, first_alt_flat)
        # form_count (uint8 at offset 32)
        if form_idx == 0:
            form_count = 1 + len(alt_forms.get(dex_num, []))
            data[off + 32] = min(255, form_count)
        else:
            data[off + 32] = 1

    # Write base forms
    for dex_num, entry_id in base_forms.items():
        write_entry(dex_num, showdown_pokedex[entry_id], dex_num, 0)

    # Write alternate forms
    for (dex_num, form_idx), flat_idx in form_flat.items():
        entry_id = alt_form_data[flat_idx]
        write_entry(flat_idx, showdown_pokedex[entry_id], dex_num, form_idx)

    # Build display name list (indexed 0..MAX_DEX, empty string for gaps)
    name_list = [''] * (MAX_DEX + 1)
    for dex_num, name in display_names.items():
        if dex_num <= MAX_DEX:
            name_list[dex_num] = name

    return bytes(data), name_list


def build_moves_binary(
    showdown_moves: Dict[str, str],
    move_enum: Dict[str, int],
    type_lookup: Dict[str, int],
) -> bytes:
    """Build personal_moves.bin indexed by Moves enum value."""
    num_moves = max(move_enum.values()) + 1
    data = bytearray(num_moves * MOVE_ENTRY_SIZE)

    # Build a lookup: normalised showdown move name → entry_id
    # Showdown move ids are lowercase no-spaces versions of the name.
    sd_name_to_id: Dict[str, str] = {}
    for entry_id, obj in showdown_moves.items():
        display = get_field_str(obj, 'name') or entry_id
        sd_name_to_id[entry_id] = entry_id
        sd_name_to_id[display.lower().replace(' ', '').replace('-', '').replace("'", '')] = entry_id

    for cpp_name, move_idx in move_enum.items():
        # Try to find the corresponding Showdown entry
        normalised = cpp_name.lower().replace('_', '').replace(' ', '')
        entry_id = sd_name_to_id.get(normalised) or sd_name_to_id.get(normalised.replace('é', 'e'))
        if entry_id is None:
            # Fallback: try some common differences
            alt = normalised
            entry_id = sd_name_to_id.get(alt)

        off = move_idx * MOVE_ENTRY_SIZE

        if entry_id and entry_id in showdown_moves:
            obj = showdown_moves[entry_id]
            bp = get_field_int(obj, 'basePower') or 0
            category_str = get_field_str(obj, 'category') or 'Status'
            category = 0 if category_str == 'Physical' else (1 if category_str == 'Special' else 2)
            type_str = get_field_str(obj, 'type') or 'Normal'
            type_idx = resolve_type(type_str, type_lookup)
            is_spread = 1 if get_target(obj) else 0
            z_bp, is_sig_z = get_z_move_info(obj)

            struct.pack_into('<H', data, off + 0, min(65535, bp))
            data[off + 2] = type_idx
            data[off + 3] = category
            data[off + 4] = is_spread
            struct.pack_into('<H', data, off + 5, min(65535, z_bp))
            data[off + 7] = 1 if is_sig_z else 0
        # else: entry stays zeroed (unknown move)

    return bytes(data)


# Item definitions that cannot be derived from Showdown data alone.
# Format: (is_removable, is_reducing_berry, reducing_berry_type_name,
#           is_restoring_berry, restore_activation_pct, restore_hp_pct)
ITEM_HARDCODED: Dict[str, Tuple] = {
    'None':           (0, 0, 'Normal',   0,  0,  0),
    'Aguav_Berry':    (1, 0, 'Normal',   1, 25, 33),
    'Assault_Vest':   (1, 0, 'Normal',   0,  0,  0),
    'Babiri_Berry':   (1, 1, 'Steel',    0,  0,  0),
    'Charti_Berry':   (1, 1, 'Rock',     0,  0,  0),
    'Chilan_Berry':   (1, 1, 'Normal',   0,  0,  0),
    'Choice_Band':    (1, 0, 'Normal',   0,  0,  0),
    'Choice_Specs':   (1, 0, 'Normal',   0,  0,  0),
    'Chople_Berry':   (1, 1, 'Fighting', 0,  0,  0),
    'Coba_Berry':     (1, 1, 'Flying',   0,  0,  0),
    'Colbur_Berry':   (1, 1, 'Dark',     0,  0,  0),
    'Figy_Berry':     (1, 0, 'Normal',   1, 25, 33),
    'Haban_Berry':    (1, 1, 'Dragon',   0,  0,  0),
    'Iapapa_Berry':   (1, 0, 'Normal',   1, 25, 33),
    'Kasib_Berry':    (1, 1, 'Ghost',    0,  0,  0),
    'Kebia_Berry':    (1, 1, 'Poison',   0,  0,  0),
    'Life_Orb':       (1, 0, 'Normal',   0,  0,  0),
    'Mago_Berry_':    (1, 0, 'Normal',   1, 25, 33),
    'Occa_Berry':     (1, 1, 'Fire',     0,  0,  0),
    'Passho_Berry':   (1, 1, 'Water',    0,  0,  0),
    'Payapa_Berry':   (1, 1, 'Psychic',  0,  0,  0),
    'Rindo_Berry':    (1, 1, 'Grass',    0,  0,  0),
    'Roseli_Berry':   (1, 1, 'Fairy',    0,  0,  0),
    'Shuca_Berry':    (1, 1, 'Ground',   0,  0,  0),
    'Sitrus_Berry':   (1, 0, 'Normal',   1, 50, 25),
    'Tanga_Berry':    (1, 1, 'Bug',      0,  0,  0),
    'Wacan_Berry':    (1, 1, 'Electric', 0,  0,  0),
    'Wiki_Berry':     (1, 0, 'Normal',   1, 25, 33),
    'Yache_Berry':    (1, 1, 'Ice',      0,  0,  0),
    # Gen 9 items
    'Booster_Energy': (1, 0, 'Normal',   0,  0,  0),
    'Clear_Amulet':   (1, 0, 'Normal',   0,  0,  0),
    'Covert_Cloak':   (1, 0, 'Normal',   0,  0,  0),
    'Loaded_Dice':    (1, 0, 'Normal',   0,  0,  0),
    'Mirror_Herb':    (1, 0, 'Normal',   0,  0,  0),
    'Punching_Glove': (1, 0, 'Normal',   0,  0,  0),
}


def build_items_binary(item_enum: Dict[str, int], type_lookup: Dict[str, int]) -> bytes:
    """Build personal_items.bin indexed by Items enum value."""
    num_items = max(item_enum.values()) + 1
    data = bytearray(num_items * ITEM_ENTRY_SIZE)

    for cpp_name, item_idx in item_enum.items():
        row = ITEM_HARDCODED.get(cpp_name)
        if row is None:
            # Unknown item: mark as removable, no special effects
            row = (1, 0, 'Normal', 0, 0, 0)
        is_rem, is_red, red_type_name, is_rst, rst_act, rst_pct = row
        off = item_idx * ITEM_ENTRY_SIZE
        data[off + 0] = is_rem
        data[off + 1] = is_red
        data[off + 2] = resolve_type(red_type_name, type_lookup)
        data[off + 3] = is_rst
        data[off + 4] = rst_act
        data[off + 5] = rst_pct

    return bytes(data)


# ──────────────────────────────────────────────────────────────────────────────
# 5. Text file builders
# ──────────────────────────────────────────────────────────────────────────────

def enum_to_display_name(cpp_name: str) -> str:
    """Convert a C++ enum identifier to a human-readable display name."""
    return cpp_name.replace('_', ' ').strip()


def build_name_txt(enum_dict: Dict[str, int], showdown_entries: Optional[Dict[str, str]] = None) -> str:
    """Build a text file with one name per line, in enum index order."""
    size = max(enum_dict.values()) + 1
    names = [''] * size
    for cpp_name, idx in enum_dict.items():
        names[idx] = enum_to_display_name(cpp_name)
    return '\n'.join(names) + '\n'


def build_species_txt(name_list: List[str]) -> str:
    return '\n'.join(name_list) + '\n'


# ──────────────────────────────────────────────────────────────────────────────
# 6. Main
# ──────────────────────────────────────────────────────────────────────────────

def main() -> None:
    parser = argparse.ArgumentParser(description="Regenerate vgcspreader binary databases from pokemon-showdown data.")
    parser.add_argument('--showdown', required=True, help="Path to the pokemon-showdown repository root")
    parser.add_argument('--project', default=str(Path(__file__).parent.parent),
                        help="Path to the vgcspreader project root (default: parent of tools/)")
    args = parser.parse_args()

    showdown_root = Path(args.showdown)
    project_root = Path(args.project)
    include_dir = project_root / 'include'
    db_dir = project_root / 'db'

    if not showdown_root.is_dir():
        sys.exit(f"ERROR: showdown root not found: {showdown_root}")
    if not include_dir.is_dir():
        sys.exit(f"ERROR: project include/ not found: {include_dir}")
    db_dir.mkdir(parents=True, exist_ok=True)

    # ── Load C++ enums ────────────────────────────────────────────────────────
    print("Loading C++ enum definitions …")
    ability_enum = parse_cpp_enum(include_dir / 'abilities.hpp')
    item_enum    = parse_cpp_enum(include_dir / 'items.hpp')
    move_enum    = parse_cpp_enum(include_dir / 'moves.hpp')
    type_enum    = parse_cpp_enum(include_dir / 'types.hpp')

    ability_lookup = build_name_index(ability_enum)
    type_lookup    = {k: v for k, v in type_enum.items()}  # already canonical

    print(f"  Abilities: {len(ability_enum)}  Items: {len(item_enum)}  "
          f"Moves: {len(move_enum)}  Types: {len(type_enum)}")

    # ── Parse Showdown TypeScript data ───────────────────────────────────────
    sd_data = showdown_root / 'data'
    print("Parsing pokemon-showdown data …")

    print("  pokedex.ts …")
    pokedex_ts = (sd_data / 'pokedex.ts').read_text(encoding='utf-8')
    pokedex = extract_ts_entries(pokedex_ts)
    print(f"    {len(pokedex)} entries")

    print("  moves.ts …")
    moves_ts = (sd_data / 'moves.ts').read_text(encoding='utf-8')
    sd_moves = extract_ts_entries(moves_ts)
    print(f"    {len(sd_moves)} entries")

    # ── Build binaries ────────────────────────────────────────────────────────
    print("Building personal_species.bin …")
    species_bytes, species_names = build_species_binary(pokedex, type_lookup, ability_lookup)
    (db_dir / 'personal_species.bin').write_bytes(species_bytes)
    print(f"  Written {len(species_bytes)} bytes "
          f"({len(species_bytes) // SPECIES_ENTRY_SIZE} entries)")

    print("Building personal_moves.bin …")
    moves_bytes = build_moves_binary(sd_moves, move_enum, type_lookup)
    (db_dir / 'personal_moves.bin').write_bytes(moves_bytes)
    print(f"  Written {len(moves_bytes)} bytes "
          f"({len(moves_bytes) // MOVE_ENTRY_SIZE} entries)")

    print("Building personal_items.bin …")
    items_bytes = build_items_binary(item_enum, type_lookup)
    (db_dir / 'personal_items.bin').write_bytes(items_bytes)
    print(f"  Written {len(items_bytes)} bytes "
          f"({len(items_bytes) // ITEM_ENTRY_SIZE} entries)")

    # ── Update text name files ────────────────────────────────────────────────
    print("Writing text name files …")
    (db_dir / 'species.txt').write_text(build_species_txt(species_names), encoding='utf-8')
    (db_dir / 'abilities.txt').write_text(build_name_txt(ability_enum), encoding='utf-8')
    (db_dir / 'moves.txt').write_text(build_name_txt(move_enum), encoding='utf-8')
    (db_dir / 'items.txt').write_text(build_name_txt(item_enum), encoding='utf-8')
    print("  Done.")

    print("\nAll done! Recompile the project to pick up the new databases.")
    print("  The binary format has changed (ability slots are now uint16 LE).")
    print("  Ensure you compile with the updated include/pokemondb.hpp and source/pokemondb.cpp.")


if __name__ == '__main__':
    main()
