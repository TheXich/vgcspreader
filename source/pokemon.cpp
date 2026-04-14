#include <cmath>
#include <climits>
#include <iostream>
#include <algorithm>
#include <thread>
#include <iterator>

#include "pokemon.hpp"
#include "turn.hpp"
#include "move.hpp"

#include <QDebug>

PokemonDB Pokemon::db;
std::mutex Pokemon::buffer_mutex;
std::mutex Pokemon::result_mutex;

/*TYPE CHART INITIALIZATION*/
const float Pokemon::type_matrix[18][18] = {
	{1, 1, 1, 1, 1, 0.5, 1, 0, 0.5, 1, 1, 1, 1, 1, 1, 1, 1, 1},
	{2, 1, 0.5, 0.5, 1, 2, 0.5, 0, 2, 1, 1, 1, 1, 0.5, 2, 1, 2, 0.5},
	{1, 2, 1, 1, 1, 0.5, 2, 1, 0.5, 1, 1, 2, 0.5, 1, 1, 1, 1, 1},
	{1, 1, 1, 0.5, 0.5, 0.5, 1, 0.5, 0, 1, 1, 2, 1, 1, 1, 1, 1, 2},
	{1, 1, 0, 2, 1, 2, 0.5, 1, 2, 2, 1, 0.5, 2, 1, 1, 1, 1, 1},
	{1, 0.5, 2, 1, 0.5, 1, 2, 1, 0.5, 2, 1, 1, 1, 1, 2, 1, 1, 1},
	{1, 0.5, 0.5, 0.5, 1, 1, 1, 0.5, 0.5, 0.5, 1, 2, 1, 2, 1, 1, 2, 0.5},
	{0, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 2, 1, 1, 0.5, 1},
	{1, 1, 1, 1, 1, 2, 1, 1, 0.5, 0.5, 0.5, 1, 0.5, 1, 2, 1, 1, 2},
	{1, 1, 1, 1, 1, 0.5, 2, 1, 2, 0.5, 0.5, 2, 1, 1, 2, 0.5, 1, 1},
	{1, 1, 1, 1, 2, 2, 1, 1, 1, 2, 0.5, 0.5, 1, 1, 1, 0.5, 1, 1},
	{1, 1, 0.5, 0.5, 2, 2, 0.5, 1, 0.5, 0.5, 2, 0.5, 1, 1, 1, 0.5, 1, 1},
	{1, 1, 2, 1, 0, 1, 1, 1, 1, 1, 2, 0.5, 0.5, 1, 1, 0.5, 1, 1},
	{1, 2, 1, 2, 1, 1, 1, 1, 0.5, 1, 1, 1, 1, 0.5, 1, 1, 0, 1},
	{1, 1, 2, 1, 2, 1, 1, 1, 0.5, 0.5, 0.5, 2, 1, 1, 0.5, 2, 1, 1},
	{1, 1, 1, 1, 1, 1, 1, 1, 0.5, 1, 1, 1, 1, 1, 1, 2, 1, 0},
	{1, 0.5, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 2, 1, 1, 0.5, 0.5},
	{1, 2, 1, 0.5, 1, 1, 1, 1, 0.5, 0.5, 1, 1, 1, 1, 1, 2, 2, 1},
};
/*----------------------------------------------------END------------------------------------*/

Pokemon::Pokemon(const unsigned int thePokedexNumber, const Stats& theStats) {
    current_hp_percentage = 100;
    abort_calculation = false;
    tera_type = Type::Typeless;
    terastallized = false;
    ruin_sword = false;
    ruin_beads = false;
    ruin_tablets = false;
    ruin_vessel = false;
    helping_hand = false;
    friend_guard = false;

    pokedex_number = thePokedexNumber;

    formes_number = db.getPokemonFormesNumber(thePokedexNumber);
    form = 0;

    for(unsigned int i = 0; i < formes_number; i++) {
        std::array<uint8_t, 6> buffer;

        buffer[Stats::HP] = db.getPokemonStat(thePokedexNumber, i, Stats::HP);
        buffer[Stats::ATK] = db.getPokemonStat(thePokedexNumber, i, Stats::ATK);
        buffer[Stats::DEF] = db.getPokemonStat(thePokedexNumber, i, Stats::DEF);
        buffer[Stats::SPATK] = db.getPokemonStat(thePokedexNumber, i, Stats::SPATK);
        buffer[Stats::SPDEF] = db.getPokemonStat(thePokedexNumber, i, Stats::SPDEF);
        buffer[Stats::SPE] = db.getPokemonStat(thePokedexNumber, i, Stats::SPE);

        base.push_back(buffer);
    }

    for(unsigned int i = 0; i < formes_number; i++) {
        std::array<Type, 2> buffer;

        buffer[0] = (Type)db.getPokemonType(thePokedexNumber, i)[0];
        buffer[1] = (Type)db.getPokemonType(thePokedexNumber, i)[1];

        types.push_back(buffer);
    }

    for(unsigned int i = 0; i < formes_number; i++) {
        std::array<Ability, 3> buffer;

        buffer[0] = (Ability)db.getPokemonAbilities(thePokedexNumber, i)[0];
        buffer[1] = (Ability)db.getPokemonAbilities(thePokedexNumber, i)[1];
        buffer[2] = (Ability)db.getPokemonAbilities(thePokedexNumber, i)[2];

        possible_abilities.push_back(buffer);
    }

    ability = possible_abilities[form][0];

    stats = theStats;

    status = Status::NO_STATUS;

    calculateTotal();

    //evaluate if it is grounded
    if( types[form][0] == Type::Flying || types[form][1] == Type::Flying || getAbility() == Ability::Levitate ) grounded = false;
    else grounded = true;
}

uint8_t Pokemon::calculateEVSNextStat(Pokemon thePokemon, const Stats::Stat& theStat, const unsigned int theStartingEVS) const {
    thePokemon.setEV(theStat, theStartingEVS);
    //auto base_evs = thePokemon.getEV(theStat);

    auto base_stat = thePokemon.getBoostedStat(theStat);

    while( thePokemon.getBoostedStat(theStat) == base_stat ) thePokemon.setEV(theStat, thePokemon.getEV(theStat) + 1);


    return thePokemon.getEV(theStat) - theStartingEVS;
}

void Pokemon::calculateTotal() {
    //calculate hp
    // Champions formula: HP = floor((base*2 + 31) * 50 / 100) + 60 + SP
    total[Stats::HP] = (2 * base[form][Stats::HP] + 31) * 50 / 100 + 60 + stats.getEV(Stats::HP);
    boosted[Stats::HP] = total[Stats::HP];

    float nature_multiplier = 1;

    //calculate atk
    float atk_modifier_multiplier;
    int16_t atk_modifier = stats.getModifier(Stats::ATK);
    if( atk_modifier == 0) atk_modifier_multiplier = 1;
    else if(atk_modifier > 0) atk_modifier_multiplier = (float(atk_modifier) + 2) / 2;
    else atk_modifier_multiplier = 2 / (float(-atk_modifier) + 2);

    if( stats.getNature() == Stats::LONELY || stats.getNature() == Stats::BRAVE || stats.getNature() == Stats::ADAMANT || stats.getNature() == Stats::NAUGHTY ) nature_multiplier = 1.1;
    else if( stats.getNature() == Stats::BOLD || stats.getNature() == Stats::TIMID || stats.getNature() == Stats::MODEST || stats.getNature() == Stats::CALM ) nature_multiplier = 0.9;
    else nature_multiplier = 1;

    // Champions formula: stat = floor((floor((base*2+31)*50/100) + 5 + SP) * nature)
    total[Stats::ATK] = ((2 * base[form][Stats::ATK] + 31) * 50 / 100 + 5 + stats.getEV(Stats::ATK)) * nature_multiplier;
    boosted[Stats::ATK] = total[Stats::ATK] * atk_modifier_multiplier;

    //calculate def
    float def_modifier_multiplier;
    int16_t def_modifier = stats.getModifier(Stats::DEF);
    if( def_modifier == 0) def_modifier_multiplier = 1;
    else if(def_modifier > 0) def_modifier_multiplier = (float(def_modifier) + 2) / 2;
    else def_modifier_multiplier = 2 / (float(-def_modifier) + 2);

    if( stats.getNature() == Stats::BOLD || stats.getNature() == Stats::RELAXED || stats.getNature() == Stats::IMPISH || stats.getNature() == Stats::LAX ) nature_multiplier = 1.1;
    else if( stats.getNature() == Stats::HASTY || stats.getNature() == Stats::MILD || stats.getNature() == Stats::LONELY || stats.getNature() == Stats::GENTLE ) nature_multiplier = 0.9;
    else nature_multiplier = 1;

    total[Stats::DEF] = ((2 * base[form][Stats::DEF] + 31) * 50 / 100 + 5 + stats.getEV(Stats::DEF)) * nature_multiplier;
    boosted[Stats::DEF] = total[Stats::DEF] * def_modifier_multiplier;

    //calculate spatk
    float spatk_modifier_multiplier;
    int16_t spatk_modifier = stats.getModifier(Stats::SPATK);
    if( spatk_modifier == 0) spatk_modifier_multiplier = 1;
    else if(spatk_modifier > 0) spatk_modifier_multiplier = (float(spatk_modifier) + 2) / 2;
    else spatk_modifier_multiplier = 2 / (float(-spatk_modifier) + 2);

    if( stats.getNature() == Stats::MODEST || stats.getNature() == Stats::QUIET|| stats.getNature() == Stats::MILD || stats.getNature() == Stats::RASH ) nature_multiplier = 1.1;
    else if( stats.getNature() == Stats::ADAMANT || stats.getNature() == Stats::IMPISH || stats.getNature() == Stats::JOLLY || stats.getNature() == Stats::CAREFUL ) nature_multiplier = 0.9;
    else nature_multiplier = 1;

    total[Stats::SPATK] = ((2 * base[form][Stats::SPATK] + 31) * 50 / 100 + 5 + stats.getEV(Stats::SPATK)) * nature_multiplier;
    boosted[Stats::SPATK] = total[Stats::SPATK] * spatk_modifier_multiplier;

    //calculate spdef
    float spdef_modifier_multiplier;
    int16_t spdef_modifier = stats.getModifier(Stats::SPDEF);
    if( spdef_modifier == 0) spdef_modifier_multiplier = 1;
    else if(spdef_modifier > 0) spdef_modifier_multiplier = (float(spdef_modifier) + 2) / 2;
    else spdef_modifier_multiplier = 2 / (float(-spdef_modifier) + 2);

    if( stats.getNature() == Stats::CALM || stats.getNature() == Stats::GENTLE || stats.getNature() == Stats::SASSY || stats.getNature() == Stats::CAREFUL ) nature_multiplier = 1.1;
    else if( stats.getNature() == Stats::NAUGHTY || stats.getNature() == Stats::LAX || stats.getNature() == Stats::NAIVE || stats.getNature() == Stats::RASH ) nature_multiplier = 0.9;
    else nature_multiplier = 1;

    total[Stats::SPDEF] = ((2 * base[form][Stats::SPDEF] + 31) * 50 / 100 + 5 + stats.getEV(Stats::SPDEF)) * nature_multiplier;
    boosted[Stats::SPDEF] = total[Stats::SPDEF] * spdef_modifier_multiplier;

    //calculate spe
    float spe_modifier_multiplier;
    int16_t spe_modifier = stats.getModifier(Stats::SPE);
    if( spe_modifier == 0) spe_modifier_multiplier = 1;
    else if(spe_modifier > 0) spe_modifier_multiplier = (float(spe_modifier) + 2) / 2;
    else spe_modifier_multiplier = 2 / (float(-spe_modifier) + 2);

    if( stats.getNature() == Stats::TIMID || stats.getNature() == Stats::HASTY || stats.getNature() == Stats::JOLLY || stats.getNature() == Stats::NAIVE ) nature_multiplier = 1.1;
    else if( stats.getNature() == Stats::BRAVE || stats.getNature() == Stats::RELAXED || stats.getNature() == Stats::QUIET || stats.getNature() == Stats::SASSY ) nature_multiplier = 0.9;
    else nature_multiplier = 1;

    total[Stats::SPE] = ((2 * base[form][Stats::SPE] + 31) * 50 / 100 + 5 + stats.getEV(Stats::SPE)) * nature_multiplier;
    boosted[Stats::SPE] = total[Stats::SPE] * spe_modifier_multiplier;

    //evaluate if it is grounded
    if( types[form][0] == Type::Flying || types[form][1] == Type::Flying || getAbility() == Ability::Levitate ) grounded = false;
    else grounded = true;
}

float Pokemon::calculateWeatherModifier(const Move& theMove) const {
    if( theMove.getWeather() == Move::RAIN && getAbility() != Ability::Cloud_Nine && getAbility() != Ability::Air_Lock ) {
        if( theMove.getMoveType() == Water ) return 1.5;
        else if( theMove.getMoveType() == Fire ) return 0.5;
        else return 1;
    }

    else if( theMove.getWeather() == Move::SUN && getAbility() != Ability::Cloud_Nine && getAbility() != Ability::Air_Lock ) {
        // Hydro Steam is not weakened in sun; it is instead boosted to 1.5×
        if( theMove.getMoveIndex() == Moves::Hydro_Steam ) return 1.5;
        if( theMove.getMoveType() == Water ) return 0.5;
        else if( theMove.getMoveType() == Fire ) return 1.5;
        else return 1;
    }

    else if( theMove.getWeather() == Move::HARSH_SUNSHINE && getAbility() != Ability::Cloud_Nine && getAbility() != Ability::Air_Lock ) {
        if( theMove.getMoveIndex() == Moves::Hydro_Steam ) return 1.5;
        if( theMove.getMoveType() == Water ) return 0;
        else if( theMove.getMoveType() == Fire ) return 1.5;
        else return 1;
    }

    else if( theMove.getWeather() == Move::HEAVY_RAIN && getAbility() != Ability::Cloud_Nine && getAbility() != Ability::Air_Lock ) {
        if( theMove.getMoveType() == Water ) return 1.5;
        else if( theMove.getMoveType() == Fire ) return 0;
        else return 1;
    }

    else return 1;
}

float Pokemon::calculateTerrainModifier(const Pokemon& theAttacker, const Move& theMove) const {
    if( theAttacker.isGrounded() ) {
        if( theMove.getTerrain() == Move::Terrain::GRASSY && theMove.getMoveType() == Type::Grass ) return 1.5;
        else if( theMove.getTerrain() == Move::Terrain::PSYCHIC && theMove.getMoveType() == Type::Psychic ) return 1.5;
        else if( theMove.getTerrain() == Move::Terrain::ELECTRIC && theMove.getMoveType() == Type::Electric ) return 1.5;
        else if( theMove.getTerrain() == Move::Terrain::MISTY && theMove.getMoveType() == Type::Dragon ) return 0.5;
        else return 1;
    }

    else return 1;
}

float Pokemon::calculateStabModifier(const Pokemon& theAttacker, const Move& theMove) const {
    bool adaptability = (theAttacker.getAbility() == Ability::Adaptability);

    if( theAttacker.isTerastallized() ) {
        // Tera STAB rules (Gen 9):
        //   - STAB only applies if move type matches the Tera type.
        //   - If the Tera type was also one of the attacker's original types, the bonus
        //     is +0.5 (giving 2.0× without Adaptability, or still 2.0× with Adaptability).
        //   - Moves matching the original type but NOT the Tera type receive no STAB.
        if( theMove.getMoveType() == theAttacker.getTeraType() ) {
            bool teraMatchesOriginal =
                (theAttacker.getTeraType() == theAttacker.getTypes()[theAttacker.getForm()][0]) ||
                (theAttacker.getTeraType() == theAttacker.getTypes()[theAttacker.getForm()][1]);
            if( adaptability || teraMatchesOriginal ) return 2.0f;
            return 1.5f;
        }
        return 1.0f; // original types no longer grant STAB while terastallized
    }

    // Normal (non-tera) STAB
    float stab_modifier = adaptability ? 2.0f : 1.5f;
    if( (theAttacker.getTypes()[theAttacker.getForm()][0] == theMove.getMoveType()) || (theAttacker.getTypes()[theAttacker.getForm()][1] == theMove.getMoveType())) return stab_modifier;
    return 1.0f;
}

float Pokemon::calculateTargetModifier(const Move& theMove) const {
    if( theMove.getTarget() == Move::SINGLE || theMove.isZ() ) return 1;
    else return 0.75;
}

static bool isAlwaysCrit(const Move& theMove) {
    return theMove.getMoveIndex() == Moves::Surging_Strikes ||
           theMove.getMoveIndex() == Moves::Wicked_Blow;
}

float Pokemon::calculateCritModifier(const Move& theMove) const {
    if( theMove.isCrit() || isAlwaysCrit(theMove) ) return 1.5;
    else return 1;
}

float Pokemon::calculateBurnModifier(const Pokemon& theAttacker, const Move& theMove) const {
    if( theAttacker.getStatus() == Status::BURNED && theMove.getMoveCategory() == Move::PHYSICAL ) return 0.5;
    else return 1;
}

float Pokemon::calculateTypeModifier(const Pokemon& theAttacker, const Move& theMove) const {
    // Tera Shell (Gen 9): at full HP the first hit is always neutral effectiveness.
    // Turboblaze/Teravolt and ability-ignoring moves bypass this.
    if( getAbility() == Ability::Tera_Shell && getCurrentHPPercentage() == 100 &&
        theAttacker.getAbility() != Ability::Turboblaze && theAttacker.getAbility() != Ability::Teravolt &&
        theMove.getMoveIndex() != Moves::Moongeist_Beam && theMove.getMoveIndex() != Moves::Sunsteel_Strike &&
        theMove.getMoveIndex() != Moves::Menacing_Moonraze_Maelstrom && theMove.getMoveIndex() != Moves::Searing_Sunraze_Smash &&
        theMove.getMoveIndex() != Moves::Photon_Geyser && theMove.getMoveIndex() != Moves::Light_That_Burns_the_Sky ) {
        return 1.0f;
    }

    // When terastallized, the defender's type becomes its Tera type (monotype).
    Type eff_type1 = terastallized ? tera_type : getTypes()[form][0];
    Type eff_type2 = terastallized ? tera_type : getTypes()[form][1];

    //taking into account the strong winds if enabled
    if( theMove.getWeather() == Move::STRONG_WINDS && getAbility() != Ability::Cloud_Nine && getAbility() != Ability::Air_Lock ) {
        if( eff_type1 == eff_type2 ) {
            if( eff_type1 == Type::Flying && type_matrix[theMove.getMoveType()][Type::Flying] > 1 ) return 1;
            else return type_matrix[theMove.getMoveType()][eff_type1];
        }
        else {
            float part_1 = (eff_type1 == Type::Flying && type_matrix[theMove.getMoveType()][Type::Flying] > 1) ? 1.0f : type_matrix[theMove.getMoveType()][eff_type1];
            float part_2 = (eff_type2 == Type::Flying && type_matrix[theMove.getMoveType()][Type::Flying] > 1) ? 1.0f : type_matrix[theMove.getMoveType()][eff_type2];
            return part_1 * part_2;
        }
    }

    //in case it is scrappy
    else if( theAttacker.getAbility() == Ability::Scrappy ) {
        float part_1 = (eff_type1 == Type::Ghost && (theMove.getMoveType() == Type::Normal || theMove.getMoveType() == Type::Fighting)) ? 1.0f : type_matrix[theMove.getMoveType()][eff_type1];
        float part_2 = (eff_type2 == Type::Ghost && (theMove.getMoveType() == Type::Normal || theMove.getMoveType() == Type::Fighting)) ? 1.0f : type_matrix[theMove.getMoveType()][eff_type2];
        if( eff_type1 == eff_type2 ) return part_1;
        else return part_1 * part_2;
    }

    else {
        if( eff_type1 == eff_type2 ) return type_matrix[theMove.getMoveType()][eff_type1];
        else return type_matrix[theMove.getMoveType()][eff_type1] * type_matrix[theMove.getMoveType()][eff_type2];
    }
}

float Pokemon::calculateOtherModifier(const Pokemon& theAttacker, const Move& theMove) const {
    float modifier = 1;

    //accounting for the reducing berry
    if( (getItem().isReducingBerry() && getItem().getReducingBerryType() == theMove.getMoveType() && calculateTypeModifier(theAttacker, theMove) >= 2) || ( getItem().isReducingBerry() && getItem().getReducingBerryType() == Type::Normal && theMove.getMoveType() == Type::Normal ) ) modifier = modifier * 0.5;

    if( getAbility() == Ability::Shadow_Shield && getCurrentHPPercentage() == 100 ) modifier = modifier * 0.5;
    else if( getAbility() == Ability::Prism_Armor && calculateTypeModifier(theAttacker, theMove) >= 2 ) modifier = modifier * 0.75;
    else if( theAttacker.getAbility() == Ability::Neuroforce && calculateTypeModifier(theAttacker, theMove) >= 2 ) modifier = modifier * 1.25;

    // Gen 9 defensive abilities
    if( getAbility() == Ability::Purifying_Salt && theMove.getMoveType() == Type::Ghost ) modifier = modifier * 0.5;

    // Fluffy (Gen 7, was missing): halves contact moves, doubles Fire moves
    if( getAbility() == Ability::Fluffy ) {
        if( theMove.getMoveType() == Type::Fire ) modifier = modifier * 2.0f;
        // Contact moves: we approximate by checking the move category is Physical.
        // A full implementation would require a per-move contact flag in the binary.
        else if( theMove.getMoveCategory() == Move::PHYSICAL ) modifier = modifier * 0.5f;
    }

    // Water Bubble (Gen 7, was missing): 0.5× Fire damage received; handled offensively below
    if( getAbility() == Ability::Water_Bubble && theMove.getMoveType() == Type::Fire ) modifier = modifier * 0.5f;

    //these effects are ignored by solgaleo & lunala & necrozma peculiar moves & zekrom e reshiram abilities
    if( theMove.getMoveIndex() != Moves::Moongeist_Beam && theMove.getMoveIndex() != Moves::Sunsteel_Strike && theMove.getMoveIndex() != Moves::Menacing_Moonraze_Maelstrom && theMove.getMoveIndex() != Moves::Searing_Sunraze_Smash && theMove.getMoveIndex() != Moves::Photon_Geyser && theMove.getMoveIndex() != Moves::Light_That_Burns_the_Sky && theAttacker.getAbility() != Ability::Turboblaze && theAttacker.getAbility() != Ability::Teravolt) {
        if( getAbility() == Ability::Wonder_Guard && calculateTypeModifier(theAttacker, theMove) < 2 ) modifier = modifier * 0;
        else if( getAbility() == Ability::Multiscale && getCurrentHPPercentage() == 100 ) modifier = modifier * 0.5;
        else if( (getAbility() == Ability::Filter || getAbility() == Ability::Solid_Rock) && calculateTypeModifier(theAttacker, theMove) > 2  ) modifier = modifier * 0.75;
        else if( getAbility() == Ability::Levitate && theMove.getMoveType() == Type::Ground ) modifier = modifier * 0;
        else if( getAbility() == Ability::Heatproof && theMove.getMoveType() == Type::Fire ) modifier = modifier * 0.5;
        else if( getAbility() == Ability::Thick_Fat && (theMove.getMoveType() == Type::Fire || theMove.getMoveType() == Type::Ice) ) modifier = modifier * 0.5;
        else if( getAbility() == Ability::Flash_Fire && theMove.getMoveType() == Type::Fire ) modifier = modifier * 0;
        else if( getAbility() == Ability::Volt_Absorb && theMove.getMoveType() == Type::Electric ) modifier = modifier * 0;
        else if( (getAbility() == Ability::Water_Absorb || getAbility() == Ability::Storm_Drain) && theMove.getMoveType() == Type::Water ) modifier = modifier * 0;
    }

    // Tinted Lens (attacker): 2× damage on not-very-effective moves
    if( theAttacker.getAbility() == Ability::Tinted_Lens && calculateTypeModifier(theAttacker, theMove) < 1.0f ) modifier = modifier * 2.0f;

    // Offensive item/ability boosts
    if( theAttacker.getItem() == Items::Life_Orb ) modifier = modifier * 1.3;

    // Water Bubble (attacker): 2× Water-type moves
    if( theAttacker.getAbility() == Ability::Water_Bubble && theMove.getMoveType() == Type::Water ) modifier = modifier * 2.0f;

    // Orichalcum Pulse (Gen 9): +ATK boost under sun.
    // Koraidon sets sun on entry automatically, so the boost is active by default.
    // Only suppressed if weather is explicitly overridden by Rain, Heavy Rain, or Strong Winds.
    if( theAttacker.getAbility() == Ability::Orichalcum_Pulse &&
        theMove.getWeather() != Move::RAIN && theMove.getWeather() != Move::HEAVY_RAIN &&
        theMove.getWeather() != Move::STRONG_WINDS &&
        theMove.getMoveCategory() == Move::PHYSICAL ) modifier = modifier * 1.3f;

    // Hadron Engine (Gen 9): +SpATK boost under Electric Terrain.
    // Miraidon sets Electric Terrain on entry automatically, so the boost is active by default.
    // Only suppressed if terrain is explicitly overridden by Grassy, Psychic, or Misty Terrain.
    if( theAttacker.getAbility() == Ability::Hadron_Engine &&
        theMove.getTerrain() != Move::Terrain::GRASSY && theMove.getTerrain() != Move::Terrain::PSYCHIC &&
        theMove.getTerrain() != Move::Terrain::MISTY &&
        theMove.getMoveCategory() == Move::SPECIAL ) modifier = modifier * 1.3f;

    // Helping Hand: ×1.5 to the attacker's move damage
    if( theAttacker.helping_hand ) modifier = modifier * 1.5f;

    // Friend Guard (ally ability): ×0.75 to damage received by this Pokémon
    if( friend_guard ) modifier = modifier * 0.75f;

    // Collision Course / Electro Drift (Gen 9): 5461/4096 ≈ 1.333× bonus when super-effective
    if( (theMove.getMoveIndex() == Moves::Collision_Course || theMove.getMoveIndex() == Moves::Electro_Drift) &&
        calculateTypeModifier(theAttacker, theMove) >= 2 ) modifier = modifier * (5461.0f / 4096.0f);

    // Punching Glove (Gen 9): 1.1× for punching moves
    if( theAttacker.getItem() == Items::Punching_Glove ) {
        // Punching moves in the current roster:
        static const Moves PUNCHING_MOVES[] = {
            Moves::Bullet_Punch, Moves::Drain_Punch, Moves::DynamicPunch,
            Moves::Fire_Punch, Moves::Focus_Punch, Moves::Hammer_Arm, Moves::Ice_Punch,
            Moves::Mach_Punch, Moves::Meteor_Mash, Moves::Power_Up_Punch, Moves::Sky_Uppercut,
            Moves::ThunderPunch, Moves::Vacuum_Wave
        };
        for( auto m : PUNCHING_MOVES ) {
            if( theMove.getMoveIndex() == m ) { modifier = modifier * 1.1f; break; }
        }
    }

    return modifier;
}

uint16_t Pokemon::calculateDefenseInMove(const Move& theMove) const {
    uint16_t defense;

    if( theMove.isCrit() || isAlwaysCrit(theMove) ) {
        if( theMove.getMoveCategory() == Move::PHYSICAL || (theMove.getMoveIndex() == Moves::Psyshock && !theMove.isZ()) ) {
            if( (getModifier(Stats::DEF) > 0) || (theMove.getMoveIndex() == Moves::Darkest_Lariat && !theMove.isZ()) || (theMove.getMoveIndex() == Moves::Sacred_Sword && !theMove.isZ()) ) defense = getStat(Stats::DEF);
            else defense = getBoostedStat(Stats::DEF);

            if( getAbility() == Ability::Fur_Coat ) defense = defense * 2;
            if( getAbility() == Ability::Marvel_Scale && getStatus() != NO_STATUS ) defense = defense * 1.5;
            if( getItem() == Items::Eviolite ) defense = defense * 1.5;
            if( ruin_sword ) defense = defense * 0.75f;

        }

        else {
            if( getModifier(Stats::SPDEF) > 0 ) defense = getStat(Stats::SPDEF);
            else defense = getBoostedStat(Stats::SPDEF);

            if( getItem() == Items::Assault_Vest ) defense = defense * 1.5;
            if( getItem() == Items::Eviolite ) defense = defense * 1.5;
            if( ruin_beads ) defense = defense * 0.75f;
        }
    }

    else {
        if( theMove.getMoveCategory() == Move::PHYSICAL || (theMove.getMoveIndex() == Moves::Psyshock && !theMove.isZ()) ) {
            defense = getBoostedStat(Stats::DEF);

            if( getAbility() == Ability::Fur_Coat ) defense = defense * 2;
            if( getAbility() == Ability::Marvel_Scale && getStatus() != NO_STATUS ) defense = defense * 1.5;
            if( getItem() == Items::Eviolite ) defense = defense * 1.5;
            if( ruin_sword ) defense = defense * 0.75f;
        }

        else if( (theMove.getMoveIndex() == Moves::Darkest_Lariat && !theMove.isZ()) || (theMove.getMoveIndex() == Moves::Sacred_Sword && !theMove.isZ()) ) {
            defense = getStat(Stats::DEF);
            if( getAbility() == Ability::Fur_Coat ) defense = defense * 2;
            if( getAbility() == Ability::Marvel_Scale && getStatus() != NO_STATUS ) defense = defense * 1.5;
            if( getItem() == Items::Eviolite ) defense = defense * 1.5;
            if( ruin_sword ) defense = defense * 0.75f;
        }

        else {
            defense = getBoostedStat(Stats::SPDEF);

            if( getItem() == Items::Assault_Vest ) defense = defense * 1.5;
            if( getItem() == Items::Eviolite ) defense = defense * 1.5;
            if( ruin_beads ) defense = defense * 0.75f;
        }
    }

    return defense;
}

uint16_t Pokemon::calculateAttackInMove(const Pokemon& theAttacker, const Move& theMove) const {
    //this function is a fucking mess
    uint16_t attack;

    if( theMove.isCrit() || isAlwaysCrit(theMove) ) {
        //foul play (uses defender's ATK — Ruin Tablets does NOT apply here)
        if( theMove.getMoveIndex() == Moves::Foul_Play && !theMove.isZ() ) {
            if( getModifier(Stats::ATK) < 0 ) attack = getStat(Stats::ATK);
            else attack = getBoostedStat(Stats::ATK);

            if( theAttacker.getItem() == Items::Choice_Band ) attack = attack * 1.5;
            if( getAbility() == Ability::Huge_Power || getAbility() == Ability::Pure_Power ) attack = attack * 2;
        }

        //photon geyser & light that burns the sky
        else if( (theMove.getMoveIndex() == Moves::Photon_Geyser && !theMove.isZ()) || theMove.getMoveIndex() == Moves::Light_That_Burns_the_Sky ) {
            if( theAttacker.getBoostedStat(Stats::ATK) > theAttacker.getBoostedStat(Stats::SPATK) ) {
                if( theAttacker.getModifier(Stats::ATK) < 0 ) attack = theAttacker.getStat(Stats::ATK);
                else attack = theAttacker.getBoostedStat(Stats::ATK);

                if( theAttacker.getItem() == Items::Choice_Band ) attack = attack * 1.5;
                if( theAttacker.getAbility() == Ability::Huge_Power || theAttacker.getAbility() == Ability::Pure_Power ) attack = attack * 2;
                if( theAttacker.ruin_tablets ) attack = attack * 0.75f;
            }

            else {
                if( theAttacker.getModifier(Stats::SPATK) < 0 ) attack = theAttacker.getStat(Stats::SPATK);
                else attack = theAttacker.getBoostedStat(Stats::SPATK);

                if( theAttacker.getItem() == Items::Choice_Specs ) attack = attack * 1.5;
                if( theAttacker.ruin_vessel ) attack = attack * 0.75f;
            }
        }

        //usual phys
        else if( theMove.getMoveCategory() == Move::PHYSICAL ) {
            if( theAttacker.getModifier(Stats::ATK) < 0 ) attack = theAttacker.getStat(Stats::ATK);
            else attack = theAttacker.getBoostedStat(Stats::ATK);

            if( theAttacker.getItem() == Items::Choice_Band ) attack = attack * 1.5;
            if( theAttacker.getAbility() == Ability::Huge_Power || theAttacker.getAbility() == Ability::Pure_Power ) attack = attack * 2;
            if( theAttacker.ruin_tablets ) attack = attack * 0.75f;
        }

        //usual special
        else {
            if( theAttacker.getModifier(Stats::SPATK) < 0 ) attack = theAttacker.getStat(Stats::SPATK);
            else attack = theAttacker.getBoostedStat(Stats::SPATK);

            if( theAttacker.getItem() == Items::Choice_Specs ) attack = attack * 1.5;
            if( theAttacker.ruin_vessel ) attack = attack * 0.75f;
        }
    }

    else {
        //foul play (uses defender's ATK — Ruin Tablets does NOT apply here)
        if( theMove.getMoveIndex() == Moves::Foul_Play && !theMove.isZ() ) {
            attack = getBoostedStat(Stats::ATK);

            if( theAttacker.getItem() == Items::Choice_Band ) attack = attack * 1.5;
            if( getAbility() == Ability::Huge_Power || getAbility() == Ability::Pure_Power ) attack = attack * 2;
        }

        //photon geyser & light that burns the sky
        else if( (theMove.getMoveIndex() == Moves::Photon_Geyser && !theMove.isZ()) || theMove.getMoveIndex() == Moves::Light_That_Burns_the_Sky ) {
            if( theAttacker.getBoostedStat(Stats::ATK) > theAttacker.getBoostedStat(Stats::SPATK) ) {
                attack = theAttacker.getBoostedStat(Stats::ATK);

                if( theAttacker.getItem() == Items::Choice_Band ) attack = attack * 1.5;
                if( theAttacker.getAbility() == Ability::Huge_Power || theAttacker.getAbility() == Ability::Pure_Power ) attack = attack * 2;
                if( theAttacker.ruin_tablets ) attack = attack * 0.75f;
            }

            else {
                attack = theAttacker.getBoostedStat(Stats::SPATK);
                if( theAttacker.getItem() == Items::Choice_Specs ) attack = attack * 1.5;
                if( theAttacker.ruin_vessel ) attack = attack * 0.75f;
            }
        }

        //usual phys
        else if( theMove.getMoveCategory() == Move::PHYSICAL ) {
            attack = theAttacker.getBoostedStat(Stats::ATK);

            if( theAttacker.getItem() == Items::Choice_Band ) attack = attack * 1.5;
            if( theAttacker.getAbility() == Ability::Huge_Power || theAttacker.getAbility() == Ability::Pure_Power ) attack = attack * 2;
            if( theAttacker.ruin_tablets ) attack = attack * 0.75f;
        }

        //usual special
        else {
            attack = theAttacker.getBoostedStat(Stats::SPATK);
            if( theAttacker.getItem() == Items::Choice_Specs ) attack = attack * 1.5;
            if( theAttacker.ruin_vessel ) attack = attack * 0.75f;
        }
    }

    if( theAttacker.getAbility() == Ability::Blaze && theAttacker.getCurrentHPPercentage() <= float(100/3) && theMove.getMoveType() == Type::Fire ) attack = attack * 1.5;
    if( theAttacker.getAbility() == Ability::Overgrow && theAttacker.getCurrentHPPercentage() <= float(100/3) && theMove.getMoveType() == Type::Grass  ) attack = attack * 1.5;
    if( theAttacker.getAbility() == Ability::Torrent && theAttacker.getCurrentHPPercentage() <= float(100/3) && theMove.getMoveType() == Type::Water ) attack = attack * 1.5;
    if( theAttacker.getAbility() == Ability::Swarm && theAttacker.getCurrentHPPercentage() <= float(100/3) && theMove.getMoveType() == Type::Bug ) attack = attack * 1.5;
    if( theAttacker.getAbility() == Ability::Guts && theAttacker.getStatus() != NO_STATUS && theMove.getMoveCategory() == Move::PHYSICAL ) attack = attack * 1.5;
    if( theAttacker.getAbility() == Ability::Hustle && theMove.getMoveCategory() == Move::PHYSICAL ) attack = attack * 1.5;

    // Water Bubble (Gen 7): 2× offensive Water multiplier applied to the attack stat
    // (already handled in calculateOtherModifier; keep here for reference — do NOT double-apply)

    return attack;
}

// Helper: returns true for slicing moves (boosted by Sharpness in Gen 9).
// Source: official Gen 9 move flag data.
static bool isSlicingMove(const Moves moveIndex) {
    switch( moveIndex ) {
        case Moves::Aqua_Cutter:
        case Moves::Bitter_Blade:
        case Moves::Cross_Poison:
        case Moves::Dire_Claw:       // Champions: added slice flag
        case Moves::Dragon_Claw:     // Champions: added slice flag
        case Moves::Kowtow_Cleave:
        case Moves::Leaf_Blade:
        case Moves::Night_Slash:
        case Moves::Psycho_Cut:
        case Moves::Sacred_Sword:
        case Moves::Shadow_Claw:     // Champions: added slice flag
        case Moves::Solar_Blade:   // Solar Blade (slicing) — not SolarBeam (beam)
        case Moves::Tachyon_Cutter:
            return true;
        default:
            return false;
    }
}

void Pokemon::calculateMoveTypeInAttack(const Pokemon& theAttacker, Move& theMove) const {
    // Tera Blast: type becomes the attacker's Tera type when terastallized.
    // Category also changes: Physical if ATK > SpATK, otherwise Special.
    if( theMove.getMoveIndex() == Moves::Tera_Blast && theAttacker.isTerastallized() && theAttacker.getTeraType() != Type::Typeless ) {
        theMove.setMoveType(theAttacker.getTeraType());
        if( theAttacker.getBoostedStat(Stats::ATK) > theAttacker.getBoostedStat(Stats::SPATK) )
            theMove.setMoveCategory(Move::PHYSICAL);
        else
            theMove.setMoveCategory(Move::SPECIAL);
        return; // Tera Blast overrides *-ate abilities
    }

    //accounting for the *late ability move type change
    if( theAttacker.getAbility() == Ability::Aerilate && theMove.getMoveType() == Type::Normal && !theMove.isZ() ) theMove.setMoveType(Type::Flying);
    if( theAttacker.getAbility() == Ability::Pixilate && theMove.getMoveType() == Type::Normal && !theMove.isZ() ) theMove.setMoveType(Type::Fairy);
    if( theAttacker.getAbility() == Ability::Normalize && !theMove.isZ() ) theMove.setMoveType(Type::Normal);
    if( theAttacker.getAbility() == Ability::Refrigerate && theMove.getMoveType() == Type::Normal && !theMove.isZ() ) theMove.setMoveType(Type::Ice);
    if( theAttacker.getAbility() == Ability::Galvanize && theMove.getMoveType() == Type::Normal && !theMove.isZ() ) theMove.setMoveType(Type::Electric);
}

unsigned int Pokemon::calculateMoveBasePowerInAttack(const Pokemon& theAttacker, const Move& theMove) const {
    unsigned int bp;

    if( theMove.isZ() ) bp = theMove.getZBasePower();

    else bp = theMove.getBasePower();

    if( theAttacker.getAbility() == Ability::Technician && bp <= 60 ) bp = bp * 1.5;
    if( getAbility() == Ability::Dry_Skin && theMove.getMoveType() == Type::Fire ) bp = bp * 1.25;
    if( (getAbility() == Ability::Dark_Aura || theAttacker.getAbility() == Ability::Dark_Aura || theMove.isDarkAura()) && theMove.getMoveType() == Type::Dark ) bp = bp * 1.33;
    if( (getAbility() == Ability::Fairy_Aura || theAttacker.getAbility() == Ability::Fairy_Aura || theMove.isFairyAura()) && theMove.getMoveType() == Type::Fairy ) bp = bp * 1.33;
    if( theAttacker.getAbility() == Ability::Iron_Fist ) {
        static const Moves IRON_FIST_MOVES[] = {
            Moves::Bullet_Punch, Moves::Drain_Punch, Moves::DynamicPunch,
            Moves::Fire_Punch, Moves::Focus_Punch, Moves::Hammer_Arm, Moves::Ice_Punch,
            Moves::Mach_Punch, Moves::Meteor_Mash, Moves::Power_Up_Punch, Moves::Sky_Uppercut,
            Moves::ThunderPunch, Moves::Vacuum_Wave
        };
        for( auto m : IRON_FIST_MOVES ) {
            if( theMove.getMoveIndex() == m ) { bp = static_cast<unsigned int>(bp * 1.2f); break; }
        }
    }
    if( !theMove.isZ() && theMove.getMoveIndex() == Moves::Acrobatics && theAttacker.getItem() == Items::None ) bp = bp * 2;
    if( !theMove.isZ() && theMove.getMoveIndex() == Moves::Facade && (theAttacker.getStatus() == Status::BURNED || theAttacker.getStatus() == Status::POISONED || theAttacker.getStatus() == PARALYZED) ) bp = bp * 2;
    if( theMove.getMoveIndex() == Moves::Knock_Off && getItem().isRemovable() && !theMove.isZ() ) bp = bp * 1.5;
    if( !theMove.isZ() && theMove.getMoveIndex() == Moves::Brine && getCurrentHPPercentage() <= 50 ) bp = bp * 2;
    if( !theMove.isZ() && (theMove.getMoveIndex() == Moves::Water_Spout || theMove.getMoveIndex() == Moves::Eruption) ) {
        bp = (theAttacker.getCurrentHP() * 150) / theAttacker.getStat(Stats::HP);
        if( bp < 1 ) return 1;
        else return bp;
    }

    // Hard Press (Gen 9): BP = floor(100 * defender_current_HP / max_HP), minimum 1
    if( !theMove.isZ() && theMove.getMoveIndex() == Moves::Hard_Press ) {
        bp = static_cast<unsigned int>((getCurrentHP() * 100.0f) / getBoostedStat(Stats::HP));
        if( bp < 1 ) return 1;
        else return bp;
    }

    // Population Bomb (Gen 9): each hit is 20 BP; the binary stores 20 BP.
    // With Loaded Dice the move always hits 10 times (handled in turn logic outside this function).
    // Base calculation uses single-hit BP = 20 (already from binary).

    // Sharpness (Gen 9): 1.5× boost for slicing moves
    if( theAttacker.getAbility() == Ability::Sharpness && isSlicingMove(theMove.getMoveIndex()) && !theMove.isZ() ) bp = static_cast<unsigned int>(bp * 1.5f);

    //*late abilities boost
    if( theAttacker.getAbility() == Ability::Aerilate && theMove.getMoveType() == Type::Normal && !theMove.isZ() ) bp = bp * 1.2;
    if( theAttacker.getAbility() == Ability::Pixilate && theMove.getMoveType() == Type::Normal && !theMove.isZ() ) bp = bp * 1.2;
    if( theAttacker.getAbility() == Ability::Normalize && !theMove.isZ() ) bp = bp * 1.2;
    if( theAttacker.getAbility() == Ability::Refrigerate && theMove.getMoveType() == Type::Normal && !theMove.isZ() ) bp = bp * 1.2;
    if( theAttacker.getAbility() == Ability::Galvanize && theMove.getMoveType() == Type::Normal && !theMove.isZ() ) bp = bp * 1.2;
    if( theAttacker.getAbility() == Ability::Tough_Claws && theMove.getMoveCategory() == Move::PHYSICAL && !theMove.isZ() ) bp = static_cast<unsigned int>(bp * 1.3333f);

    return bp;
}

std::vector<int> Pokemon::getDamage(const Pokemon& theAttacker, Move theMove) const {
    // Flower Trick (Gen 9): always lands as a critical hit.
    if( theMove.getMoveIndex() == Moves::Flower_Trick ) theMove.setCrit(true);

    // Ruination (Gen 9): deals exactly half the defender's current HP as fixed damage.
    // Returns 16 identical rolls (no variance on fixed-damage moves).
    if( theMove.getMoveIndex() == Moves::Ruination ) {
        int fixed_damage = getCurrentHP() / 2;
        if( fixed_damage < 1 ) fixed_damage = 1;
        return std::vector<int>(16, fixed_damage);
    }

    // Tera Blast early pre-processing: set type and category BEFORE calculateAttackInMove
    // so the correct offensive stat is selected.
    if( theMove.getMoveIndex() == Moves::Tera_Blast && theAttacker.isTerastallized() && theAttacker.getTeraType() != Type::Typeless ) {
        theMove.setMoveType(theAttacker.getTeraType());
        if( theAttacker.getBoostedStat(Stats::ATK) > theAttacker.getBoostedStat(Stats::SPATK) )
            theMove.setMoveCategory(Move::PHYSICAL);
        else
            theMove.setMoveCategory(Move::SPECIAL);
    }

    uint16_t defense;
    uint16_t attack;

    attack = calculateAttackInMove(theAttacker, theMove);
    defense = calculateDefenseInMove(theMove);

    theMove.setBasePower(calculateMoveBasePowerInAttack(theAttacker, theMove));

    unsigned int base_damage = floor(floor((floor((2 * getLevel()) / 5 + 2) * theMove.getBasePower() * attack) / defense) / 50 + 2);
    if( base_damage == 0 ) base_damage = 1;

    //changing the move type before anything else if needed (taking into account the *late abilities for example)
    calculateMoveTypeInAttack(theAttacker, theMove);

    //calculating move modifiers
    theMove.setModifier().setWeatherModifier(calculateWeatherModifier(theMove)); //WEATHER
    theMove.setModifier().setTerrainModifier(calculateTerrainModifier(theAttacker, theMove)); //TERRAIN
    theMove.setModifier().setStabModifier(calculateStabModifier(theAttacker, theMove)); //STAB
    theMove.setModifier().setTargetModifier(calculateTargetModifier(theMove)); //TARGET
    theMove.setModifier().setCritModifier(calculateCritModifier(theMove)); //CRIT
    theMove.setModifier().setBurnModifier(calculateBurnModifier(theAttacker, theMove)); //BURN
    theMove.setModifier().setTypeModifier(calculateTypeModifier(theAttacker, theMove)); //TYPE
    theMove.setModifier().setOtherModifier(calculateOtherModifier(theAttacker, theMove)); //OTHER

    std::vector<int> buffer;
    for(int mod = 15; mod >= 0; mod-- ) {
        Modifier modifier = theMove.getModifier();
        modifier.setRandomModifier(std::floor((100-mod)/100));
        std::array<float, Modifier::MOD_NUM> modifiers = modifier.getModifiers();

        int damage = base_damage;
        for(unsigned int i = 0; i < modifiers.size(); i++) {
            const unsigned int RANDOM_MODIFIER_NUM = 5; //THIS PROBABLY SHOULDN'T BE HERE, IF NUMBER OF MODIFIER CHANGES THIS SHOULD CHANGE TOO, I'M SO LAZY
            if( i == RANDOM_MODIFIER_NUM ) damage = std::floor(damage * (100-mod) / 100);
            else damage = pokeRound(damage * modifiers[i]);
        }

        buffer.push_back(damage);
    }

    return buffer;
}

std::vector<float> Pokemon::getDamagePercentage(const Turn& theTurn) const {
    std::vector<int> damages = getDamageInt(theTurn);
    std::vector<float> buffer;
    for(unsigned int i = 0; i < damages.size(); i++) buffer.push_back(damages[i] / float(getCurrentHP()) * 100);

    return buffer;
}

void Pokemon::recursiveDamageCalculation(Pokemon theDefendingPokemon, std::vector<int>& theIntVector, std::vector<bool>& theBerryVector, std::vector<std::pair<Pokemon, Move>>& theVector, const unsigned int theHitNumber, std::vector<std::pair<Pokemon, Move>>::iterator& it) const {
    std::vector<std::pair<Pokemon, Move>> buffer_vector = theVector; //WE DO THIS BECAUSE LATER ON WE NEED TO MODIFY SOME OF THE TURN FOR DAMAGE CALCULATION PURPOSES AND IT'S BETTER TO MODIFY A COPY OF THE VECTOR
    std::vector<std::pair<Pokemon, Move>>::iterator buffer_it = it;

    if( it == theVector.begin() ) {
        theIntVector = theDefendingPokemon.getDamage(buffer_it->first, buffer_it->second);

        //this vector holds informations about consumed berries for each damage roll dealt
        for(auto it = theIntVector.begin(); it < theIntVector.end(); it++) theBerryVector.push_back(false);

    }

    else if( it == theVector.end() ) return;

    else {
        std::vector<int> buffer;
        std::vector<bool> new_berries;
        for(unsigned int j = 0; j < theIntVector.size(); j++) {
            int new_hp = theDefendingPokemon.getCurrentHP() - theIntVector[j];
            if( new_hp < 0 ) new_hp = 0;
            if( new_hp > theDefendingPokemon.getBoostedStat(Stats::HP) ) new_hp = theDefendingPokemon.getStat(Stats::HP);
            theDefendingPokemon.setCurrentHPPercentage((new_hp/theDefendingPokemon.getBoostedStat(Stats::HP))*100);
            auto new_damages = theDefendingPokemon.getDamage(it->first, it->second);

            for( auto it2 = new_damages.begin(); it2 < new_damages.end(); it2++ ) {
                buffer.push_back(*it2 + theIntVector[j]);
                new_berries.push_back(theBerryVector[j]);
            }
        }

        theIntVector = buffer;
        theBerryVector = new_berries;
    }

    //INFRA TURN MODIFIERS (these effects apply after each attack in a turn)
    if( buffer_it->second.isParentalBondMove() ) { //adding another move if it is a parental bond
        Pokemon parental_pokemon = buffer_it->first;
        Move parental_move = buffer_it->second;
        parental_move.setBasePower(parental_move.getBasePower()/4);
        parental_move.setParentalBondMove(false);
         it = theVector.insert(it+1, std::make_pair(parental_pokemon, parental_move));
         it--;
    }
    if( buffer_it->second.getMoveIndex() == Moves::Knock_Off && theDefendingPokemon.getItem().isRemovable() && !buffer_it->second.isZ() && !buffer_it->second.isParentalBondMove() ) theDefendingPokemon.setItem(Item(Items::None)); //setting the item as none after a knock off
    if( theDefendingPokemon.getItem().isReducingBerry() && theDefendingPokemon.getItem().getReducingBerryType() == buffer_it->second.getMoveType() && calculateTypeModifier(buffer_it->first, buffer_it->second) >= 2 ) theDefendingPokemon.setItem(Item(Items::None)); //setting the item as none if a reducing berry is consumed

    //taking the restoring berries into account
    if( theDefendingPokemon.getItem().isRestoringBerry() ) {
        for(unsigned int i = 0; i < theIntVector.size(); i++) {
            int hp_remaining = theDefendingPokemon.getBoostedStat(Stats::HP) - theIntVector[i];
            float hp_percentage_remaining = ((float(hp_remaining) / float(theDefendingPokemon.getBoostedStat(Stats::HP))) * 100);
            if( hp_percentage_remaining <= theDefendingPokemon.getItem().getRestoringActivation() && hp_percentage_remaining > 0 && theBerryVector[i] == false ) { //if this is true we restore some hps using berries
                theBerryVector[i] = true;
                int damage_restoring = (theDefendingPokemon.getBoostedStat(Stats::HP) / 100) * theDefendingPokemon.getItem().getRestoringPercentage();
                theIntVector[i] = theIntVector[i] - damage_restoring;
            }
        }
    }


    //END OF TURN MODIFIERS (these effects apply at the end of the turn
    //if the pokemon is not dead and some effects are in place we modify the damages
    if( std::distance(theVector.begin(), it) % theHitNumber == 0 ) {
        for( auto it_last = theIntVector.begin(); it_last < theIntVector.end(); it_last++ ) {
            if( *it_last < theDefendingPokemon.getBoostedStat(Stats::HP) ) {

                //RESTORING FOR GRASSY TERRAIN
                //Note: recovery is shown as a text note only; not subtracted from displayed damage (matching Showdown format)

                //SOON SOME MORE
            }
        }
    }

    it++;
    recursiveDamageCalculation(theDefendingPokemon, theIntVector, theBerryVector, theVector, theHitNumber, it);
}

std::vector<int> Pokemon::getDamageInt(const Turn& theTurn) const {
    std::vector<std::pair<Pokemon, Move>> buffer = theTurn.getMovesEffective();
    std::vector<std::pair<Pokemon, Move>>::iterator it = buffer.begin();
    std::vector<int> vec;
    std::vector<bool> berry_vec;

    recursiveDamageCalculation(*this, vec, berry_vec, buffer, theTurn.getHits(), it);
    return vec;
}

float Pokemon::getKOProbability(const Turn& theTurn) const {
    std::vector<float> damages = getDamagePercentage(theTurn);

    unsigned int ko_count = 0;
    for(auto it = damages.begin(); it < damages.end(); it++) if( *it >= 100 ) ko_count++;

    return (float(ko_count) / damages.size()) * 100;
}

int Pokemon::outspeedPokemon(const std::vector<Pokemon>& theVector) {
    // Champions: each stat has 0-32 SPs (Stat Points), no shared pool
    const unsigned int MAX_EVS = 66;
    const unsigned int MAX_EVS_SINGLE_STAT = 32;

    Pokemon defender = *this;

    unsigned int assignable_evs = MAX_EVS - (defender.getEV(Stats::ATK) + defender.getEV(Stats::SPATK) + defender.getEV(Stats::DEF) + defender.getEV(Stats::SPDEF) + defender.getEV(Stats::HP));
    defender.setAllEV(0, 0, 0, 0, 0, 0);

    std::vector<uint16_t> speed_stats;
    for(auto it = theVector.begin(); it < theVector.end(); it++) speed_stats.push_back(it->getBoostedStat(Stats::SPE));

    uint16_t speed_max = *std::max_element(speed_stats.begin(), speed_stats.end());

    for(unsigned int i = 0; i < MAX_EVS_SINGLE_STAT; i++) {
        defender.setEV(Stats::SPE, i);
        if( defender.getBoostedStat(Stats::SPE) > speed_max && i <= assignable_evs ) return i;
    }

    return -1;
}

DefenseResult Pokemon::resistMove(const std::vector<Turn>& theTurn, const std::vector<defense_modifier>& theDefModifiers) {
    //this would'nt be a valid calculation
    if( theTurn.size() != theDefModifiers.size() ) {
        DefenseResult temp;
        temp.hp_ev.push_back(-2);
        temp.def_ev.push_back(-2);
        temp.spdef_ev.push_back(-2);

        return temp;
    }

    //if there is no attack in the vector
    if( theTurn.empty() ) {
        DefenseResult temp;
        temp.hp_ev.push_back(-3);
        temp.def_ev.push_back(-3);
        temp.spdef_ev.push_back(-3);

        return temp;
    }

    //calculating if we could use the faster loop
    Turn::Type previous = theTurn.begin()->getType();
    bool simplified = true;
    for( auto it = theTurn.begin(); it < theTurn.end(); it++ ) {
        if( it->getType() == Turn::MIXED || it->getType() != previous ) simplified = false;
        previous = it->getType();
    }

    Move::Category simplified_type = Move::SPECIAL; //just random assignment
    if( simplified ) {
        if( previous == Turn::PHYSICAL ) simplified_type = Move::PHYSICAL;
        else simplified_type = Move::SPECIAL;
    }

    auto results = resistMoveLoop(theTurn, theDefModifiers, simplified, simplified_type);
    //if no result has been found
    if( results.first.empty() && !abort_calculation ) { //checking any of the two would be good
        DefenseResult temp;
        temp.hp_ev.push_back(-1);
        temp.def_ev.push_back(-1);
        temp.spdef_ev.push_back(-1);

        return temp;
    }

    //if an abort has been requested
    else if( abort_calculation ) {
        DefenseResult temp;
        temp.hp_ev.push_back(-4);
        temp.def_ev.push_back(-4);
        temp.spdef_ev.push_back(-4);

        return temp;
    }

    DefenseResult returnable;

    for(unsigned int i = 0; i < RESULT_TYPE_NUM; i++) {
        const float WEIGHT = 2; //this is the weight used when calculating spreads more focused on hp (the bigger is weight the more they are focused on hp)
        float effective_weight;
        if( i == 0 ) effective_weight = 1;
        else effective_weight = WEIGHT * i;

        //finding all the minimum elements
        std::vector<unsigned int> sum_results;
        //creating the vector with the sums

        //different vectors for different euristhics
        std::vector<std::tuple<uint8_t, uint8_t, uint8_t>> temp_vec;
        if( i == 0 ) temp_vec = results.first;
        else if ( i == 1 ) temp_vec = results.second;

        for( auto it = temp_vec.begin(); it < temp_vec.end(); it++ ) sum_results.push_back(std::get<0>(*it) + std::get<1>(*it) * effective_weight + std::get<2>(*it) * effective_weight );
        //finding the minimum sums
        auto min_index = std::min_element(sum_results.begin(), sum_results.end());
        std::vector<std::vector<unsigned int>::iterator> final_results;
        //all of them
        for(auto it = sum_results.begin(); it < sum_results.end(); it++) if( *it == *min_index ) final_results.push_back(it);

        //finding the one with the max hp
        std::vector<std::tuple<uint8_t, uint8_t, uint8_t>> returnable_results;
        for(auto it = final_results.begin(); it < final_results.end(); it++) returnable_results.push_back(temp_vec[std::distance(sum_results.begin(), *it)]);

        std::tuple<uint8_t, uint8_t, uint8_t> final_pair = returnable_results[0];
        for(auto it = returnable_results.begin(); it < returnable_results.end(); it++)
            if( std::get<0>(*it) >= std::get<0>(final_pair) ) final_pair = *it;

        if( (i == 0) || (returnable.hp_ev[i-1] != std::get<0>(final_pair) || returnable.def_ev[i-1] != std::get<1>(final_pair) || returnable.spdef_ev[i-1] != std::get<2>(final_pair)  ) ) {
            returnable.hp_ev.push_back(std::get<0>(final_pair));
            returnable.def_ev.push_back(std::get<1>(final_pair));
            returnable.spdef_ev.push_back(std::get<2>(final_pair));
        }

        //if( !simplified ) break; //if we are not in a simplified loop (meaning we are calculating for attacks on different sides) there is really no need to move on
    }

    //now calculating all the damages
    for( unsigned int i = 0; i < returnable.hp_ev.size(); i++ ) { //we use hp.ev.size but we could really use any of the 3
        Pokemon buffer = *this;
        buffer.setEV(Stats::HP, returnable.hp_ev[i]);
        buffer.setEV(Stats::DEF, returnable.def_ev[i]);
        buffer.setEV(Stats::SPDEF, returnable.spdef_ev[i]);

        returnable.def_ko_prob.push_back(std::vector<float>());
        returnable.def_damage_perc.push_back(std::vector<std::vector<float>>());
        returnable.def_damage_int.push_back(std::vector<std::vector<int>>());
        for( auto it = 0; it < theTurn.size(); it++ ) {
            buffer.setModifier(Stats::HP, std::get<0>(theDefModifiers[it]));
            buffer.setModifier(Stats::DEF, std::get<1>(theDefModifiers[it]));
            buffer.setModifier(Stats::SPDEF, std::get<2>(theDefModifiers[it]));
            buffer.setTeraType(std::get<3>(theDefModifiers[it]));
            buffer.setTerastallized(std::get<4>(theDefModifiers[it]));
            buffer.setRuinSword(std::get<5>(theDefModifiers[it]));
            buffer.setRuinBeads(std::get<6>(theDefModifiers[it]));
            buffer.setFriendGuard(std::get<10>(theDefModifiers[it]));

            // Apply attacker-side modifiers (tablets/vessel/helping_hand) for accurate display
            Turn display_turn;
            for(auto& mp : theTurn[it].getMoves()) {
                Pokemon atk_copy = mp.first;
                atk_copy.setRuinTablets(std::get<7>(theDefModifiers[it]));
                atk_copy.setRuinVessel(std::get<8>(theDefModifiers[it]));
                atk_copy.setHelpingHand(std::get<9>(theDefModifiers[it]));
                display_turn.addMove(atk_copy, mp.second);
            }
            display_turn.setHits(theTurn[it].getHits());

            returnable.def_ko_prob[i].push_back(buffer.getKOProbability(display_turn));
            returnable.def_damage_perc[i].push_back(buffer.getDamagePercentage(display_turn));
            returnable.def_damage_int[i].push_back(buffer.getDamageInt(display_turn));
        }
    }

    return returnable;
}

std::pair<std::vector<std::tuple<uint8_t, uint8_t, uint8_t>>, std::vector<std::tuple<uint8_t, uint8_t, uint8_t>>> Pokemon::resistMoveLoop(const std::vector<Turn>& theTurn, const std::vector<defense_modifier>& theDefModifiers, const bool isSimplified, const Move::Category simplifiedType) {
    // Champions: each stat has 0-32 SPs (Stat Points), no shared pool
    const unsigned int MAX_EVS = 66;
    const unsigned int MAX_EVS_SINGLE_STAT = 32;
    const unsigned int ARRAY_SIZE = MAX_EVS_SINGLE_STAT + 1;

    unsigned int THREAD_NUM = std::thread::hardware_concurrency();
    if( THREAD_NUM == 0 ) THREAD_NUM = 1; //otherwise single thread machines would bug out
    std::vector<std::thread*> threads;

    Pokemon defender = *this;

    unsigned int assignable_evs = MAX_EVS - (defender.getEV(Stats::ATK) + defender.getEV(Stats::SPATK) + defender.getEV(Stats::SPE));
    defender.setAllEV(0, 0, 0, 0, 0, 0);

    std::vector<std::tuple<uint8_t, uint8_t, uint8_t>> results;
    std::vector<std::tuple<uint8_t, uint8_t, uint8_t>> alternative_results; //this one contains all the results that are suitable for the second euristhics but not for the first one

    std::vector<std::vector<float>> results_buffer;
    results_buffer.resize(theTurn.size());
    for(auto it = results_buffer.begin(); it < results_buffer.end(); it++) it->resize(ARRAY_SIZE*ARRAY_SIZE*ARRAY_SIZE);

    for(unsigned int hp_assigned = getEV(Stats::HP); hp_assigned < MAX_EVS_SINGLE_STAT + 1; hp_assigned = hp_assigned + calculateEVSNextStat(defender, Stats::HP, hp_assigned)) {
        defender.setEV(Stats::HP, hp_assigned);

        for(unsigned int spdef_assigned = getEV(Stats::SPDEF); spdef_assigned < MAX_EVS_SINGLE_STAT + 1; spdef_assigned = spdef_assigned + calculateEVSNextStat(defender, Stats::SPDEF, spdef_assigned)) {
            if( isSimplified && simplifiedType == Move::PHYSICAL && spdef_assigned > getEV(Stats::SPDEF) ) break;
                defender.setEV(Stats::SPDEF, spdef_assigned);

                for(unsigned int def_assigned = getEV(Stats::DEF); def_assigned < MAX_EVS_SINGLE_STAT + 1; def_assigned = def_assigned + calculateEVSNextStat(defender, Stats::DEF, def_assigned)) {
                    //qDebug() << QString::number(hp_assigned) + " " + QString::number(spdef_assigned) + " " + QString::number(def_assigned);
                    if( isSimplified && simplifiedType == Move::SPECIAL && def_assigned > getEV(Stats::DEF) ) break;
                    //if an abort has been requested we return an empty result
                    if( abort_calculation ) return std::make_pair(std::vector<std::tuple<uint8_t, uint8_t, uint8_t>>(), std::vector<std::tuple<uint8_t, uint8_t, uint8_t>>());

                    defender.setEV(Stats::DEF, def_assigned);

                    //if we are on a single-core architecture launching it in a separate thread just creates an useless overhead
                    if( THREAD_NUM < 2 ) resistMoveLoopThread(defender, theTurn, results, theDefModifiers, results_buffer, assignable_evs);

                    else {
                        threads.push_back(new std::thread(&Pokemon::resistMoveLoopThread, this, defender, std::cref(theTurn), std::ref(results), std::cref(theDefModifiers), std::ref(results_buffer), assignable_evs));
                        //if we spawned too much thread
                        if( threads.size() >= THREAD_NUM ) {
                            for( auto it = threads.begin(); it < threads.end(); it++ ) { (*it)->join(); delete *it; }
                            threads.clear();
                        }
                    }
                }
            }
        }

    //just in case some thread hasn't finished yet
    for( auto it = threads.begin(); it < threads.end(); it++ ) { (*it)->join(); delete *it; }
    threads.clear();

    //if an abort has been requested between the loop and the join operation we return an empty result
    if( abort_calculation ) return std::make_pair(std::vector<std::tuple<uint8_t, uint8_t, uint8_t>>(), std::vector<std::tuple<uint8_t, uint8_t, uint8_t>>());

    //if no result is found we search some rolls
    bool roll_found = false;
    unsigned int roll_count = 0;
    unsigned int roll_count_min = 0;
    const unsigned int MAX_ROLL = 70;
    const unsigned int MIN_ROLL = 20;
    const unsigned int ROLL_OFFSET = 1;
    std::vector<float> tolerances;
    tolerances.resize(theTurn.size(), 0);

    if( results.empty() ) {
        while( (!roll_found || roll_count_min < (MIN_ROLL+1) * theTurn.size()) && roll_count < (MAX_ROLL+1) * theTurn.size() && !abort_calculation/*((MAX_ROLL+1)*(pow(MAX_ROLL+1, theTurn.size()-1))) used for the alternative version of the algorithm*/ ) {
            //for( auto it = tolerances.begin(); it < tolerances.end(); it++ ) qDebug() << *it;

            unsigned int tolerance_index = roll_count / (MAX_ROLL+1);
            (*(tolerances.end()-1-tolerance_index)) += ROLL_OFFSET;

            /*these are two different algorithm that produce different result i like the first one more so i'm keeping it for now
            bool to_increment = true;
            for( auto it = tolerances.begin(); it < tolerances.end(); it++ ) {
                if( *it == MAX_ROLL ) {
                    bool to_increment_local = true;
                    for( auto it2 = it+1; it2 < tolerances.end(); it2++ ) if( *it2 != MAX_ROLL ) to_increment_local = false;
                    if( to_increment_local && (*(tolerances.begin())) != MAX_ROLL ) {
                        (*(it-1)) += ROLL_OFFSET;
                        for(auto it3 = it; it3 < tolerances.end(); it3++) *it3 = 0;
                        to_increment = false;
                    }
                }
            }

            if( to_increment ) tolerances.back()++;
            */

            for(unsigned int hp_assigned = getEV(Stats::HP); hp_assigned < MAX_EVS_SINGLE_STAT + 1; hp_assigned = hp_assigned + calculateEVSNextStat(defender, Stats::HP, hp_assigned)) {

                for(unsigned int spdef_assigned = getEV(Stats::SPDEF); spdef_assigned < MAX_EVS_SINGLE_STAT + 1; spdef_assigned = spdef_assigned + calculateEVSNextStat(defender, Stats::SPDEF, spdef_assigned)) {
                    if( isSimplified && simplifiedType == Move::PHYSICAL && spdef_assigned > getEV(Stats::SPDEF) ) break;

                    for(unsigned int def_assigned = getEV(Stats::DEF); def_assigned < MAX_EVS_SINGLE_STAT + 1; def_assigned = def_assigned + calculateEVSNextStat(defender, Stats::DEF, def_assigned)) {
                        //qDebug() << QString::number(roll_count) + " " + QString::number(hp_assigned) + " " + QString::number(spdef_assigned) + " " + QString::number(def_assigned);
                        //if an abort request is made
                        if( abort_calculation ) return std::make_pair(std::vector<std::tuple<uint8_t, uint8_t, uint8_t>>(), std::vector<std::tuple<uint8_t, uint8_t, uint8_t>>());
                        if( isSimplified && simplifiedType == Move::SPECIAL && def_assigned > getEV(Stats::DEF) ) break;

                        bool to_add = true;
                        for(unsigned int it = 0; it < theTurn.size() && to_add; it++ ) {
                            defender.setCurrentHPPercentage(std::get<0>(theDefModifiers[it]));
                            defender.setModifier(Stats::DEF, std::get<1>(theDefModifiers[it]));
                            defender.setModifier(Stats::SPDEF, std::get<2>(theDefModifiers[it]));
                            defender.setTeraType(std::get<3>(theDefModifiers[it]));
                            defender.setTerastallized(std::get<4>(theDefModifiers[it]));
                            defender.setRuinSword(std::get<5>(theDefModifiers[it]));
                            defender.setRuinBeads(std::get<6>(theDefModifiers[it]));
                            if( hp_assigned + def_assigned + spdef_assigned > assignable_evs ) to_add = false;
                            else if( results_buffer[it][hp_assigned +  def_assigned * ARRAY_SIZE + spdef_assigned * ARRAY_SIZE * ARRAY_SIZE] > (0 + tolerances[it]) ) to_add = false;
                        }

                        if( to_add ) {
                            if( roll_found && roll_count_min > 0 ) alternative_results.push_back(std::make_tuple(hp_assigned, def_assigned, spdef_assigned));
                            else results.push_back(std::make_tuple(hp_assigned, def_assigned, spdef_assigned));
                            roll_found = true;
                        }
                    }
                }

            }

            if( roll_found ) roll_count_min++;
            roll_count++;
        }
    }

    //merging the two vectors
    alternative_results.insert(alternative_results.end(), results.begin(), results.end());
    return std::make_pair(results, alternative_results);
}

void Pokemon::resistMoveLoopThread(Pokemon theDefender, const std::vector<Turn>& theTurn, std::vector<std::tuple<uint8_t, uint8_t, uint8_t>>& theResult, const std::vector<defense_modifier>& theDefModifiers, std::vector<std::vector<float>>& theResultBuffer, const unsigned int theAssignableEVS) {
    const unsigned int MAX_EVS_SINGLE_STAT = 32;
    const unsigned int ARRAY_SIZE = MAX_EVS_SINGLE_STAT + 1;

    bool to_add = true;
    for(unsigned int it = 0; it < theTurn.size(); it++ ) {
        theDefender.setCurrentHPPercentage(std::get<0>(theDefModifiers[it]));
        theDefender.setModifier(Stats::DEF, std::get<1>(theDefModifiers[it]));
        theDefender.setModifier(Stats::SPDEF, std::get<2>(theDefModifiers[it]));
        theDefender.setTeraType(std::get<3>(theDefModifiers[it]));
        theDefender.setTerastallized(std::get<4>(theDefModifiers[it]));
        theDefender.setRuinSword(std::get<5>(theDefModifiers[it]));
        theDefender.setRuinBeads(std::get<6>(theDefModifiers[it]));
        theDefender.setFriendGuard(std::get<10>(theDefModifiers[it]));

        // Apply attacker-side modifiers (tablets/vessel/helping_hand) to attackers in a copy of the turn
        Turn modified_turn;
        for(auto& mp : theTurn[it].getMoves()) {
            Pokemon atk_copy = mp.first;
            atk_copy.setRuinTablets(std::get<7>(theDefModifiers[it]));
            atk_copy.setRuinVessel(std::get<8>(theDefModifiers[it]));
            atk_copy.setHelpingHand(std::get<9>(theDefModifiers[it]));
            modified_turn.addMove(atk_copy, mp.second);
        }
        modified_turn.setHits(theTurn[it].getHits());

        float ko_prob;
        if( theDefender.getEV(Stats::HP) + theDefender.getEV(Stats::DEF) + theDefender.getEV(Stats::SPDEF) > theAssignableEVS ) to_add = false;
        else if( (ko_prob = theDefender.getKOProbability(modified_turn)) > 0 ) {
            buffer_mutex.lock();
            theResultBuffer[it][theDefender.getEV(Stats::HP) + theDefender.getEV(Stats::DEF) * ARRAY_SIZE + theDefender.getEV(Stats::SPDEF) * ARRAY_SIZE * ARRAY_SIZE] = ko_prob;
            buffer_mutex.unlock();
            to_add = false;
        }

        else {
            buffer_mutex.lock();
            theResultBuffer[it][theDefender.getEV(Stats::HP) + theDefender.getEV(Stats::DEF) * ARRAY_SIZE + theDefender.getEV(Stats::SPDEF) * ARRAY_SIZE * ARRAY_SIZE] = ko_prob;
            buffer_mutex.unlock();
        }
    }

    if( to_add ) {
        result_mutex.lock();
        theResult.push_back(std::make_tuple(theDefender.getEV(Stats::HP), theDefender.getEV(Stats::DEF), theDefender.getEV(Stats::SPDEF)));
        result_mutex.unlock();
    }
}

int Pokemon::pokeRound(const float theValue) const {
    if( fmod(theValue, 1) > 0.5 ) return ceil(theValue);
    else return floor(theValue);
}

AttackResult Pokemon::koMove(const std::vector<Turn>& theTurn, const std::vector<Pokemon>& theDefendingPokemon, const std::vector<attack_modifier>& theAtkModifier) {
    //this would'nt be a valid calculation
    if( theTurn.size() != theAtkModifier.size() || theTurn.size() != theDefendingPokemon.size() ) {
        AttackResult temp;
        temp.atk_ev.push_back(-2);
        temp.spatk_ev.push_back(-2);

        return temp;
    }

    //if there is no attack in the vector
    if( theTurn.empty() ) {
        AttackResult temp;
        temp.atk_ev.push_back(-3);
        temp.spatk_ev.push_back(-3);

        return temp;
    }

    //algorithm here
    // Champions: each stat has 0-32 SPs (Stat Points), no shared pool
    const unsigned int MAX_EVS = 66;
    const unsigned int MAX_EVS_SINGLE_STAT = 32;
    const unsigned int ARRAY_SIZE = MAX_EVS_SINGLE_STAT + 1;

    Pokemon attacker = *this;

    //calculating if we could use the faster loop
    Turn::Type previous = theTurn.begin()->getType();
    bool simplified = true;
    for( auto it = theTurn.begin(); it < theTurn.end(); it++ ) {
        if( it->getType() == Turn::MIXED || it->getType() != previous ) simplified = false;
        previous = it->getType();
    }

    Move::Category simplified_type = Move::SPECIAL; //just random assignment
    if( simplified ) {
        if( previous == Turn::PHYSICAL ) simplified_type = Move::PHYSICAL;
        else simplified_type = Move::SPECIAL;
    }

    unsigned int assignable_evs = MAX_EVS - (attacker.getEV(Stats::HP) + attacker.getEV(Stats::DEF) + attacker.getEV(Stats::SPDEF) + attacker.getEV(Stats::SPE));
    attacker.setAllEV(0, 0, 0, 0, 0, 0);

    std::vector<std::pair<int, int>> results;

    std::vector<std::vector<float>> results_buffer;
    results_buffer.resize(theTurn.size());
    for(auto it = results_buffer.begin(); it < results_buffer.end(); it++) it->resize(ARRAY_SIZE*ARRAY_SIZE);

    for(unsigned int spatk_assigned = getEV(Stats::SPATK); spatk_assigned < MAX_EVS_SINGLE_STAT + 1; spatk_assigned = spatk_assigned + calculateEVSNextStat(attacker, Stats::SPATK, spatk_assigned)) {
        if( simplified && simplified_type == Move::PHYSICAL && spatk_assigned > getEV(Stats::SPATK) ) break;
        attacker.setEV(Stats::SPATK, spatk_assigned);

        for(unsigned int atk_assigned = getEV(Stats::ATK); atk_assigned < MAX_EVS_SINGLE_STAT + 1; atk_assigned = atk_assigned + calculateEVSNextStat(attacker, Stats::ATK, atk_assigned)) {
            if( simplified && simplified_type == Move::SPECIAL && atk_assigned > getEV(Stats::ATK) ) break;
            //if an abort has been requested we return an empty result
            if( abort_calculation ) { AttackResult temp; temp.atk_ev.push_back(-4); temp.spatk_ev.push_back(-4); return temp; }

            attacker.setEV(Stats::ATK, atk_assigned);

            const unsigned int ARRAY_SIZE = MAX_EVS_SINGLE_STAT + 1;

            bool to_add = true;
            for(unsigned int it = 0; it < theTurn.size(); it++ ) {
                attacker.setModifier(Stats::ATK, std::get<0>(theAtkModifier[it]));
                attacker.setModifier(Stats::SPATK, std::get<1>(theAtkModifier[it]));
                attacker.setTeraType(std::get<2>(theAtkModifier[it]));
                attacker.setTerastallized(std::get<3>(theAtkModifier[it]));
                attacker.setRuinTablets(std::get<4>(theAtkModifier[it]));
                attacker.setRuinVessel(std::get<5>(theAtkModifier[it]));
                attacker.setHelpingHand(std::get<8>(theAtkModifier[it]));

                // Ruin sword/beads reduce the defender's DEF/SpDef — apply on a local copy
                Pokemon def_copy = theDefendingPokemon[it];
                def_copy.setRuinSword(std::get<6>(theAtkModifier[it]));
                def_copy.setRuinBeads(std::get<7>(theAtkModifier[it]));
                def_copy.setFriendGuard(std::get<9>(theAtkModifier[it]));

                //reversing because of the offensive nature of the calc
                Turn temp_turn;
                temp_turn.addMove(attacker, theTurn[it].getMoves()[0].second);
                temp_turn.setHits(theTurn[it].getHits());

                float ko_prob;
                if( attacker.getEV(Stats::ATK) + attacker.getEV(Stats::SPATK) > assignable_evs ) to_add = false;
                else if( (ko_prob = def_copy.getKOProbability(temp_turn)) < 100 ) { results_buffer[it][attacker.getEV(Stats::ATK) + attacker.getEV(Stats::SPATK) * ARRAY_SIZE] = ko_prob; to_add = false; }

                else results_buffer[it][attacker.getEV(Stats::ATK) + attacker.getEV(Stats::SPATK) * ARRAY_SIZE] = ko_prob;
            }

            if( to_add ) results.push_back(std::make_pair(attacker.getEV(Stats::ATK), attacker.getEV(Stats::SPATK)));
        }
    }

    //if no result is found we search some rolls
    unsigned int roll_count = 0;
    const unsigned int MAX_ROLL = 70;
    const unsigned int ROLL_OFFSET = 1;
    std::vector<float> tolerances;
    tolerances.resize(theTurn.size(), 0);

    while( results.empty() && roll_count < (MAX_ROLL+1) * theTurn.size() && !abort_calculation/*((MAX_ROLL+1)*(pow(MAX_ROLL+1, theTurn.size()-1))) used for the alternative version of the algorithm*/ ) {
        //for( auto it = tolerances.begin(); it < tolerances.end(); it++ ) qDebug() << *it;

        unsigned int tolerance_index = roll_count / (MAX_ROLL+1);
        (*(tolerances.end()-1-tolerance_index)) += ROLL_OFFSET;

        /*these are two different algorithm that produce different result i like the first one more so i'm keeping it for now
        bool to_increment = true;
        for( auto it = tolerances.begin(); it < tolerances.end(); it++ ) {
            if( *it == MAX_ROLL ) {
                bool to_increment_local = true;
                for( auto it2 = it+1; it2 < tolerances.end(); it2++ ) if( *it2 != MAX_ROLL ) to_increment_local = false;
                if( to_increment_local && (*(tolerances.begin())) != MAX_ROLL ) {
                    (*(it-1)) += ROLL_OFFSET;
                    for(auto it3 = it; it3 < tolerances.end(); it3++) *it3 = 0;
                    to_increment = false;
                }
            }
        }

        if( to_increment ) tolerances.back()++;
        */

        for(unsigned int spatk_assigned = 0; spatk_assigned < MAX_EVS_SINGLE_STAT + 1; spatk_assigned = spatk_assigned + calculateEVSNextStat(attacker, Stats::SPATK, spatk_assigned)) {
            if( simplified && simplified_type == Move::PHYSICAL && spatk_assigned > getEV(Stats::SPATK) ) break;

            for(unsigned int atk_assigned = 0; atk_assigned < MAX_EVS_SINGLE_STAT + 1; atk_assigned = atk_assigned + calculateEVSNextStat(attacker, Stats::ATK, atk_assigned)) {
                //qDebug() << QString::number(roll_count) + " " + QString::number(hp_assigned) + " " + QString::number(spdef_assigned) + " " + QString::number(def_assigned);
                //if an abort request is made
                if( abort_calculation ) { AttackResult temp; temp.atk_ev.push_back(-4); temp.spatk_ev.push_back(-4); return temp; }
                if( simplified && simplified_type == Move::SPECIAL && atk_assigned > getEV(Stats::ATK) ) break;

                bool to_add = true;
                for(unsigned int it = 0; it < theTurn.size() && to_add; it++ ) {
                    if( spatk_assigned + atk_assigned > assignable_evs ) to_add = false;
                    else if( results_buffer[it][atk_assigned + spatk_assigned * ARRAY_SIZE] < (100 - tolerances[it]) ) to_add = false;
                }

                if( to_add ) results.push_back(std::make_pair(atk_assigned, spatk_assigned));
            }
        }

    roll_count++;
    }
    //

    //if no result has been found
    if( results.empty() && !abort_calculation ) {
        AttackResult temp;
        temp.atk_ev.push_back(-1);
        temp.spatk_ev.push_back(-1);

        return temp;
    }

    //if an abort has been requested
    else if( abort_calculation ) {
        AttackResult temp;
        temp.atk_ev.push_back(-4);
        temp.spatk_ev.push_back(-4);

        return temp;
    }

    //finding all the minimum elements
    std::vector<unsigned int> sum_results;
    //creating the vector with the sums
    for( auto it = results.begin(); it < results.end(); it++ ) sum_results.push_back(it->first + it->second);
    //finding the minimum sums
    auto min_index = std::min_element(sum_results.begin(), sum_results.end());

    AttackResult final_result;
    final_result.atk_ev.push_back(results[std::distance(sum_results.begin(), min_index)].first);
    final_result.spatk_ev.push_back(results[std::distance(sum_results.begin(), min_index)].second);

    //now calculating all the damages
    Pokemon buffer = *this;
    buffer.setEV(Stats::ATK, final_result.atk_ev[0]);
    buffer.setEV(Stats::SPATK, final_result.spatk_ev[0]);

    final_result.atk_ko_prob.push_back(std::vector<float>());
    final_result.atk_damage_perc.push_back(std::vector<std::vector<float>>());
    final_result.atk_damage_int.push_back(std::vector<std::vector<int>>());
    for( auto it = 0; it < theTurn.size(); it++ ) {
        //doing this switch because the Turn is used in an offensive sense
        buffer.setModifier(Stats::ATK, std::get<0>(theAtkModifier[it]));
        buffer.setModifier(Stats::SPATK, std::get<1>(theAtkModifier[it]));
        buffer.setTeraType(std::get<2>(theAtkModifier[it]));
        buffer.setTerastallized(std::get<3>(theAtkModifier[it]));
        buffer.setRuinTablets(std::get<4>(theAtkModifier[it]));
        buffer.setRuinVessel(std::get<5>(theAtkModifier[it]));
        buffer.setHelpingHand(std::get<8>(theAtkModifier[it]));

        Pokemon def_copy_final = theDefendingPokemon[it];
        def_copy_final.setRuinSword(std::get<6>(theAtkModifier[it]));
        def_copy_final.setRuinBeads(std::get<7>(theAtkModifier[it]));

        Turn temp_turn;
        temp_turn.addMove(buffer, theTurn[it].getMoves()[0].second);
        temp_turn.setHits(theTurn[it].getHits());

        final_result.atk_ko_prob[0].push_back(def_copy_final.getKOProbability(temp_turn));
        final_result.atk_damage_perc[0].push_back(def_copy_final.getDamagePercentage(temp_turn));
        final_result.atk_damage_int[0].push_back(def_copy_final.getDamageInt(temp_turn));
    }

    return final_result;
}

std::pair<DefenseResult, AttackResult> Pokemon::calculateEVSDistrisbution(const EVCalculationInput& theInput) {

    // Auto nature: trigger if explicitly set to Auto, or if nature is neutral (doesn't affect any stat)
    auto isNeutralNature = [](Stats::Nature n) {
        return n == Stats::HARDY || n == Stats::DOCILE || n == Stats::SERIOUS ||
               n == Stats::BASHFUL || n == Stats::QUIRK;
    };
    if( getNature() == Stats::AUTO_NATURE || isNeutralNature(getNature()) ) {
        static const Stats::Nature CANDIDATE_NATURES[] = {
            Stats::ADAMANT, Stats::BRAVE, Stats::LONELY, Stats::NAUGHTY,  // +Atk
            Stats::MODEST,  Stats::QUIET, Stats::MILD,   Stats::RASH,     // +SpAtk
            Stats::BOLD,    Stats::RELAXED, Stats::IMPISH, Stats::LAX,    // +Def
            Stats::CALM,    Stats::SASSY, Stats::GENTLE, Stats::CAREFUL   // +SpDef
        };

        std::pair<DefenseResult, AttackResult> best_result;
        int best_total = INT_MAX;
        Stats::Nature best_nature = Stats::ADAMANT;
        bool found_any = false;

        for( Stats::Nature nat : CANDIDATE_NATURES ) {
            if( abort_calculation ) break;
            Pokemon clone = *this;
            clone.setNature(nat);
            auto result = clone.calculateEVSDistrisbution(theInput);
            if( abort_calculation ) break;

            bool def_ok = result.first.isEmptyInput() || (result.first.isValid() && !result.first.isEmpty());
            bool atk_ok = result.second.isEmptyInput() || (result.second.isValid() && !result.second.isEmpty());
            if( !def_ok || !atk_ok ) continue;

            int total = 0;
            if( !result.first.isEmptyInput() )  total += result.first.hp_ev[0]  + result.first.def_ev[0]  + result.first.spdef_ev[0];
            if( !result.second.isEmptyInput() ) total += result.second.atk_ev[0] + result.second.spatk_ev[0];

            if( !found_any || total < best_total ) {
                best_total   = total;
                best_result  = result;
                best_nature  = nat;
                found_any    = true;
            }
        }

        if( abort_calculation ) {
            DefenseResult d; d.hp_ev.push_back(-4); d.def_ev.push_back(-4); d.spdef_ev.push_back(-4);
            AttackResult  a; a.atk_ev.push_back(-4); a.spatk_ev.push_back(-4);
            return std::make_pair(d, a);
        }

        if( found_any ) {
            setNature(best_nature);
            return best_result;
        }

        // Nothing worked with any nature — return impossible spread with a concrete nature set
        setNature(Stats::ADAMANT);
        DefenseResult d; d.hp_ev.push_back(-1); d.def_ev.push_back(-1); d.spdef_ev.push_back(-1);
        AttackResult  a; a.atk_ev.push_back(-1); a.spatk_ev.push_back(-1);
        return std::make_pair(d, a);
    }

    //we do this calculation on a copy of *this
    Pokemon buffer = *this;

    //we check if any of the turn contains the move FOUL PLAY, if so the PRIORITIZE DEFENSE option should not be considered otherwise the damage calculation would be wrong
    bool foul_play = false;
    for( auto it = theInput.def_turn.begin(); it < theInput.def_turn.end(); it++ )
        if( it->isFoulPlay() ) foul_play = true;

    if( theInput.priority == PRIORITY_DEFENSE && !foul_play ) {
        auto def_result = resistMove(theInput.def_turn, theInput.def_modifier);
        //saving these variable because we change them and we need to restore them back later
        unsigned int tamp_hp = getEV(Stats::HP);
        unsigned int tamp_def = getEV(Stats::DEF);
        unsigned int tamp_spdef = getEV(Stats::SPDEF);
        AttackResult atk_result;
        for(unsigned int i = 0; i < def_result.hp_ev.size(); i++) { //we use hp.ev but really could use any of the 3
            setEV(Stats::HP, def_result.hp_ev[i]);
            setEV(Stats::DEF, def_result.def_ev[i]);
            setEV(Stats::SPDEF, def_result.spdef_ev[i]);

            auto atk_buffer = koMove(theInput.atk_turn, theInput.defending_pokemon, theInput.atk_modifier);

            if( atk_buffer.isEmpty() && i > 0 ) { //if the alternative spreads are unsuitable because they don't match for the offensive side we erase them
                def_result.hp_ev.erase(def_result.hp_ev.begin()+i);
                def_result.def_ev.erase(def_result.def_ev.begin()+i);
                def_result.spdef_ev.erase(def_result.spdef_ev.begin()+i);
                def_result.def_ko_prob.erase(def_result.def_ko_prob.begin()+i);
                def_result.def_damage_int.erase(def_result.def_damage_int.begin()+i);
                def_result.def_damage_perc.erase(def_result.def_damage_perc.begin()+i);
                i--;
            }

            else {
                atk_result.atk_ev.push_back(atk_buffer.atk_ev[0]);
                atk_result.spatk_ev.push_back(atk_buffer.spatk_ev[0]);
                if( !atk_buffer.atk_ko_prob.empty() ) atk_result.atk_ko_prob.push_back(atk_buffer.atk_ko_prob[0]); //because in case of error the function returns empty vectors
                if( !atk_buffer.atk_damage_int.empty() ) atk_result.atk_damage_int.push_back(atk_buffer.atk_damage_int[0]);
                if( !atk_buffer.atk_damage_perc.empty() ) atk_result.atk_damage_perc.push_back(atk_buffer.atk_damage_perc[0]);
            }

        }

        //restoring them
        setEV(Stats::HP, tamp_hp);
        setEV(Stats::DEF, tamp_def);
        setEV(Stats::SPDEF, tamp_spdef);

        return std::make_pair(def_result, atk_result);
    }

    else {
        auto atk_result = koMove(theInput.atk_turn, theInput.defending_pokemon, theInput.atk_modifier);
        //same as before, saving them because we need to restore them
        unsigned int tamp_atk = getEV(Stats::ATK);
        unsigned int tamp_spatk = getEV(Stats::SPATK);

        setEV(Stats::ATK, atk_result.atk_ev[0]);
        setEV(Stats::SPATK, atk_result.spatk_ev[0]);

        auto def_result = resistMove(theInput.def_turn, theInput.def_modifier);

        //if the defense result returned more than 1 suitable spread we fill the atk result with the same result just for coherence
        if( def_result.hp_ev.size() > 1 ) { //honestly we could check any of them
            for( unsigned int i = 1; i < def_result.hp_ev.size(); i++ ) {
                atk_result.atk_ev.push_back(atk_result.atk_ev[i-1]);
                atk_result.spatk_ev.push_back(atk_result.spatk_ev[i-1]);
                atk_result.atk_ko_prob.push_back(atk_result.atk_ko_prob[i-1]);
                atk_result.atk_damage_int.push_back(atk_result.atk_damage_int[i-1]);
                atk_result.atk_damage_perc.push_back(atk_result.atk_damage_perc[i-1]);
            }
        }

        setEV(Stats::ATK, tamp_atk);
        setEV(Stats::SPATK, tamp_spatk);
        return std::make_pair(def_result, atk_result);
    }
}

float Pokemon::getDEFTier() const {
    float score = getStat(Stats::HP) * getStat(Stats::DEF);
    return log(score) / log(1.1);
}

float Pokemon::getSPDEFTier() const {
    float score = getStat(Stats::HP) * getStat(Stats::SPDEF);
    return log(score) / log(1.1);
}
