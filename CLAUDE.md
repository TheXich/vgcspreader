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

PNGs en `db/sprites/`, nombrados `{dex}.png` (forma base) o `{dex}-{form}.png` (formas alternativas). Todos listados en `resources.qrc`.

- Formas base: `{dex}.png` para #1–#1025.
- Formas alternativas: `{dex}-{form_index}.png` donde `form_index` empieza en 1. El índice corresponde al orden en que `build_db.py` encontró las formas en el `pokedex.ts` de Showdown (generalmente alfabético por ID interno de PS).
- El código en la GUI construye la ruta como `":/db/sprites/" + dex + "-" + form_index + ".png"`.
- Disponibles: ~1025 base + ~335 formas alternativas (incluyendo Mega, G-Max, formas regionales Alola/Galar/Hisui/Paldea, formas competitivas Gen 8–9).
- Para añadir sprites nuevos: descargar PNG, colocar en `db/sprites/`, añadir entrada en `resources.qrc`, recompilar.

---

## Tipos importantes

```cpp
// defense_modifier: por turno en el cálculo defensivo
typedef std::tuple<float, int16_t, int16_t, Type, bool, bool, bool> defense_modifier;
// campos: HP%, mod_DEF, mod_SPDEF, tera_type, terastallized, sword_of_ruin, beads_of_ruin

// attack_modifier: por turno en el cálculo ofensivo
typedef std::tuple<int16_t, int16_t, Type, bool, bool, bool, bool, bool, bool> attack_modifier;
// campos: mod_ATK, mod_SPATK, tera_type, terastallized, tablets_of_ruin, vessel_of_ruin, sword_of_ruin, beads_of_ruin, helping_hand
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
- Se aplica al Pokémon defensor en `resistMoveLoopThread` (get<3/4>)
- Se aplica al atacante en `koMove` (get<2/3>)
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

### Habilidades con boost climático/terreno
- **Orichalcum Pulse** (Koraidon): ×1.3 en Atk en movimientos Físicos. Se activa **por defecto** (Koraidon siempre activa Sol al entrar). Solo se suprime si el clima está explícitamente en Rain, Heavy Rain o Strong Winds.
- **Hadron Engine** (Miraidon): ×1.3 en SpAtk en movimientos Especiales. Se activa **por defecto** (Miraidon siempre activa Electric Terrain al entrar). Solo se suprime si el terreno está en Grassy, Psychic o Misty.

### Habilidades defensivas implementadas

| Habilidad | Efecto en cálculo |
|-----------|------------------|
| `Shadow_Shield` / `Multiscale` | ×0.5 daño recibido al 100% HP |
| `Prism_Armor` | ×0.75 a movimientos super-efectivos |
| `Filter` / `Solid_Rock` | ×0.75 a movimientos super-efectivos |
| `Wonder_Guard` | ×0 a todo excepto super-efectivo |
| `Levitate` | Inmunidad a Tierra |
| `Heatproof` | ×0.5 al tipo Fuego |
| `Thick_Fat` | ×0.5 a Fuego e Hielo |
| `Flash_Fire` | Inmunidad al tipo Fuego |
| `Volt_Absorb` | Inmunidad al tipo Eléctrico |
| `Water_Absorb` / `Storm_Drain` | Inmunidad al tipo Agua |
| `Fluffy` | ×0.5 a físicos (contacto), ×2 a Fuego |
| `Water_Bubble` (defensa) | ×0.5 al tipo Fuego |
| `Purifying_Salt` | ×0.5 al tipo Fantasma |
| `Fur_Coat` | ×2 DEF |
| `Marvel_Scale` | ×1.5 DEF cuando tiene estado (quemado/envenenado/paralizado) |
| `Tera_Shell` | Primera vez que es golpeado al 100% HP: efectividad neutra |
| `Neuroforce` | ×1.25 en los super-efectivos del atacante (ofensiva) |

### Habilidades ofensivas implementadas

| Habilidad | Efecto en cálculo |
|-----------|------------------|
| `Huge_Power` / `Pure_Power` | ×2 ATK en ataques físicos (incl. Foul Play sobre defensor) |
| `Technician` | ×1.5 BP si BP ≤ 60 |
| `Blaze` / `Overgrow` / `Torrent` / `Swarm` | ×1.5 al tipo correspondiente a ≤33% HP |
| `Guts` | ×1.5 ATK físico cuando tiene estado |
| `Hustle` | ×1.5 ATK en físicos (sin modelar pérdida de precisión) |
| `Marvel_Scale` | ×1.5 DEF defensiva cuando tiene estado |
| `Iron_Fist` | ×1.2 BP a movimientos de puñetazo |
| `Tough_Claws` | ×1.333 BP a físicos (aproxima contacto = físico) |
| `Tinted_Lens` | ×2 daño a movimientos poco efectivos (NVE) |
| `Sharpness` | ×1.5 BP a movimientos cortantes |
| `Water_Bubble` (ataque) | ×2 a movimientos de tipo Agua |
| `Aerilate` / `Pixilate` / `Refrigerate` / `Galvanize` / `Normalize` | Convierte Normal al tipo correspondiente + ×1.2 BP |
| `Dark_Aura` / `Fairy_Aura` | ×1.33 a movimientos Siniestro/Hada (aplica si atacante **o** defensor tiene el aura) |
| `Adaptability` | STAB sube de ×1.5 a ×2 |
| `Scrappy` | Normal y Lucha golpean a Fantasma |
| `Collision_Course` / `Electro_Drift` | ×1.333 si el movimiento es super-efectivo |

### Habilidades de campo (Ruin abilities) + Helping Hand

Las Ruin abilities son efectos de campo activos mientras el Pokémon correspondiente esté en el campo; reducen el stat de todos los **demás** Pokémon un 25%.

| Habilidad | Stat afectado | Dónde se aplica |
|-----------|---------------|-----------------|
| `Sword_of_Ruin` (Chien-Pao) | −25% DEF del defensor | `calculateDefenseInMove` (todos los branches físicos) |
| `Beads_of_Ruin` (Ting-Lu) | −25% SpDef del defensor | `calculateDefenseInMove` (todos los branches especiales) |
| `Tablets_of_Ruin` (Wo-Chien) | −25% ATK del atacante | `calculateAttackInMove` (físicos, excluye Foul Play) |
| `Vessel_of_Ruin` (Chi-Yu) | −25% SpAtk del atacante | `calculateAttackInMove` (especiales) |

**Helping Hand**: ×1.5 al daño del movimiento del atacante, aplicado en `calculateOtherModifier`.

Estos efectos se almacenan como flags en la clase `Pokemon` (`ruin_sword`, `ruin_beads`, `ruin_tablets`, `ruin_vessel`, `helping_hand`) y se setean desde los modifier tuples antes de cada llamada a `getKOProbability`:
- `defense_modifier` get<5/6>: sword_of_ruin, beads_of_ruin (setter en defensor)
- `attack_modifier` get<4/5/6/7/8>: tablets, vessel, sword, beads, helping_hand (tablets/vessel/helping_hand en atacante; sword/beads en copia local del defensor dentro de `koMove`)

**Nota sobre Foul Play y Tablets of Ruin**: Foul Play usa el ATK del **defensor**, no del atacante, por lo que Tablets of Ruin (que reduce el ATK del atacante) no se aplica en Foul Play. Esto es correcto mecánicamente.

**UI**: Los checkboxes se encuentran en la sección "Modifiers:" de cada ventana:
- `DefenseMoveWindow`: Sword of Ruin (−25% Def) y Beads of Ruin (−25% SpDef)
- `AttackMoveWindow`: los 4 Ruin abilities + Helping Hand (×1.5)

### Notas críticas sobre habilidades
- **`Huge_Power`/`Pure_Power`**: solo aplica ×2 al **atacante** en ataques físicos normales. En **Foul Play** aplica al **defensor** (correcto, porque Foul Play usa el ATK del defensor). En Photon Geyser, solo afecta a la rama de ATK, no a la de SpATK.
- **`Dark_Aura`/`Fairy_Aura`**: verifican tanto `theAttacker.getAbility()` como `getAbility()` (defensor); antes solo chequeaba al defensor.
- **`Orichalcum_Pulse`/`Hadron_Engine`**: lógica invertida — activos por defecto, suprimibles. Ver sección anterior.

---

## Convenciones de código

- Los enums nuevos (movimientos, etc.) se **añaden al final** del enum existente para preservar los índices binarios
- Los campos `get<0>`, `get<1>`, etc. en tuples corresponden al orden documentado en el typedef (`include/pokemon.hpp`)
- El Pokémon defensor recibe su Tera y los flags de Ruin ANTES de llamar a `getKOProbability` en todos los loops de cálculo
- Para las Ruin abilities del defensor en `koMove`, se usa una copia local (`def_copy`) para no mutar el vector `const theDefendingPokemon`
- `abort_calculation` es un campo de instancia en `Pokemon`; al clonar el Pokémon para el auto-nature, el abort se detecta entre naturalezas (no mid-nature)

---

## Ordenación alfabética de los combo boxes (GUI)

Todos los combo boxes de datos de modelo (especies, movimientos, habilidades, ítems, tipos, naturalezas) muestran sus ítems **ordenados alfabéticamente**. El índice visual del combo ≠ índice del enum; el índice original se guarda en `Qt::UserRole`.

### Helpers en `MainWindow` (`include/gui/mainwindow.hpp` + `source/gui/mainwindow.cpp`)

```cpp
static void populateSortedComboBox(QComboBox* combo, const std::vector<QString>& names);
static void setComboByOriginalIdx(QComboBox* combo, int originalIdx);
```

- **`populateSortedComboBox`**: ordena (case-insensitive) los ítems y asigna `Qt::UserRole = índice_original` a cada uno.
- **`setComboByOriginalIdx`**: busca el ítem con el `UserRole` deseado y llama a `setCurrentIndex`. Usar siempre en lugar de `setCurrentIndex(enum_val)` en combos ordenados.

### Reglas de uso

- **Leer valor**: `combo->currentData(Qt::UserRole).toInt()` en lugar de `currentIndex()`.
- **Escribir valor desde modelo** (al cargar especie, preset, edición): `setComboByOriginalIdx(combo, enum_val)`.
- **Naturalezas**: Hardy (índice original 0, neutra) es la selección por defecto. "Auto" se añade al final con `UserRole = natures_names.size()` (25). Al resetear, usar `setComboByOriginalIdx(natures_combo, 0)` para volver a Hardy.
- **Ítems**: None (índice original 0) es la selección por defecto. Al resetear, usar `setComboByOriginalIdx(items_combo, 0)` para volver a None.
- **No ordenados** (dejar con `currentIndex()` normal): formas, weather, terrain, categoría de movimiento, target (Single/Double).
- Los helpers se llaman desde las ventanas hija como `MainWindow::populateSortedComboBox(...)` y `MainWindow::setComboByOriginalIdx(...)`.

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

### `tools/download_form_sprites.py`
Descarga sprites de **formas alternativas** (regionales, Mega, G-Max, formas competitivas Gen 8–9, etc.) desde Smogon/PS y actualiza `resources.qrc`. Contiene un diccionario `FORMS` con ~120 entradas mapeando `(dex, form_index) → nombre_sprite`.
```bash
python tools/download_form_sprites.py
```
- Fuentes: `smogon.com/forums/media/minisprites/` (primario) y `play.pokemonshowdown.com/sprites/gen5/` (fallback).
- Solo descarga los que no existen; salta los ya presentes.
- Después de ejecutarlo, recompilar para incluir los nuevos sprites en el ejecutable.

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
- El enum `Status` usa `NO_STATUS` (no `HEALTHY`) como valor neutro — importante al implementar habilidades que dependen del estado
- Algunas formas del binario no tienen sprite disponible en Smogon/PS: formas Totem (Raticate #20, Marowak #105), Lucario G-Max (no existe en los juegos — posible error en el binario), Zeraora G-Max, Maushold-Three, Toxtricity-Low-Key-Gmax. El juego muestra sprite en blanco para esas formas.
- Los nombres de formas en la GUI son genéricos ("Form 1", "Form 2"…); hay código comentado que sugería usar `db/forms.txt` — pendiente de implementar nombres propios por forma
