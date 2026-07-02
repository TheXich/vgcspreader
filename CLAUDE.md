# CLAUDE.md — vgcspreader

Calculadora de daño VGC (Video Game Championship) con optimización de EV spreads para **Pokémon Champions** (Gen 10). Proyecto Qt5/C++.

---

## Compilar

### Windows (desarrollo local)

```bash
export PATH="/c/Qt/Tools/mingw810_64/bin:/c/Qt/5.15.2/mingw81_64/bin:$PATH"
cd "c:/Users/ximob/OneDrive/Documentos/GitHub/vgcspreader"
mingw32-make -f Makefile.Release.Release
```

Si se modifica `vgcspreader.pro` (añadir fuentes, icono, etc.), regenerar primero con qmake:
```bash
export PATH="/c/Qt/Tools/mingw810_64/bin:/c/Qt/5.15.2/mingw81_64/bin:$PATH"
qmake vgcspreader.pro -o Makefile.Release CONFIG+=release
mingw32-make -f Makefile.Release.Release
```

El ejecutable resultante es `release/vgcspreader.exe`.

Si los cambios son solo en `resources.qrc` (sprites, etc.) y make no los detecta, forzar regeneración:
```bash
rm -f release/qrc_resources.cpp release/qrc_resources.o
mingw32-make -f Makefile.Release.Release
```

### macOS (compilación local)

Para compilar y ejecutar localmente desde Finder, son **tres pasos obligatorios**:

```bash
export PATH="/opt/homebrew/opt/qt@5/bin:$PATH"
make -j$(sysctl -n hw.logicalcpu)
macdeployqt vgcspreader.app
codesign --force --deep --sign - vgcspreader.app
```

- `macdeployqt` copia las frameworks de Qt dentro del `.app` — sin este paso el app crashea al abrirlo desde Finder porque macOS no encuentra las librerías.
- `codesign --sign -` aplica firma ad-hoc local — necesaria para que macOS permita ejecutar el binario.
- Hay que repetir `macdeployqt` + `codesign` después de **cada** `make`, ya que el linking sobreescribe el bundle.

Si solo se modifican archivos que no requieren recompilar (solo `resources.qrc`, etc.), igualmente hay que re-firmar.

### macOS (compilación automática en CI)

La versión macOS se compila automáticamente mediante GitHub Actions en cada push a `master`. No requiere intervención manual. El workflow `.github/workflows/build.yml` se encarga de:

1. Instalar Qt 5 vía Homebrew en un runner Apple Silicon (`macos-latest`)
2. Generar el icono `.icns` desde `logo.png`
3. Compilar con `qmake` + `make`
4. Empaquetar con `macdeployqt` y firmar ad-hoc
5. Crear `VGCSpreader-macOS.dmg` como artefacto descargable desde la pestaña Actions de GitHub

Para generar una **release pública** con el `.dmg` adjunto como descarga permanente:
```bash
git tag v1.X
git push --tags
```

### Actualizar el icono de la app

`ico_preview.png` es el archivo fuente del icono (diseño actual). `logo.png` es la copia que usa el CI para generar el `.icns`.

Para actualizar el icono:
1. Colocar el nuevo diseño en `ico_preview.png`
2. Copiar sobre `logo.png`: `cp ico_preview.png logo.png`
3. Regenerar el iconset localmente:
```bash
mkdir -p macos/vgcspreader.iconset
sips --resampleHeightWidth 16   16   logo.png --out macos/vgcspreader.iconset/icon_16x16.png
sips --resampleHeightWidth 32   32   logo.png --out macos/vgcspreader.iconset/icon_16x16@2x.png
sips --resampleHeightWidth 32   32   logo.png --out macos/vgcspreader.iconset/icon_32x32.png
sips --resampleHeightWidth 64   64   logo.png --out macos/vgcspreader.iconset/icon_32x32@2x.png
sips --resampleHeightWidth 128  128  logo.png --out macos/vgcspreader.iconset/icon_128x128.png
sips --resampleHeightWidth 256  256  logo.png --out macos/vgcspreader.iconset/icon_128x128@2x.png
sips --resampleHeightWidth 256  256  logo.png --out macos/vgcspreader.iconset/icon_256x256.png
sips --resampleHeightWidth 512  512  logo.png --out macos/vgcspreader.iconset/icon_256x256@2x.png
sips --resampleHeightWidth 512  512  logo.png --out macos/vgcspreader.iconset/icon_512x512.png
sips --resampleHeightWidth 1024 1024 logo.png --out macos/vgcspreader.iconset/icon_512x512@2x.png
iconutil -c icns macos/vgcspreader.iconset -o macos/vgcspreader.icns
```
4. Hacer commit de `ico_preview.png`, `logo.png`, `macos/vgcspreader.icns` y `macos/vgcspreader.iconset/`
5. Push a `master` → GitHub Actions regenera el `.icns` y compila el `.dmg` automáticamente

### Regla de paridad de plataformas

**Toda modificación al proyecto debe funcionar en ambas plataformas.** Antes de hacer commit, verificar:

- Código nuevo: no usar APIs ni includes exclusivos de Windows (`<windows.h>`, `WinAPI`, etc.). Qt y la STL de C++17 son portables.
- `vgcspreader.pro`: cambios en `SOURCES`, `HEADERS` o `RESOURCES` aplican a ambas plataformas automáticamente. Cambios de icono, firma o deployment deben ir dentro de `win32 {}` o `macx {}` según corresponda.
- Base de datos binaria (`db/`): los archivos `.bin` y `.txt` son cross-platform, no requieren cambios.
- Scripts Python (`tools/`): ya son portables.
- Si se añade una dependencia externa nueva (librería), debe estar disponible también en macOS vía Homebrew o estar incluida en el propio repositorio.

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

### `personal_items.bin` — 6 bytes por entrada, indexado por `Items` enum

```
[0]    uint8   is_removable (1 si se puede quitar con Knock Off, etc.)
[1]    uint8   is_reducing_berry (1 si reduce daño de un tipo)
[2]    uint8   reducing_berry_type (índice del enum Type)
[3]    uint8   is_restoring_berry (1 si restaura HP)
[4]    uint8   restoring_activation (% de HP por debajo del cual se activa)
[5]    uint8   restoring_percentage (% de HP que restaura)
```

> **Nota**: la función `isItemRemovable` en `pokemondb.hpp` lee por error de `readMovesData` en lugar de `readItemsData` — bug preexistente que afecta a Knock Off.  
> La mecánica de Leftovers **no** usa el binario: se detecta directamente comparando el índice del enum con `Items::Leftovers` mediante `Item::isLeftovers()`.

### Archivos de texto en `db/`

Uno por línea, en el mismo orden que el enum correspondiente:
- `moves.txt` — 421 nombres (sincronizado con `Moves` enum)
- `species.txt` — 1026 nombres (Pokédex)
- `abilities.txt` — 272 nombres
- `items.txt` — 55 nombres (sincronizado con `Items` enum; índice 0 = None, índice 54 = Leftovers)
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
typedef std::tuple<float, int16_t, int16_t, Type, bool, bool, bool, bool, bool, bool> defense_modifier;
// campos: HP%, mod_DEF, mod_SPDEF, tera_type, terastallized, sword_of_ruin, beads_of_ruin, tablets_of_ruin, vessel_of_ruin, helping_hand

// attack_modifier: por turno en el cálculo ofensivo
typedef std::tuple<int16_t, int16_t, Type, bool, bool, bool, bool, bool, bool> attack_modifier;
// campos: mod_ATK, mod_SPATK, tera_type, terastallized, tablets_of_ruin, vessel_of_ruin, sword_of_ruin, beads_of_ruin, helping_hand
```

---

## Flujo de cálculo principal

1. El usuario configura su Pokémon en el panel principal (`mainwindow`)
2. Añade ataques defensivos en `DefenseMoveWindow` → se guardan en `turns_def` / `modifiers_def` / `thresholds_def`
3. Añade ataques ofensivos en `AttackMoveWindow` → se guardan en `turns_atk` / `modifiers_atk` / `thresholds_atk`
4. `calculate()` llama a `Pokemon::calculateEVSDistrisbution(EVCalculationInput)` en un thread separado (`QtConcurrent::run`)
5. Internamente: `resistMove` (mínimos EVs defensivos) + `koMove` (mínimos EVs ofensivos), con threads internos en `resistMoveLoopThread`
6. El resultado se muestra en `ResultWindow`

### Naturaleza automática

Si el Pokémon tiene naturaleza neutra (Hardy, Docile, Serious, Bashful, Quirky) o `AUTO_NATURE`, `calculateEVSDistrisbution` prueba las **16 naturalezas beneficiosas** (las que suben ATK, SpATK, DEF o SpDEF — excluye las que suben SPE y las neutras) y elige la mejor. La naturaleza elegida se actualiza en `*this` antes de retornar y se muestra en el resultado.

Las naturalezas que **suben SPE** (Timid, Jolly, Hasty, Naive) nunca se prueban automáticamente.

**Lax** (+Def/−SpDef) y **Gentle** (+SpDef/−Def) también están excluidas: al penalizar uno de los dos stats defensivos pueden agotar el cap de 32 SPs sin cubrir los umbrales, dando resultados peores que sus equivalentes sin penalización defensiva (Bold e Impish, Calm y Careful).

#### Criterio de selección de naturaleza (implementado en `source/pokemon.cpp`)

El criterio es **dos niveles**:

1. **Primario — maximizar cobertura mínima**: se calcula el mínimo de `def_ko_prob[0][j]` sobre todos los movimientos defensivos `j`. Una naturaleza que penaliza un stat defensivo (p.ej. Lax baja SpDef) puede agotar los 32 SPs máximos sin alcanzar el umbral requerido, dando cobertura real inferior aunque use menos SPs en total.
2. **Secundario — minimizar SPs totales**: entre naturalezas con igual cobertura mínima, se elige la que requiere menos SPs.

> **Bug histórico corregido**: el criterio original era solo "menos SPs totales", lo que causaba que el algoritmo eligiera p.ej. Lax (+Def/−SpDef) sobre Bold (+Def/−Atk) cuando había ataques físicos Y especiales, porque Lax ahorraba SPs en Def pero dejaba SpDef sin cubrir al cap de 32 SPs. El resultado mostraba 56% de resistencia a movimientos especiales en vez del 100% que daría Bold.

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

#### Arquitectura
- `Move::multi_hit_count` (campo en `include/move.hpp`, default=1): golpes que da el movimiento en un solo uso.
- `Turn::getMovesEffective()` (`source/turn.cpp`) repite cada entry del vector `multi_hit_count` veces por ciclo HKO.
- El campo se guarda en el Turn (via Move), por lo que los presets lo preservan automáticamente.

#### UI (ambas ventanas: attack y defense)
- Al seleccionar un movimiento multi-golpe aparece el spinbox **"Hits:"** junto al selector de movimiento. Se oculta para movimientos normales.
- Spinbox editable (variable hits): Bullet Seed, Icicle Spear, Pin Missile, Rock Blast, Tail Slap, Scale Shot, Water Shuriken → 2–5; Arm Thrust → 3–5; Population Bomb → 1–10.
- Spinbox visible pero desactivado (hits fijos): Double Hit/Kick, Dual Chop, Bonemerang, Gear Grind, Tachyon Cutter → 2; Surging Strikes → 3; Triple Axel → 3.
- La función helper `multiHitRange(Moves) → {min, max}` está definida como `static` al principio de `attackmovewindow.cpp` y `defensemovewindow.cpp` (duplicada).

#### Casos especiales
- **Surging Strikes**: hits fijos = 3, cada golpe es siempre crit (manejado por `isAlwaysCrit()` en `pokemon.cpp`, no por la UI).
- **Wicked Blow**: 1 golpe, siempre crit (`isAlwaysCrit()`). No tiene spinbox multi-golpe.
- **Triple Axel**: hits fijos = 3, BP escalante (20 → 40 → 60). Implementado en `Turn::getMovesEffective()` (`source/turn.cpp`): genera 3 entradas con BP hardcodeado 20/40/60, ignorando el BP del binario y el `multi_hit_count`.
- **`isAlwaysCrit()`** en `pokemon.cpp` maneja Surging Strikes y Wicked Blow.

### Daño variable (Eruption/Water Spout/Dragon Energy)
- BP = base_power × (HP_actual / HP_max), mínimo 1

### Grassy Terrain
- La recuperación de HP se muestra como nota de texto en el resultado, **no** se resta del daño calculado (igual que Showdown)

### Weather Ball
- Implementado en `getDamage()` (`source/pokemon.cpp`), antes de `calculateAttackInMove`.
- Si hay clima activo (y el atacante no tiene Cloud Nine / Air Lock): BP → 100, tipo cambia según el clima:
  - Sun / Harsh Sunshine → Fire
  - Rain / Heavy Rain → Water
  - Sand → Rock
  - Snow → Ice
  - Strong Winds → Normal (pero 100 BP)
- Sin clima: Normal, 50 BP (valores del binario sin modificar).
- Al ser Fire en Sol, el move recibe automáticamente el ×1.5 de `calculateWeatherModifier` (igual con Water en Lluvia).

### Efectos de clima (Sand y Snow) sobre stats defensivos
- **Sand** (Sandstorm): +50% SpDef a Pokémon de tipo Rock. Implementado en `calculateDefenseInMove` en los branches de SpDef.
- **Snow**: +50% Def a Pokémon de tipo Ice. Implementado en `calculateDefenseInMove` en los branches de Def (incluyendo Darkest Lariat / Sacred Sword).
- Ambos efectos se suprimen si el defensor tiene Cloud Nine o Air Lock.
- Sand y Snow **no** boostean daño de tipo Rock ni Ice respectivamente (no hay boost en `calculateWeatherModifier`).

### Enum `Move::Weather` — valores completos
```
WEATHER_NONE = 0, SUN = 1, RAIN = 2, HARSH_SUNSHINE = 3,
HEAVY_RAIN = 4, STRONG_WINDS = 5, SAND = 6, SNOW = 7
```
El valor se serializa como entero en XML/saves → añadir siempre al final para no romper datos existentes.
Los combo boxes de Weather en ambas ventanas listan los 8 valores en ese orden.

### Default weather por habilidad del atacante

El clima se actualiza automáticamente en dos momentos distintos:

**1. Al abrir una ventana nueva** (`openMoveWindowDefense` / `openMoveWindowAttack` en `mainwindow.cpp`):
- Lee la habilidad directamente del combo `defending_abilities_combobox` del panel principal (NO de `selected_pokemon`, que solo existe tras pulsar Calcular y sería null).
- Llama a `setDefaultWeather(Move::Weather)` en la ventana correspondiente.
- Solo aplica al abrir; al editar un movimiento ya guardado, el clima guardado se restaura sin cambios.

**2. Al seleccionar / cambiar forma del atacante dentro de `DefenseMoveWindow`**:
- `setSpecies1`, `setForm1`, `setSpecies2`, `setForm2` comprueban la habilidad del nuevo Pokémon tras asignarla y actualizan el combo de Weather si induce clima.
- Guard obligatorio: `if( modifier_groupbox )` antes de acceder — las señales `currentIndexChanged` se disparan durante la construcción del diálogo, cuando `modifier_groupbox` aún es `nullptr` (se crea en `createDefendingGroupBox()`, llamada después de `createAtk1GroupBox()` y `createAtk2GroupBox()`). Sin el guard, crash al arrancar.
- Si el Pokémon seleccionado NO tiene habilidad de clima, el combo de Weather **no se toca** (el usuario puede haberlo puesto manualmente).

**Helper:** `MainWindow::abilityToWeather(Ability)` — método estático público en `mainwindow.hpp` / `mainwindow.cpp`, accesible desde cualquier ventana.

| Habilidad | Clima |
|-----------|-------|
| `Drought` | Sun |
| `Desolate_Land` | Harsh Sunshine |
| `Drizzle` | Rain |
| `Primordial_Sea` | Heavy Rain |
| `Delta_Stream` | Strong Winds |
| `Sand_Stream` | Sand |
| `Snow_Warning` | Snow |
| `Orichalcum_Pulse` | Sun |

**Nota:** `setDefaultWeather(Move::Weather)` en `AttackMoveWindow` actúa sobre `move_modifier_groupbox`; en `DefenseMoveWindow` actúa sobre `modifier_groupbox`.

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
| `Levitate` / `Eelevate` | Inmunidad a Tierra |
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
| `Fire_Mane` | ×1.5 a movimientos de tipo Fuego (Mega Pyroar, Gen 10) |

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
- `defense_modifier` get<5/6>: sword_of_ruin, beads_of_ruin (setter en defensor); get<7/8/9>: tablets, vessel, helping_hand (setter en atacante — se aplican reconstruyendo el Turn con una copia del atacante en `resistMoveLoopThread`)
- `attack_modifier` get<4/5/6/7/8>: tablets, vessel, sword, beads, helping_hand (tablets/vessel/helping_hand en atacante; sword/beads en copia local del defensor dentro de `koMove`)

**Nota sobre Foul Play y Tablets of Ruin**: Foul Play usa el ATK del **defensor**, no del atacante, por lo que Tablets of Ruin (que reduce el ATK del atacante) no se aplica en Foul Play. Esto es correcto mecánicamente.

**UI**: Ambas ventanas exponen los mismos 5 checkboxes en su sección "Modifiers:" (Field:):
- `DefenseMoveWindow`: Tablets (−25% Atk), Vessel (−25% SpAtk), Sword (−25% Def), Beads (−25% SpDef), Helping Hand (×1.5)
- `AttackMoveWindow`: ídem

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

## Cambios para Pokémon Champions (Gen 10)

### Sistema de stat points (SPs)
- Los EVs clásicos se reemplazaron por **Stat Points (SPs)**
- Cada SP suma +1 al stat directamente (antes: 4 EVs = +1)
- Máximo por stat: **32 SPs** (antes: 252 EVs)
- Pool total: **66 SPs** repartidos entre los 6 stats (antes: 510 EVs / 192 en cálculo interno)
- Fórmula nueva: `HP = floor((base×2 + 31) × 50/100) + 60 + SP`
- Fórmula stats: `stat = floor((floor((base×2 + 31) × 50/100) + 5 + SP) × nature)`
- Los IVs están fijados a 31 y el nivel a 50 (Champions los fuerza)
- En la GUI los spinboxes tienen rango **0–32** en todos los sitios (mainwindow, attackmovewindow, defensemovewindow)
- En el cálculo interno: `MAX_EVS = 66`, `MAX_EVS_SINGLE_STAT = 32`

### Cambios de movimientos (Champions)
| Movimiento | BP anterior | BP nuevo |
|-----------|-------------|---------|
| Beak Blast | 100 | 120 |
| Fire Lash | 80 | 90 |
| First Impression | 90 | 100 |
| Night Daze | 85 | 90 |
| Spirit Shackle | 80 | 90 |
| Trop Kick | 70 | 85 |
| Infernal Parade | 50 | 65 |
| Mountain Gale | 80 | 120 |

### Movimientos que ahora son Slice (Sharpness)
Dragon Claw, Shadow Claw, Dire Claw (añadidos a `isSlicingMove()` en `pokemon.cpp`)

### Nuevas Mega Evoluciones (Champions/Legends ZA)
La base de datos (`personal_species.bin`) incluye las nuevas Megas de Champions. Fueron añadidas mediante el script `build_db.py` más adiciones manuales:
- **Mega Eelektross** (#604-1): Electric, HP/Atk/Def/SpA/SpD/Spe = 85/145/80/135/90/80. Habilidad: **Eelevate** (índice 274 — nueva habilidad Gen 10).
- **Mega Falinks** (#870-1): Fighting, 65/135/135/70/65/100. Habilidad: **Defiant** (índice 128).
- **Mega Floette** (#670-2): Fairy, HP/Atk/Def/SpA/SpD/Spe = 74/85/87/155/148/102 (BST=651). Forma 2 de Floette (form index 2 en binario, entry 1243). La forma 1 (#670-1) es Floette-E (AZ's Floette, BST=551). Habilidad: Fairy_Aura (índice 187 en `abilities.hpp`).

#### Habilidades añadidas en Champions 1.1.0 (11 Megas nuevas)
Las siguientes megas ya existían en el binario pero sin habilidad correcta asignada. Actualizadas en junio 2026:

| Mega | # Pokédex | Forma | Habilidad | Índice |
|------|-----------|-------|-----------|--------|
| Mega Raichu X | #26-2 | form 2 | Electric_Surge | 226 |
| Mega Raichu Y | #26-3 | form 3 | No_Guard | 99 |
| Mega Staraptor | #398-1 | form 1 | Contrary | 126 |
| Mega Scolipede | #545-1 | form 1 | Shell_Armor | 75 |
| Mega Scrafty | #560-1 | form 1 | Intimidate | 22 |
| Mega Eelektross | #604-1 | form 1 | Eelevate | 274 |
| Mega Pyroar | #668-1 | form 1 | Fire_Mane | 273 |
| Mega Malamar | #687-1 | form 1 | Contrary | 126 |
| Mega Barbaracle | #689-1 | form 1 | Tough_Claws | 181 (ya correcto) |
| Mega Dragalge | #691-1 | form 1 | Regenerator | 144 |
| Mega Falinks | #870-1 | form 1 | Defiant | 128 |

#### Nuevas habilidades Gen 10 (añadidas al enum y abilities.txt)
- **Fire_Mane** (índice 273): habilidad de Mega Pyroar. Boost ×1.5 a todos los movimientos de tipo Fuego del atacante. Implementada en `calculateOtherModifier` en `source/pokemon.cpp`.
- **Eelevate** (índice 274): habilidad de Mega Eelektross. Combina Levitate (inmunidad a Tierra, excluye de grounded checks) + Beast Boost (KO → +1 al stat más alto; este efecto no aplica al cálculo de spreads). Implementada como alias de Levitate en los tres puntos donde se verifica inmunidad a Tierra en `source/pokemon.cpp`.

Los sprites de todas las Megas Champions están en `db/sprites/` + `resources.qrc` en formato **40×30 px** (igual que todos los demás). Fuente primaria: **Smogon minisprites** (`smogon.com/forums/media/minisprites/{name}.png`). Para los que Smogon aún no tiene (26-2, 26-3, 448-2), se redimensionan desde los 120×120 de Serebii. El script `tools/fix_mega_sprites.py` automatiza esto: detecta sprites con tamaño incorrecto, descarga el minisprite correcto de Smogon/PS, y redimensiona como fallback.

**Referencia de sprites Champions en Serebii** (solo para descarga inicial, luego redimensionar con el script): `https://www.serebii.net/pokemonhome/pokemon/small/NNN-m.png` donde NNN es el número de Pokédex con ceros. Para Z-variants: `-mz.png`. Para variantes X/Y: `-mx.png`, `-my.png`.

### Habilidades en el binario para formas Mega
Cuando una forma Mega tiene la habilidad incorrecta en el binario, hay que parchearlo directamente con Python:
```python
import struct
POKEMON_OFFSET = 84; FORM_OFFSET = 28; ABILITY_OFFSET = 8
with open('db/personal_species.bin', 'rb') as f: data = bytearray(f.read())
dex = 670  # número de Pokédex
form_ptr = struct.unpack_from('<H', data, POKEMON_OFFSET * dex + FORM_OFFSET)[0]
form_num = 2  # 1=primera forma alterna, 2=segunda, etc.
off = (form_ptr + form_num - 1) * POKEMON_OFFSET + ABILITY_OFFSET
struct.pack_into('<H', data, off, 187)  # 187 = Fairy_Aura
with open('db/personal_species.bin', 'wb') as f: f.write(data)
```
**Caso real corregido**: Mega Floette (#670-2) tenía `Flower_Veil` (166) en lugar de `Fairy_Aura` (187) — parcheado en el binario.

---

## Roll de daño y umbral por movimiento

### Mecánica del roll

Cada ataque genera 16 valores de daño posibles (multiplicadores del 85/100 al 100/100 en pasos de 1/100). `getDamage()` los calcula todos; `getKOProbability()` devuelve qué porcentaje de esos 16 rolls resultan en KO (escala 0–100, donde 0 = ningún roll mata = sobrevive el 100%).

### Umbral configurable por movimiento

El usuario puede especificar, para cada movimiento añadido, qué porcentaje de rolls quiere cubrir:

- **Defensivo** (`DefenseMoveWindow`): combo **"Survive:"** — desde "100% (all rolls)" (default, busca spread que sobreviva el peor roll) hasta "50% (8/16)" (acepta que la mitad de los rolls maten).
- **Ofensivo** (`AttackMoveWindow`): combo **"KO:"** — desde "100% (all rolls)" (default, busca spread que mate con el mejor roll) hasta "50% (8/16)".

Los valores disponibles son los 9 umbrales significativos en pasos de 1/16: 100%, 93.75%, 87.5%, 81.25%, 75%, 68.75%, 62.5%, 56.25%, 50%.

### Flujo de datos

```
DefenseMoveWindow::getRollThreshold()  → MainWindow::thresholds_def[i]
AttackMoveWindow::getRollThreshold()   → MainWindow::thresholds_atk[i]
                                             ↓
                              EVCalculationInput::def_roll_thresholds
                              EVCalculationInput::atk_roll_thresholds
                                             ↓
                              Pokemon::def_roll_thresholds  (vector<float>)
                              Pokemon::atk_roll_thresholds  (vector<float>)
```

### Uso en el cálculo

- `resistMoveLoopThread`: rechaza un spread si `ko_prob > def_roll_thresholds[i]` (antes fijo `> 0`).
- `resistMoveLoop` (fallback): umbral base es `def_roll_thresholds[i]`, sobre el que se aplica la tolerancia incremental.
- `koMove`: rechaza si `ko_prob < atk_roll_thresholds[i]` (antes fijo `< 100`).
- `koMove` (fallback): umbral base es `atk_roll_thresholds[i]`, restando la tolerancia incremental.
- Si el vector está vacío o el índice supera su tamaño, se usa el default seguro (0.0 defensivo, 100.0 ofensivo).

### Comportamiento en edición y borrado

- Al editar un movimiento ya añadido (`openMoveWindowEditDefense/Attack`), el combo se restaura al valor guardado.
- Al borrar una fila de la tabla, el threshold se elimina del vector en el índice correspondiente.
- `clearAll()` limpia ambos vectores junto con el resto.

> **Nota**: los presets XML y las partidas guardadas (`SavedCalculation`) no almacenan el roll threshold — al cargarlos se usa el default (100%).

---

## Recuperación al final de turno (EOT) en cálculo multi-turno

### Arquitectura

La función `recursiveDamageCalculation` en `source/pokemon.cpp` aplica efectos de fin de turno entre hits consecutivos. La condición correcta para detectar el final de cada turno (excepto el turno final, donde el Pokémon cae) es:

```cpp
unsigned int entries_per_turn = theVector.size() / theHitNumber;
unsigned int dist_plus1 = (unsigned int)std::distance(theVector.begin(), it) + 1;
if( entries_per_turn > 0 && dist_plus1 % entries_per_turn == 0 && dist_plus1 != theVector.size() )
```

donde `theHitNumber = turn.getHits()` = número de repeticiones de turno (valor del spinbox HKO − 1).

> **Bug histórico corregido**: la condición original `distance % theHitNumber == 0` solo era correcta para 3HKO; para 4HKO y 5HKO no disparaba el EOT en los turnos intermedios.

### Leftovers

- Cura **1/16 del HP máximo** al final de cada turno en que el Pokémon sobrevive.
- Se detecta con `Item::isLeftovers()` (comparación directa con `Items::Leftovers`, sin leer el binario).
- Es removible (Knock Off), lo que se refleja en `is_removable = 1` en `personal_items.bin`.
- El result window muestra `(Leftovers recovery factored in)` cuando el HKO es > 1 turno.
- **No** se aplica en el turno en que el Pokémon cae (la condición `*it_last < maxHP` lo evita).

### Grassy Terrain

La recuperación de Grassy Terrain **no** se resta del daño calculado (se muestra solo como nota de texto en el resultado, igual que Showdown).

---

## Verificación de datos de movimientos

Ante la sospecha de que un movimiento tenga BP, tipo o categoría incorrectos en `personal_moves.bin`, contrastar siempre con estas tres fuentes antes de editar:

1. **Serebii** — https://www.serebii.net/ → buscar el movimiento en la sección de ataques de la generación correspondiente
2. **NCP VGC Damage Calculator (nerd-of-now)** — https://github.com/nerd-of-now/NCP-VGC-Damage-Calculator → ver los datos de movimientos que usa la calculadora de referencia
3. **WikiDex** — https://www.wikidex.net/wiki/WikiDex → buscar el movimiento para confirmar BP, tipo y categoría en español

Si las tres fuentes coinciden con un valor distinto al del binario, corregir con el script Python:
```python
import struct
path = 'db/personal_moves.bin'
with open(path, 'rb') as f:
    data = bytearray(f.read())
off = INDEX * 8  # INDEX = índice en moves.txt (0-based)
struct.pack_into('<H', data, off, NEW_BP)
with open(path, 'wb') as f:
    f.write(data)
```

> **Correcciones históricas**: Dire Claw (idx 380) tenía BP=60, corregido a 80. Last Respects (idx 395) tenía BP=65, corregido a 50.

---

## Panel de IV oculto (Champions)

En Pokémon Champions los IVs están siempre fijados a 31 y no son modificables por el jugador. Por ello los controles de IV están **ocultos** (no eliminados) en la GUI con `->hide()`, de modo que toda la lógica de cálculo permanece intacta con los valores por defecto 31.

### Widgets ocultados

- **`mainwindow.cpp`** (~línea 490): `iv_groupbox->hide()` — oculta el group box completo "IV:" del panel del Pokémon propio.
- **`attackmovewindow.cpp`** (cálculo ofensivo): `hp_iv_label->hide()`, `hp_iv->hide()`, `iv_label->hide()` (`def_iv_label`), `iv->hide()` (`def_iv_spinbox`) — oculta HP IV y Def/SpDef IV del defensor.
- **`defensemovewindow.cpp`** (cálculo defensivo): para `atk1` y `atk2`, `iv_label->hide()` y `iv->hide()` — oculta Atk/SpAtk IV de los atacantes.

Los objetos siguen existiendo con nombre (ObjectName) para los `findChild<>` que los leen/escriben internamente. Si en el futuro los IVs vuelven a ser configurables, basta con eliminar las líneas `->hide()`.

---

## Pendientes / mejoras conocidas

- El typo `Psichic` en el enum (`moves.hpp`) debería ser `Psychic`, pero corregirlo rompería los índices binarios ya guardados en presets XML de usuarios
- Los presets XML no guardan el `attack_modifier` completo (Tera del atacante por turno); si se añade en el futuro habría que versionar el formato XML
- Los presets XML y `SavedCalculation` no guardan el roll threshold por movimiento (`thresholds_def` / `thresholds_atk`); al cargar, todos los movimientos usan el default (100% defensivo / 100% ofensivo)
- Los presets XML guardan los 5 checkboxes de Ruin + Helping Hand del `defense_modifier` (get<5..9>); los campos son opcionales en la carga (fallback a false) para compatibilidad con presets antiguos
- `NATURE_NUM` y `AUTO_NATURE` tienen el mismo valor numérico (25); el combobox de naturaleza tiene 26 ítems (índices 0–24 = naturales reales, índice 25 = Auto)
- El enum `Status` usa `NO_STATUS` (no `HEALTHY`) como valor neutro — importante al implementar habilidades que dependen del estado
- Algunos sprites Champions no tienen minisprite en Smogon/PS aún: Raichu Mega (#26-2, #26-3), Lucario Gmax (#448-2) — se usan versiones redimensionadas de los Serebii 120×120. Formas Totem (Raticate #20, Marowak #105), Maushold-Three, Toxtricity-Low-Key-Gmax tampoco tienen minisprite disponible.
- Los nombres de formas en la GUI son genéricos ("Form 1", "Form 2"…); hay código comentado que sugería usar `db/forms.txt` — pendiente de implementar nombres propios por forma
- El pool total de SPs es **66** (`MAX_EVS = 66` en `pokemon.cpp`), con máximo **32** por stat (`MAX_EVS_SINGLE_STAT = 32`). El cálculo de spreads óptimos usa la suma de SPs asignados como proxy de "coste total" y respeta ambos límites.
