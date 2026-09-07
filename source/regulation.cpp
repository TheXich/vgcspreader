#include "regulation.hpp"

#include <algorithm>
#include <cstdint>

#include "items.hpp"

/*
Legal rosters for the Pokemon Champions Ranked Battle regulations.

Sources (checked 2026-09-07):
  https://www.serebii.net/pokemonchampions/rankedbattle/regulationm-a.shtml
  https://www.serebii.net/pokemonchampions/rankedbattle/regulationm-b.shtml
  https://rotompicks.com/en/m-a/items/  and  https://rotompicks.com/en/items/   (held item pools)

Regulation M-A ran April 8th 2026 - June 17th 2026 (260 legal species/form combinations).
Regulation M-B is additive over M-A: it kept the whole M-A roster and added 22 species,
16 Mega Evolutions and 15 held items (298 legal species/form combinations).

Each entry is a (National Dex number, form index) pair, where the form index matches the one used by
personal_species.bin (0 = base form, 1 = first alternate form, ...). Mega Evolutions are ordinary
alternate forms here, so banning Mega Lucario Z simply means not listing that form.
The tables are kept sorted so they can be searched with std::binary_search.
*/

static const unsigned int MAX_FORM_SCAN = 16; //no species in the database has more alternate forms than this

#define P(dex, form) (((uint32_t)(dex) << 8) | (uint32_t)(form))

//Regulation M-A roster
static const uint32_t CHAMPIONS_MA_FORMS[] = {
    P(3,0), P(3,1), P(6,0), P(6,1), P(6,2), P(9,0), P(9,1), P(15,0),
    P(15,1), P(18,0), P(18,1), P(24,0), P(25,0), P(26,0), P(26,1), P(36,0),
    P(36,1), P(38,0), P(38,1), P(59,0), P(59,1), P(65,0), P(65,1), P(68,0),
    P(71,0), P(71,1), P(80,0), P(80,1), P(80,2), P(94,0), P(94,1), P(115,0),
    P(115,1), P(121,0), P(121,1), P(127,0), P(127,1), P(128,0), P(128,1), P(128,2),
    P(128,3), P(130,0), P(130,1), P(132,0), P(134,0), P(135,0), P(136,0), P(142,0),
    P(142,1), P(143,0), P(149,0), P(149,1), P(154,0), P(154,1), P(157,0), P(157,1),
    P(160,0), P(160,1), P(168,0), P(181,0), P(181,1), P(184,0), P(186,0), P(196,0),
    P(197,0), P(199,0), P(199,1), P(205,0), P(208,0), P(208,1), P(212,0), P(212,1),
    P(214,0), P(214,1), P(227,0), P(227,1), P(229,0), P(229,1), P(248,0), P(248,1),
    P(279,0), P(282,0), P(282,1), P(302,0), P(302,1), P(306,0), P(306,1), P(308,0),
    P(308,1), P(310,0), P(310,1), P(319,0), P(319,1), P(323,0), P(323,1), P(324,0),
    P(334,0), P(334,1), P(350,0), P(351,0), P(354,0), P(354,1), P(358,0), P(358,1),
    P(359,0), P(359,1), P(362,0), P(362,1), P(389,0), P(392,0), P(395,0), P(405,0),
    P(407,0), P(409,0), P(411,0), P(428,0), P(428,1), P(442,0), P(445,0), P(445,1),
    P(448,0), P(448,1), P(450,0), P(454,0), P(460,0), P(460,1), P(461,0), P(464,0),
    P(470,0), P(471,0), P(472,0), P(473,0), P(475,0), P(475,1), P(478,0), P(478,1),
    P(479,0), P(497,0), P(500,0), P(500,1), P(503,0), P(503,1), P(505,0), P(510,0),
    P(512,0), P(514,0), P(516,0), P(530,0), P(530,1), P(531,0), P(531,1), P(534,0),
    P(547,0), P(553,0), P(563,0), P(569,0), P(571,0), P(571,1), P(579,0), P(584,0),
    P(587,0), P(609,0), P(609,1), P(614,0), P(618,0), P(618,1), P(623,0), P(623,1),
    P(635,0), P(637,0), P(652,0), P(652,1), P(655,0), P(655,1), P(658,0), P(658,3),
    P(660,0), P(663,0), P(666,0), P(670,1), P(670,2), P(671,0), P(675,0), P(676,0),
    P(678,0), P(678,2), P(681,0), P(683,0), P(685,0), P(693,0), P(695,0), P(697,0),
    P(699,0), P(700,0), P(701,0), P(701,1), P(702,0), P(706,0), P(706,1), P(707,0),
    P(709,0), P(711,0), P(713,0), P(713,1), P(715,0), P(724,0), P(724,1), P(727,0),
    P(730,0), P(733,0), P(740,0), P(740,1), P(745,0), P(748,0), P(750,0), P(752,0),
    P(758,0), P(763,0), P(765,0), P(766,0), P(778,0), P(780,0), P(780,1), P(784,0),
    P(823,0), P(841,0), P(842,0), P(844,0), P(855,0), P(858,0), P(866,0), P(867,0),
    P(869,0), P(877,0), P(887,0), P(899,0), P(900,0), P(902,0), P(903,0), P(908,0),
    P(911,0), P(914,0), P(925,0), P(934,0), P(936,0), P(937,0), P(939,0), P(952,0),
    P(952,1), P(956,0), P(959,0), P(964,0), P(968,0), P(970,0), P(970,1), P(981,0),
    P(983,0), P(1013,0), P(1018,0), P(1019,0),
};

//Pokemon and forms introduced by Regulation M-B on top of the M-A roster
static const uint32_t CHAMPIONS_MB_NEW_FORMS[] = {
    P(26,2), P(26,3), P(45,0), P(211,0), P(254,0), P(254,1), P(257,0), P(257,1),
    P(260,0), P(260,1), P(303,0), P(303,1), P(376,0), P(376,1), P(398,0), P(398,1),
    P(518,0), P(545,0), P(545,1), P(560,0), P(560,1), P(604,0), P(604,1), P(668,0),
    P(668,1), P(687,0), P(687,1), P(689,0), P(689,1), P(691,0), P(691,1), P(861,0),
    P(870,0), P(870,1), P(904,0), P(972,0), P(979,0), P(1000,0),
};

#undef P

//Items that Regulation M-A left out of the game's held item pool, among the ones this program knows about
static bool isItemLegalInMA(const unsigned int theItemIndex) {
    switch(theItemIndex) {
        //not available in Pokemon Champions at all
        case Items::Assault_Vest:
        case Items::Choice_Band:
        case Items::Choice_Specs:
        case Items::Booster_Energy:
        case Items::Clear_Amulet:
        case Items::Covert_Cloak:
        case Items::Loaded_Dice:
        case Items::Mirror_Herb:
        case Items::Punching_Glove:
        case Items::Eviolite:
        //the confusion-healing pinch berries are not part of the Champions item pool either
        case Items::Aguav_Berry:
        case Items::Figy_Berry:
        case Items::Iapapa_Berry:
        case Items::Mago_Berry_:
        case Items::Wiki_Berry:
        //added later, in Regulation M-B
        case Items::Life_Orb:
            return false;
        default:
            return true;
    }
}

/*static*/ Regulation::Format Regulation::current = Regulation::DEFAULT_FORMAT;

/*static*/ const char* Regulation::getName(const Format theFormat) {
    switch(theFormat) {
        case CHAMPIONS_MB: return "Champions Reg. M-B";
        case CHAMPIONS_MA: return "Champions Reg. M-A";
        case NATIONAL_DEX: return "National Dex";
        default:           return "Unknown";
    }
}

static bool contains(const uint32_t* theTable, const size_t theSize, const uint32_t theKey) {
    return std::binary_search(theTable, theTable + theSize, theKey);
}

/*static*/ bool Regulation::isFormLegal(const unsigned int thePokedexNumber, const unsigned int theForm) {
    if( current == NATIONAL_DEX ) return true;

    const uint32_t key = ((uint32_t)thePokedexNumber << 8) | (uint32_t)theForm;
    if( contains(CHAMPIONS_MA_FORMS, sizeof(CHAMPIONS_MA_FORMS) / sizeof(uint32_t), key) ) return true;
    if( current == CHAMPIONS_MB ) return contains(CHAMPIONS_MB_NEW_FORMS, sizeof(CHAMPIONS_MB_NEW_FORMS) / sizeof(uint32_t), key);
    return false;
}

/*static*/ int Regulation::getFirstLegalForm(const unsigned int thePokedexNumber) {
    if( current == NATIONAL_DEX ) return 0;

    //form indexes never go past a handful of entries, so a linear scan over the possible range is enough
    for(unsigned int form = 0; form < MAX_FORM_SCAN; form++)
        if( isFormLegal(thePokedexNumber, form) ) return (int)form;
    return -1;
}

/*static*/ bool Regulation::isSpeciesLegal(const unsigned int thePokedexNumber) {
    return getFirstLegalForm(thePokedexNumber) >= 0;
}

/*static*/ bool Regulation::isItemLegal(const unsigned int theItemIndex) {
    switch(current) {
        case NATIONAL_DEX: return true;
        case CHAMPIONS_MA: return isItemLegalInMA(theItemIndex);
        case CHAMPIONS_MB: return isItemLegalInMA(theItemIndex) || theItemIndex == Items::Life_Orb;
        default:           return true;
    }
}
