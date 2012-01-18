#include "PrototypeWordList.h"
#include <string.h>
#include <cstdlib>
#include <sifteo.h>
#include "WordGame.h"

//#define DAWG_TEST 1
#ifdef DAWG_TEST
//static const int sDAWG[22000] = {};

static const char sList[][7] =
{
"aa",
"aah",
};

#else

// compressed 5 bit letters
static const uint32_t sList[] =
{
    0x21441,		// ABED
    0xa1441,		// ABET
    0x21461,		// ACED
    0x99461,		// ACES
    0x904c1,		// AFAR
    0x8890c8c1L,		// AFRAID, seed word (533D: 6)
    0x214e1,		// AGED
    0x914e1,		// AGER
    0x8852c8e1L,		// AGREED, seed word (533D: 6)
    0xcb181,		// ALLY
    0xa937b581L,		// ALMOST, seed word (533D: 6)
    0x9d181,		// ALTS
    0x725a1,		// AMIN
    0x995c1,		// ANES
    0x73dc1,		// ANON
    0x2d1c1,		// ANTE
    0x4d1c1,		// ANTI
    0x91601,		// APER
    0x40e41,		// ARCH
    0x22641,		// ARID
    0xa932ca41L,		// ARREST, seed word (533D: 6)
    0x8b64ca41L,		// ARRIVE, seed word (533D: 6)
    0x9d241,		// ARTS
    0x8b2ace61L,		// ASSURE, seed word (533D: 6)
    0x29022,		// BADE
    0x2d022,		// BATE
    0xa05e2,		// BOAT
    0x344b9e2,		// BONITA
    0x6bde2,		// BOOM
    0xd1e2,		// BOTA
    0xa51e2,		// BOTT
    0x9afa51e2L,		// BOTTOM, seed word (533D: 6)
    0x29023,		// CADE
    0x63023,		// CALL
    0x88563023L,		// CALLED, seed word (533D: 6)
    0x2c023,		// CAPE
    0x2cc23,		// CASE
    0xa38a3,		// CENT
    0x245a38a3,		// CENTER
    0xb2a38a3,		// CENTRE
    0x2c8a3,		// CERE
    0x90503,		// CHAR
    0x8a790503L,		// CHARGE, seed word (533D: 6)
    0x72503,		// CHIN
    0x26572503,		// CHINES
    0x725e3,		// COIN
    0x2b5e3,		// COME
    0x4b9e3,		// CONI
    0x249e3,		// CORD
    0x245249e3,		// CORDER
    0x2c9e3,		// CORE
    0x996a3,		// CUES
    0x2caa3,		// CURE
    0x9c024,		// DAPS
    0x2c824,		// DARE
    0x8b4088a4L,		// DEBATE, seed word (533D: 6)
    0x8a448ca4L,		// DECIDE, seed word (533D: 6)
    0x58ca4,		// DECK
    0x214a4,		// DEED
    0x914a4,		// DEER
    0xa14a4,		// DEET
    0x498a4,		// DEFI
    0xa18a4,		// DEFT
    0x851a4a4,		// DEICED
    0x624a4,		// DEIL
    0x4b0a4,		// DELI
    0x630a4,		// DELL
    0x7b4a4,		// DEMO
    0x854b8a4,		// DENIED
    0x2c8a4,		// DERE
    0x9c74cca4L,		// DESIGN, seed word (533D: 6)
    0x8b24cca4L,		// DESIRE, seed word (533D: 6)
    0x21524,		// DIED
    0x8a44d924L,		// DIVIDE, seed word (533D: 6)
    0xa4fa0de4L,		// DOCTOR, seed word (533D: 6)
    0x93de4,		// DOOR
    0x2c1e4,		// DOPE
    0x38644,		// DRAG
    0x9c5b2644L,		// DRIVEN, seed word (533D: 6)
    0x9a085,		// EDHS
    0x2cd85,		// ELSE
    0xb27915c5L,		// ENERGY, seed word (533D: 6)
    0x8a709dc5L,		// ENGAGE, seed word (533D: 6)
    0x2934b1c5,		// ENLIST
    0x8b24d1c5L,		// ENTIRE, seed word (533D: 6)
    0x98645,		// ERAS
    0x2ba45,		// ERNE
    0xa4e45,		// ERST
    0x8b008e65L,		// ESCAPE, seed word (533D: 6)
    0xba85,		// ETNA
    0x2c8a6,		// FERE
    0x63126,		// FILL
    0x88563126L,		// FILLED, seed word (533D: 6)
    0xa2586,		// FLIT
    0x265a2586,		// FLITES
    0x725e6,		// FOIN
    0x2c9e6,		// FORE
    0xa932c9e6L,		// FOREST, seed word (533D: 6)
    0x6c9e6,		// FORM
    0x245a4de6,		// FOSTER
    0x955e6,		// FOUR
    0x914955e6L,		// FOURTH, seed word (533D: 6)
    0x2cea6,		// FUSE
    0x21427,		// GAED
    0x71427,		// GAEN
    0x29c27,		// GAGE
    0x2b827,		// GANE
    0x9c524827L,		// GARDEN, seed word (533D: 6)
    0x904a7,		// GEAR
    0x2b8a7,		// GENE
    0x2cb27,		// GYRE
    0xae2cb27,		// GYRENE
    0x21469,		// ICED
    0x40dc9,		// INCH
    0xa6540dc9L,		// INCHES, seed word (533D: 6)
    0x8ad78dc9L,		// INCOME, seed word (533D: 6)
    0x885291c9L,		// INDEED, seed word (533D: 6)
    0x799c9,		// INFO
    0x9b2799c9L,		// INFORM, seed word (533D: 6)
    0x8a44cdc9L,		// INSIDE, seed word (533D: 6)
    0x2b269,		// ISLE
    0x8cc2ce89L,		// ITSELF, seed word (533D: 6)
    0xa4c2c,		// LAST
    0x9d02c,		// LATS
    0x994ac,		// LEES
    0x9a4ac,		// LEIS
    0x7b8ac,		// LENO
    0xa38ac,		// LENT
    0x9ccac,		// LESS
    0x9cf9ccacL,		// LESSON, seed word (533D: 6)
    0x2152c,		// LIED
    0x3152c,		// LIEF
    0x2453152c,		// LIEFER
    0x7152c,		// LIEN
    0x9152c,		// LIER
    0x2992c,		// LIFE
    0xa192c,		// LIFT
    0x885a192cL,		// LIFTED, seed word (533D: 6)
    0x2b92c,		// LINE
    0x2c92c,		// LIRE
    0xa4d2c,		// LIST
    0x9c5a4d2cL,		// LISTEN, seed word (533D: 6)
    0x9d12c,		// LITS
    0x2cb2c,		// LYRE
    0x7242d,		// MAIN
    0x2c82d,		// MARE
    0xa4c2d,		// MAST
    0x245a4c2d,		// MASTER
    0x9d02d,		// MATS
    0xa502d,		// MATT
    0xa45a502dL,		// MATTER, seed word (533D: 6)
    0x291ed,		// MODE
    0x9d2291edL,		// MODERN, seed word (533D: 6)
    0xb1ed,		// MOLA
    0x7b9ed,		// MONO
    0x73ded,		// MOON
    0x2c9ed,		// MORE
    0x749ed,		// MORN
    0xa4ded,		// MOST
    0x451ed,		// MOTH
    0xa45451edL,		// MOTHER, seed word (533D: 6)
    0x9cf4d1edL,		// MOTION, seed word (533D: 6)
    0x9d1ed,		// MOTS
    0xa51ed,		// MOTT
    0x9cf4d02eL,		// NATION, seed word (533D: 6)
    0x248ae,		// NERD
    0xa4cae,		// NEST
    0x9d0ae,		// NETS
    0x4d8ae,		// NEVI
    0x26540d2e,		// NICHES
    0x2912e,		// NIDE
    0x4cd2e,		// NISI
    0x615ee,		// NOEL
    0xb9ee,		// NONA
    0x9c90d04fL,		// OBTAIN, seed word (533D: 6)
    0x93c8f,		// ODOR
    0x915af,		// OMER
    0xa25af,		// OMIT
    0x7924f,		// ORDO
    0x9b2ef,		// OWLS
    0x92430,		// PAIR
    0x2c830,		// PARE
    0x265184b0,		// PEACES
    0x614b0,		// PEEL
    0x994b0,		// PEES
    0x2b0b0,		// PELE
    0x8ac83cb0L,		// PEOPLE, seed word (533D: 6)
    0x7c0b0,		// PEPO
    0x4c8b0,		// PERI
    0x88f4c8b0L,		// PERIOD, seed word (533D: 6)
    0x58d30,		// PICK
    0x88558d30L,		// PICKED, seed word (533D: 6)
    0x21530,		// PIED
    0x2b1f0,		// POLE
    0x2c1f0,		// POPE
    0x2c9f0,		// PORE
    0xa49f0,		// PORT
    0x245a49f0,		// PORTER
    0x83e50,		// PROP
    0xa4583e50L,		// PROPER, seed word (533D: 6)
    0x44eb0,		// PUSH
    0x88544eb0L,		// PUSHED, seed word (533D: 6)
    0x29c32,		// RAGE
    0x22432,		// RAID
    0x2c832,		// RARE
    0x2932c832,		// RAREST
    0x245a4c32,		// RASTER
    0x45032,		// RATH
    0xa4545032L,		// RATHER, seed word (533D: 6)
    0x9d032,		// RATS
    0x2d832,		// RAVE
    0x204b2,		// READ
    0x604b2,		// REAL
    0xb2c604b2L,		// REALLY, seed word (533D: 6)
    0x684b2,		// REAM
    0x804b2,		// REAP
    0x904b2,		// REAR
    0xa8e28cb2L,		// RECENT, seed word (533D: 6)
    0x89278cb2L,		// RECORD, seed word (533D: 6)
    0xb3a8cb2,		// RECUSE
    0x290b2,		// REDE
    0x214b2,		// REED
    0x314b2,		// REEF
    0x994b2,		// REES
    0x8b3a98b2L,		// REFUSE, seed word (533D: 6)
    0x724b2,		// REIN
    0x9a4b2,		// REIS
    0x8c54b0b2L,		// RELIEF, seed word (533D: 6)
    0xcb0b2,		// RELY
    0x9c90b4b2L,		// REMAIN, seed word (533D: 6)
    0xa38b2,		// RENT
    0xa490c0b2L,		// REPAIR, seed word (533D: 6)
    0x7c0b2,		// REPO
    0xa927c0b2L,		// REPORT, seed word (533D: 6)
    0x9c0b2,		// REPS
    0xa44ccb2,		// RESIDE
    0x2934ccb2,		// RESIST
    0xa4cb2,		// REST
    0xa8caccb2L,		// RESULT, seed word (533D: 6)
    0x2d0b2,		// RETE
    0x2932d0b2,		// RETEST
    0xae4d0b2,		// RETINE
    0x9d0b2,		// RETS
    0x2c132,		// RIPE
    0x2d932,		// RIVE
    0x291f2,		// RODE
    0x631f2,		// ROLL
    0x885631f2L,		// ROLLED, seed word (533D: 6)
    0x23df2,		// ROOD
    0x58eb2,		// RUCK
    0xa4eb2,		// RUST
    0x452b2,		// RUTH
    0x9d2b2,		// RUTS
    0xa0cb3,		// SECT
    0x8b2a8cb3L,		// SECURE, seed word (533D: 6)
    0x614b3,		// SEEL
    0x814b3,		// SEEP
    0x914b3,		// SEER
    0xa832b0b3L,		// SELECT, seed word (533D: 6)
    0x330b3,		// SELF
    0x8b40b8b3L,		// SENATE, seed word (533D: 6)
    0x2c8b3,		// SERE
    0x21513,		// SHED
    0x29133,		// SIDE
    0x71d33,		// SIGN
    0x8571d33,		// SIGNED
    0xa8e2b133L,		// SILENT, seed word (533D: 6)
    0x83533,		// SIMP
    0x8ac83533L,		// SIMPLE, seed word (533D: 6)
    0x3b933,		// SING
    0x853b933,		// SINGED
    0x8ac3b933L,		// SINGLE, seed word (533D: 6)
    0x2c933,		// SIRE
    0xa45a4d33L,		// SISTER, seed word (533D: 6)
    0x9d133,		// SITS
    0xbbd93,		// SLOW
    0xa45bbd93L,		// SLOWER, seed word (533D: 6)
    0x885625b3L,		// SMILED, seed word (533D: 6)
    0x24520613,		// SPADER
    0x8812ca13L,		// SPREAD, seed word (533D: 6)
    0x90693,		// STAR
    0x24590693,		// STARER
    0xa1693,		// STET
    0x1816be93,		// STOMAL
    0x9a12ca93L,		// STREAM, seed word (533D: 6)
    0xa852ca93L,		// STREET, seed word (533D: 6)
    0x963aca93L,		// STRUCK, seed word (533D: 6)
    0x916b3,		// SUER
    0xa4531ab3L,		// SUFFER, seed word (533D: 6)
    0xa456b6b3L,		// SUMMER, seed word (533D: 6)
    0x2cab3,		// SURE
    0x245652b3,		// SUTLER
    0x72434,		// TAIN
    0x214b4,		// TEED
    0x994b4,		// TEES
    0xa4cb4,		// TEST
    0x245a4cb4,		// TESTER
    0x9d0b4,		// TETS
    0xac914,		// THRU
    0x91534,		// TIER
    0x99534,		// TIES
    0x2b934,		// TINE
    0x2c934,		// TIRE
    0x83e54,		// TROP
    0x90674,		// TSAR
    0x91675,		// USER
    0xc8b6,		// VERA
    0x29136,		// VIDE
    0x21536,		// VIED
    0x2b936,		// VINE
    0x2c9f7,		// WORE
};
#endif

PrototypeWordList::PrototypeWordList()
{
}


bool PrototypeWordList::pickWord(char* buffer)
{
    unsigned numWords = arraysize(sList);
    unsigned startIndex = WordGame::rand(numWords);
    unsigned i = startIndex;
    do
    {
        // if the 3 seed word bits are set to this num cubes
        if (((sList[i] >> 31)))
        {
            char word[MAX_LETTERS_PER_WORD + 1];
            if (!bitsToString(sList[i], word))
            {
                ASSERT(0);
                return false;
            }

            if (strlen(word) == MAX_LETTERS_PER_WORD)
            {
                strcpy(buffer, word);
                return true;
            }
        }
        i = (i + 1) % numWords;
    } while (i != startIndex);

    ASSERT(0);
    return false;
}

static int bsearch_strcmp(const void*a,const void*b)
{
    // a is pKey in previous call, char**
    //STATIC_ASSERT(sizeof(uint32_t) == sizeof(unsigned));
    uint32_t* pb = (uint32_t*)b;
#if DEBUG
    const char** pa = (const char**)a;
    //printf("a %s, b %s\n", *(const char **)a, (const char *)b);
#endif

    char word[MAX_LETTERS_PER_WORD + 1];
    if (PrototypeWordList::bitsToString(*pb, word))
    {
        return strcmp(*(const char **)a, word);
    }
    ASSERT(0);
    return 0;
}

bool PrototypeWordList::isWord(const char* string)
{
#ifndef DAWG_TEST
    //STATIC_ASSERT(arraysize(sList) == 28839);
#endif
    const char** pKey = &string;
    const uint32_t* array = (uint32_t*)sList;
    //const char** pArray = &array;
    const char* pItem =
            (const char*) bsearch(
                pKey,
                array,
                arraysize(sList),
                sizeof(sList[0]),
                (int(*)(const void*,const void*)) bsearch_strcmp);

    return (pItem!=NULL);
}

bool PrototypeWordList::bitsToString(uint32_t bits, char* buffer)
{
    char word[MAX_LETTERS_PER_WORD + 1];
    memset(word, 0, sizeof(word));
    const unsigned LTR_MASK = 0x1f; // 5 bits per letter
    const unsigned BITS_PER_LETTER = 5;
    for (unsigned j = 0; j < MAX_LETTERS_PER_WORD; ++j)
    {
        char letter = 'A' - 1 + ((bits >> (j * BITS_PER_LETTER)) & LTR_MASK);
        if (letter < 'A' || letter > 'Z')
        {
            break;
        }
        word[j] = letter;
    }
    strcpy(buffer, word);
    return strlen(buffer) > 0;
}
