#ifndef _REGULATION_HPP_
#define _REGULATION_HPP_

/*This class holds the battle format ("regulation") the calculator is currently working with.
A regulation restricts which species, which forms of those species and which held items may be used,
so the GUI uses it to filter every species / form / item combo box.
The National Dex format imposes no restriction at all (the historical behaviour of this program).
NOTE: This is a Qt-independant class*/

class Regulation {
    public:
        enum Format {
            NATIONAL_DEX = 0,   //everything in the database
            CHAMPIONS_MA = 1,   //Pokemon Champions Ranked Battle, Regulation M-A
            CHAMPIONS_MB = 2,   //Pokemon Champions Ranked Battle, Regulation M-B
            FORMAT_NUM
        };

        static const Format DEFAULT_FORMAT = CHAMPIONS_MB; //the most recent regulation

        static Format getCurrent() { return current; }
        static void setCurrent(const Format theFormat) { current = theFormat; }

        static const char* getName(const Format theFormat);

        static bool isFormLegal(const unsigned int thePokedexNumber, const unsigned int theForm); //is this exact (species, form) usable in the current format?
        static bool isSpeciesLegal(const unsigned int thePokedexNumber); //is at least one form of this species usable in the current format?
        static int getFirstLegalForm(const unsigned int thePokedexNumber); //lowest legal form index, -1 when the species is banned
        static bool isItemLegal(const unsigned int theItemIndex);

    private:
        static Format current;
};

#endif
