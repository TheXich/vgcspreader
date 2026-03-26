# CLAUDE.md — vgcspreader

Calculadora de daño VGC (Video Game Championship) con optimización de EV spreads. Proyecto Qt5/C++.

---

## Compilar

```bash
export PATH="/c/Qt/Tools/mingw810_64/bin:$PATH"
cd "c:/Users/ximob/OneDrive/Documentos/GitHub/vgcspreader"
mingw32-make -f Makefile.Release
```

El ejecutable resultante es `release/vgcspreader.exe`.

Si los cambios son solo en `resources.qrc` (sprites, etc.) y make no los detecta, forzar regeneración:
```bash
rm -f release/qrc_resources.cpp release/qrc_resources.o
mingw32-make -f Makefile.Release
```

---

## Stack

- **Qt 5.15.2** con MinGW 8.1.0 64-bit (`C:/Qt/`)
- **C++17**, módulos `widgets` y `concurrent`
- **TinyXML2** para presets en XML
- **Python 3** para scripts de mantenimiento de la base de datos (`tools/`)

---

## Estructura del proyecto

```
include/          Headers del modelo (Stats, Pokemon, Move, Turn, etc.)
include/gui/      Headers de ventanas Qt
source/           Implementaciones del modelo
source/gui/       Implementaciones de ventanas Qt
db/               Base de datos binaria + textos de nombres
db/sprites/       Sprites PNG individuales por número de Pokédex
tools/            Scripts Python para mantenimiento
resources.qrc     Manifiesto de recursos Qt (sprites, db)
```

### Archivos clave del modelo

| Archivo | Responsabilidad |
|---------|----------------|
| `include/stats.hpp` | Enum `Stat`, enum `Nature` (incluye `AUTO_NATURE`) |
| `include/moves.hpp` | Enum `Moves` (421 entradas, Gen 1–9 + DLC) |
| `include/types.hpp` | Enum `Type` |
| `include/pokemon.hpp` | Clase `Pokemon`, typedefs `defense_modifier`, `attack_modifier` |
| `source/pokemon.cpp` | Toda la lógica de cálculo de daño y EV spread |

### Archivos clave de la GUI

| Archivo | Ventana |
|---------|---------|
| `mainwindow` | Ventana principal: panel del Pokémon propio, lista de ataques |
| `defensemovewindow` | Configurar ataques entrantes (cálculo defensivo) |
| `attackmovewindow` | Configurar ataques salientes (cálculo ofensivo) |
| `resultwindow` | Mostrar el EV spread óptimo y los cálculos |

---

## Base de datos binaria

Todos los archivos binarios en `db/` deben mantenerse sincronizados con los enums en `include/`.

### `personal_moves.bin` — 8 bytes por entrada, indexado por `Moves` enum

```
[0-1]  uint16  base_power (little-endian)
[2]    uint8   type (índice del enum Type)
[3]    uint8   category  (0=Physical, 1=Special, 2=Status)
[4]    uint8   is_spread (1 si golpea a ambos rivales)
[5-6]  uint16  z_power (0 si no aplica)
[7]    uint8   is_signature_z
```

### `personal_species.bin` — 84 bytes por entrada, indexado por número de Pokédex

```
[0-5]   uint8×6  base stats (HP, ATK, DEF, SpATK, SpDEF, SPE)
[6-7]   uint8×2  tipo1, tipo2
[8-13]  uint16×3 ability0, ability1, abilityH
[28-29] uint16   índice de la primera forma alternativa (0 si no hay)
[32]    uint8    número total de formas
```

El orden de stats en el binario es `HP, ATK, DEF, SpATK, SpDEF, SPE`, pero el enum `Stats::Stat` es `HP=0, ATK=1, DEF=2, SPE=3, SPATK=4, SPDEF=5`. Hay un **remap** en `pokemondb.hpp`:
```cpp
static constexpr int remap[] = {0, 1, 2, 5, 3, 4};
```

### `personal_items.bin` — 6 bytes por entrada

```
[0-1]  uint16  base power bonus
[2]    uint8   type
[3]    uint8   category flag
[4-5]  reserved
```

### Archivos de texto en `db/`

Uno por línea, en el mismo orden que el enum correspondiente:
- `moves.txt` — 421 nombres (sincronizado con `Moves` enum)
- `species.txt` — 1026 nombres (Pokédex)
- `abilities.txt` — 272 nombres
- `items.txt` — 35 nombres
- `types.txt` — 18 nombres
- `natures.txt` — 25 nombres (Hardy…Quirky, sin "Auto")

### Sprites

PNGs en `db/sprites/`, nombrados `{dex}.png` o `{dex}-{form}.png`. Todos listados en `resources.qrc`. Disponibles para #1–#1025 + formas alternativas.

---

## Tipos importantes

```cpp
// defense_modifier: por turno en el cálculo defensivo
typedef std::tuple<float, int16_t, int16_t, Type, bool> defense_modifier;
// campos: HP%, mod_DEF, mod_SPDEF, tera_type, terastallized

// attack_modifier: por turno en el cálculo ofensivo
typedef std::tuple<int16_t, int16_t, Type, bool> attack_modifier;
// campos: mod_ATK, mod_SPATK, tera_type, terastallized
```

---

## Flujo de cálculo principal

1. El usuario configura su Pokémon en el panel principal (`mainwindow`)
2. Añade ataques defensivos en `DefenseMoveWindow` → se guardan en `turns_def` / `modifiers_def`
3. Añade ataques ofensivos en `AttackMoveWindow` → se guardan en `turns_atk` / `modifiers_atk`
4. `calculate()` llama a `Pokemon::calculateEVSDistrisbution(EVCalculationInput)` en un thread separado (`QtConcurrent::run`)
5. Internamente: `resistMove` (mínimos EVs defensivos) + `koMove` (mínimos EVs ofensivos), con threads internos en `resistMoveLoopThread`
6. El resultado se muestra en `ResultWindow`

### Naturaleza automática

Si el Pokémon tiene naturaleza neutra (Hardy, Docile, Serious, Bashful, Quirky) o `AUTO_NATURE`, `calculateEVSDistrisbution` prueba las **16 naturalezas beneficiosas** (las que suben ATK, SpATK, DEF o SpDEF — excluye las que suben SPE y las neutras) y elige la que usa menos EVs totales. La naturaleza elegida se actualiza en `*this` antes de retornar y se muestra en el resultado.

Las naturalezas que **suben SPE** (Timid, Jolly, Hasty, Naive) nunca se prueban automáticamente.

---

## Mecánicas implementadas

### Tera type
- `defense_modifier` y `attack_modifier` incluyen `Type tera_type` y `bool terastallized`
- Se aplica al Pokémon defensor en `resistMoveLoopThread` (std::get<3/4>)
- Se aplica al atacante en `koMove` (std::get<2/3>)
- `calculateStabModifier`: con Tera activo, STAB = ×2 si el tipo del move coincide con tera_type, ×1.5 si el move coincide con un tipo original, ×1 en otro caso
- `calculateTypeModifier`: usa `tera_type` como tipo efectivo cuando `terastallized == true`
- El resultado muestra "Tera-{Tipo} NombrePokemon" cuando está activo

### Movimientos multi-golpe
- Surging Strikes (índice 372): Water/Physical/25 BP × 3, siempre crit
- Wicked Blow (índice 373): Dark/Physical/75 BP, siempre crit
- `isAlwaysCrit()` en `pokemon.cpp` maneja ambos

### Daño variable (Eruption/Water Spout/Dragon Energy)
- BP = base_power × (HP_actual / HP_max), mínimo 1

### Grassy Terrain
- La recuperación de HP se muestra como nota de texto en el resultado, **no** se resta del daño calculado (igual que Showdown)

### Habilidades con boost climático
- Orichalcum Pulse (Koraidon): ×1.3 en Atk bajo Sol
- Hadron Engine (Miraidon): ×1.3 en SpAtk bajo Electric Terrain

---

## Convenciones de código

- Los enums nuevos (movimientos, etc.) se **añaden al final** del enum existente para preservar los índices binarios
- Los campos `get<0>`, `get<1>`, etc. en tuples corresponden al orden documentado en el typedef
- El Pokémon defensor recibe su Tera ANTES de llamar a `getKOProbability` en todos los loops de cálculo
- `abort_calculation` es un campo de instancia en `Pokemon`; al clonar el Pokémon para el auto-nature, el abort se detecta entre naturalezas (no mid-nature)

---

## Scripts de mantenimiento

### `tools/build_db.py`
Regenera los binarios desde datos de pokemon-showdown. Requiere clon local de pokemon-showdown.
```bash
python tools/build_db.py --showdown <ruta-showdown> --project .
```

### `tools/download_sprites.py`
Descarga sprites de Smogon para Pokédex #808–#1025 y actualiza `resources.qrc`.
```bash
python tools/download_sprites.py
```

---

## Añadir movimientos nuevos

1. Añadir entrada al final del enum `Moves` en `include/moves.hpp`
2. Añadir nombre al final de `db/moves.txt`
3. Añadir entrada binaria (8 bytes) al final de `db/personal_moves.bin`:
   ```python
   import struct
   entry = struct.pack('<H', base_power) + bytes([type_idx, category, is_spread]) + struct.pack('<H', 0) + bytes([0])
   ```
4. Recompilar

Los tres ficheros deben tener siempre el **mismo número de entradas**.

---

## Pendientes / mejoras conocidas

- El typo `Psichic` en el enum (`moves.hpp`) debería ser `Psychic`, pero corregirlo rompería los índices binarios ya guardados en presets XML de usuarios
- Los presets XML no guardan el `attack_modifier` completo (Tera del atacante por turno); si se añade en el futuro habría que versionar el formato XML
- `NATURE_NUM` y `AUTO_NATURE` tienen el mismo valor numérico (25); el combobox de naturaleza tiene 26 ítems (índices 0–24 = naturales reales, índice 25 = Auto)
