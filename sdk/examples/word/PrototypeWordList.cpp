#include "PrototypeWordList.h"
#include <sifteo.h>
#include "WordGame.h"

/*
 * XXX: Only used for bsearch() currently. We should think about what kind of low-level VM
 *      primitives the search should be based on (with regard to ABI, as well as cache
 *      behavior) and design it with a proper syscall interface. But for now, we're leaking
 *      some libc code into the game :(
 */
#include <stdlib.h>

//#define DAWG_TEST 1
#ifdef DAWG_TEST
//static const int sDAWG[22000] = {};

static const char sList[][7] =
{
"aa",
"aah",
};

#else

// TODO a spell check representation that expands to higher number of letters
// while allowing for breaking into pieces to be loaded from flash
// (possibly pieces of a DAWG dictionary representation, up to 15 letters or so)
//
// 5 bits per letter, up to 6 letter words, plus flags
static const uint32_t sList[] =
{
    0x2b041,		// ABLE
    0x21461,		// ACED
    0x99461,		// ACES
    0x2b461,		// ACME
    0x2c861,		// ACRE
    0x5061,		// ACT
    0xa49098c1L,		// AFFAIR, seed word (533D: 6)
    0x8177b181L,		// ALLOW, seed word (533D: 5)
    0xcb181,		// ALLY
    0xa937b581L,		// ALMOST, seed word (533D: 6)
    0x9d181,		// ALTS
    0x725a1,		// AMIN
    0x51c1,		// ANT
    0x21601,		// APED
    0x91601,		// APER
    0xa932ca41L,		// ARREST, seed word (533D: 6)
    0x5241,		// ART
    0x9d241,		// ARTS
    0x66641,		// ARYL
    0x261,		// AS
    0x8b2ace61L,		// ASSURE, seed word (533D: 6)
    0x281,		// AT
    0x2e1,		// AW
    0x2b022,		// BALE
    0x8ad08ca2L,		// BECAME, seed word (533D: 6)
    0x8ad78ca2L,		// BECOME, seed word (533D: 6)
    0x8ee7b0a2L,		// BELONG, seed word (533D: 6)
    0x8a44cca2L,		// BESIDE, seed word (533D: 6)
    0xa05e2,		// BOAT
    0x344b9e2,		// BONITA
    0xd1e2,		// BOTA
    0xbbe42,		// BROW
    0x80ebbe42L,		// BROWN, seed word (533D: 5)
    0x29023,		// CADE
    0x63023,		// CALL
    0x88563023L,		// CALLED, seed word (533D: 6)
    0x2b423,		// CAME
    0x2c023,		// CAPE
    0x2c823,		// CARE
    0xa452c823L,		// CAREER, seed word (533D: 6)
    0x2cc23,		// CASE
    0x80005023L,		// CAT, seed word (533D: 3)
    0xa38a3,		// CENT
    0x245a38a3,		// CENTER
    0xb2a38a3,		// CENTRE
    0x2c8a3,		// CERE
    0x725e3,		// COIN
    0x2b5e3,		// COME
    0x4b9e3,		// CONI
    0xcc1e3,		// COPY
    0x2c9e3,		// CORE
    0x749e3,		// CORN
    0xa45749e3L,		// CORNER, seed word (533D: 6)
    0x9c024,		// DAPS
    0x2c824,		// DARE
    0x6424,		// DAY
    0x604a4,		// DEAL
    0x914a4,		// DEER
    0x498a4,		// DEFI
    0x30a4,		// DEL
    0x8190b0a4L,		// DELAY, seed word (533D: 5)
    0x630a4,		// DELL
    0x2c8a4,		// DERE
    0x9c74cca4L,		// DESIGN, seed word (533D: 6)
    0x8b24cca4L,		// DESIRE, seed word (533D: 6)
    0x64a4,		// DEY
    0x21524,		// DIED
    0x8a44d924L,		// DIVIDE, seed word (533D: 6)
    0x1e4,		// DO
    0xa4fa0de4L,		// DOCTOR, seed word (533D: 6)
    0x80001de4L,		// DOG, seed word (533D: 3)
    0x93de4,		// DOOR
    0xb8644,		// DRAW
    0x80ebbe44L,		// DROWN, seed word (533D: 5)
    0x3baa4,		// DUNG
    0x8ee4caa4L,		// DURING, seed word (533D: 6)
    0x9a085,		// EDHS
    0x1185,		// ELD
    0x2cd85,		// ELSE
    0x8b24c1a5L,		// EMPIRE, seed word (533D: 6)
    0x2934b1c5,		// ENLIST
    0x8b24d1c5L,		// ENTIRE, seed word (533D: 6)
    0x98645,		// ERAS
    0x2ba45,		// ERNE
    0xa4e45,		// ERST
    0x8b008e65L,		// ESCAPE, seed word (533D: 6)
    0x2285,		// ETH
    0x716c5,		// EVEN
    0x814716c5L,		// EVENT, seed word (533D: 5)
    0x92426,		// FAIR
    0x2c8a6,		// FERE
    0x1126,		// FID
    0x80461526L,		// FIELD, seed word (533D: 5)
    0x8b2a9d26L,		// FIGURE, seed word (533D: 6)
    0x2b126,		// FILE
    0x63126,		// FILL
    0x88563126L,		// FILLED, seed word (533D: 6)
    0x9134b926L,		// FINISH, seed word (533D: 6)
    0x2c926,		// FIRE
    0x44d26,		// FISH
    0x725e6,		// FOIN
    0x2c9e6,		// FORE
    0xa932c9e6L,		// FOREST, seed word (533D: 6)
    0x6c9e6,		// FORM
    0x245a4de6,		// FOSTER
    0x955e6,		// FOUR
    0x914955e6L,		// FOURTH, seed word (533D: 6)
    0x1e7,		// GO
    0x11e7,		// GOD
    0x904a8,		// HEAR
    0x814904a8L,		// HEART, seed word (533D: 5)
    0x48a8,		// HER
    0x50a8,		// HET
    0x1e8,		// HO
    0x5de8,		// HOW
    0x8ad78dc9L,		// INCOME, seed word (533D: 6)
    0x799c9,		// INFO
    0x9b2799c9L,		// INFORM, seed word (533D: 6)
    0x8a44cdc9L,		// INSIDE, seed word (533D: 6)
    0x269,		// IS
    0x88e0b269L,		// ISLAND, seed word (533D: 6)
    0x2b269,		// ISLE
    0x289,		// IT
    0x4e89,		// ITS
    0xbbdcb,		// KNOW
    0x80ebbdcbL,		// KNOWN, seed word (533D: 5)
    0x2902c,		// LADE
    0x2382c,		// LAND
    0xa4c2c,		// LAST
    0x9d02c,		// LATS
    0x5c2c,		// LAW
    0x642c,		// LAY
    0x984ac,		// LEAS
    0x814984acL,		// LEAST, seed word (533D: 5)
    0x10ac,		// LED
    0x994ac,		// LEES
    0x9a4ac,		// LEIS
    0x7b8ac,		// LENO
    0x9ccac,		// LESS
    0x9cf9ccacL,		// LESSON, seed word (533D: 6)
    0x50ac,		// LET
    0x3152c,		// LIEF
    0x2453152c,		// LIEFER
    0x7152c,		// LIEN
    0x9152c,		// LIER
    0x2992c,		// LIFE
    0x2b92c,		// LINE
    0x2c92c,		// LIRE
    0xa4d2c,		// LIST
    0x9c5a4d2cL,		// LISTEN, seed word (533D: 6)
    0x9d12c,		// LITS
    0x289ec,		// LOBE
    0x3b9ec,		// LONG
    0x5dec,		// LOW
    0x2cb2c,		// LYRE
    0x7242d,		// MAIN
    0x382d,		// MAN
    0x2c82d,		// MARE
    0xa4c2d,		// MAST
    0x245a4c2d,		// MASTER
    0x9d02d,		// MATS
    0x704ad,		// MEAN
    0x814704adL,		// MEANT, seed word (533D: 5)
    0xa45134adL,		// MEMBER, seed word (533D: 6)
    0x38ad,		// MEN
    0xbbcad,		// MEOW
    0x2c8ad,		// MERE
    0x50ad,		// MET
    0x8ac2112dL,		// MIDDLE, seed word (533D: 6)
    0x2b12d,		// MILE
    0xb1ed,		// MOLA
    0x7b9ed,		// MONO
    0x73ded,		// MOON
    0x2c9ed,		// MORE
    0xa4ded,		// MOST
    0x451ed,		// MOTH
    0xa45451edL,		// MOTHER, seed word (533D: 6)
    0x9cf4d1edL,		// MOTION, seed word (533D: 6)
    0x9d1ed,		// MOTS
    0x342e,		// NAM
    0x2b42e,		// NAME
    0x904ae,		// NEAR
    0xb2c904aeL,		// NEARLY, seed word (533D: 6)
    0xa4cae,		// NEST
    0x50ae,		// NET
    0x9d0ae,		// NETS
    0x2d8ae,		// NEVE
    0x2912e,		// NIDE
    0x4cd2e,		// NISI
    0x1ee,		// NO
    0x615ee,		// NOEL
    0x8a34d1eeL,		// NOTICE, seed word (533D: 6)
    0x80005deeL,		// NOW, seed word (533D: 3)
    0x292ae,		// NUDE
    0x9c90d04fL,		// OBTAIN, seed word (533D: 6)
    0xb30a8c6fL,		// OCCUPY, seed word (533D: 6)
    0x8f,		// OD
    0x93c8f,		// ODOR
    0x10f,		// OH
    0xb18f,		// OLLA
    0x915af,		// OMER
    0xa25af,		// OMIT
    0x1cf,		// ON
    0x28dcf,		// ONCE
    0x7924f,		// ORDO
    0x2ef,		// OW
    0x3aef,		// OWN
    0x92430,		// PAIR
    0x2c830,		// PARE
    0xa4830,		// PART
    0x819a4830L,		// PARTY, seed word (533D: 5)
    0x9cc30,		// PASS
    0x8859cc30L,		// PASSED, seed word (533D: 6)
    0x6430,		// PAY
    0x265184b0,		// PEACES
    0x614b0,		// PEEL
    0x994b0,		// PEES
    0x2b0b0,		// PELE
    0x8ac83cb0L,		// PEOPLE, seed word (533D: 6)
    0x7c0b0,		// PEPO
    0x91530,		// PIER
    0x2b1f0,		// POLE
    0x2c1f0,		// POPE
    0x44eb0,		// PUSH
    0x88544eb0L,		// PUSHED, seed word (533D: 6)
    0x2c832,		// RARE
    0x2932c832,		// RAREST
    0x245a4c32,		// RASTER
    0x5032,		// RAT
    0x9d032,		// RATS
    0x808184b2L,		// REACH, seed word (533D: 5)
    0x204b2,		// READ
    0x819204b2L,		// READY, seed word (533D: 5)
    0x604b2,		// REAL
    0xb2c604b2L,		// REALLY, seed word (533D: 6)
    0x684b2,		// REAM
    0x804b2,		// REAP
    0x904b2,		// REAR
    0xa8e28cb2L,		// RECENT, seed word (533D: 6)
    0x10b2,		// RED
    0x290b2,		// REDE
    0x214b2,		// REED
    0x314b2,		// REEF
    0x324b2,		// REIF
    0x724b2,		// REIN
    0x9a4b2,		// REIS
    0x8c54b0b2L,		// RELIEF, seed word (533D: 6)
    0xcb0b2,		// RELY
    0x9c90b4b2L,		// REMAIN, seed word (533D: 6)
    0xa38b2,		// RENT
    0xa490c0b2L,		// REPAIR, seed word (533D: 6)
    0x9c0b2,		// REPS
    0xa44ccb2,		// RESIDE
    0x2934ccb2,		// RESIST
    0xa4cb2,		// REST
    0xa8caccb2L,		// RESULT, seed word (533D: 6)
    0x2d0b2,		// RETE
    0x2932d0b2,		// RETEST
    0xae4d0b2,		// RETINE
    0x8b24d0b2L,		// RETIRE, seed word (533D: 6)
    0x9d0b2,		// RETS
    0x9d2ad0b2L,		// RETURN, seed word (533D: 6)
    0x3b932,		// RING
    0x9c132,		// RIPS
    0x23df2,		// ROOD
    0x3ab2,		// RUN
    0x452b2,		// RUTH
    0x2b033,		// SALE
    0x5033,		// SAT
    0x80005c33L,		// SAW, seed word (533D: 3)
    0x88e78cb3L,		// SECOND, seed word (533D: 6)
    0xa0cb3,		// SECT
    0x14b3,		// SEE
    0x614b3,		// SEEL
    0x814b3,		// SEEP
    0xa832b0b3L,		// SELECT, seed word (533D: 6)
    0x238b3,		// SEND
    0x50b3,		// SET
    0x21513,		// SHED
    0x72513,		// SHIN
    0xbbd13,		// SHOW
    0x80ebbd13L,		// SHOWN, seed word (533D: 5)
    0x133,		// SI
    0x60533,		// SIAL
    0x29133,		// SIDE
    0x71d33,		// SIGN
    0x8571d33,		// SIGNED
    0x3b933,		// SING
    0x853b933,		// SINGED
    0x8ac3b933L,		// SINGLE, seed word (533D: 6)
    0x2c933,		// SIRE
    0xa45a4d33L,		// SISTER, seed word (533D: 6)
    0x80005133L,		// SIT, seed word (533D: 3)
    0x9d133,		// SITS
    0x24520613,		// SPADER
    0x8812ca13L,		// SPREAD, seed word (533D: 6)
    0x8ee4ca13L,		// SPRING, seed word (533D: 6)
    0x90693,		// STAR
    0x24590693,		// STARER
    0xa1693,		// STET
    0x92693,		// STIR
    0x1816be93,		// STOMAL
    0x9a12ca93L,		// STREAM, seed word (533D: 6)
    0xa852ca93L,		// STREET, seed word (533D: 6)
    0x8ee4ca93L,		// STRING, seed word (533D: 6)
    0x916b3,		// SUER
    0xa4531ab3L,		// SUFFER, seed word (533D: 6)
    0xa456b6b3L,		// SUMMER, seed word (533D: 6)
    0x2cab3,		// SURE
    0x245652b3,		// SUTLER
    0x34,		// TA
    0x72434,		// TAIN
    0x3834,		// TAN
    0x4834,		// TAR
    0x4c34,		// TAS
    0x30b4,		// TEL
    0x38b4,		// TEN
    0x6c8b4,		// TERM
    0x8136c8b4L,		// TERMS, seed word (533D: 5)
    0xa4cb4,		// TEST
    0x245a4cb4,		// TESTER
    0x9d0b4,		// TETS
    0x1514,		// THE
    0x80599514L,		// THESE, seed word (533D: 5)
    0x72514,		// THIN
    0xac914,		// THRU
    0x134,		// TI
    0x91534,		// TIER
    0x2b934,		// TINE
    0x2c934,		// TIRE
    0x4d34,		// TIS
    0x8920ddf4L,		// TOWARD, seed word (533D: 6)
    0x80654,		// TRAP
    0x6654,		// TRY
    0x90674,		// TSAR
    0x74ab4,		// TURN
    0x88574ab4L,		// TURNED, seed word (533D: 6)
    0x24574ab4,		// TURNER
    0x8ac105d5L,		// UNABLE, seed word (533D: 6)
    0x291d5,		// UNDE
    0x812291d5L,		// UNDER, seed word (533D: 5)
    0xa732b1d5L,		// UNLESS, seed word (533D: 6)
    0x91675,		// USER
    0x9859ccb6L,		// VESSEL, seed word (533D: 6)
    0x50b6,		// VET
    0x29136,		// VIDE
    0x21536,		// VIED
    0x24837,		// WARD
    0x4c37,		// WAS
    0x2d837,		// WAVE
    0x8132d837L,		// WAVES, seed word (533D: 5)
    0x80003d17L,		// WHO, seed word (533D: 3)
    0x23937,		// WIND
    0xaef23937L,		// WINDOW, seed word (533D: 6)
    0xa45a3937L,		// WINTER, seed word (533D: 6)
    0x2c937,		// WIRE
    0x45137,		// WITH
    0x9c945137L,		// WITHIN, seed word (533D: 6)
    0x1f7,		// WO
    0x80e0b5f7L,		// WOMAN, seed word (533D: 5)
    0x80e2b5f7L,		// WOMEN, seed word (533D: 5)
    0x39f7,		// WON
    0x5b9f7,		// WONK
    0x249f7,		// WORD
    0x4039,		// YAP


};
#endif

PrototypeWordList::PrototypeWordList()
{
}


bool PrototypeWordList::pickWord(char* buffer)
{
    unsigned numWords = arraysize(sList);
    unsigned startIndex = WordGame::random.randrange(numWords);
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

            if (_SYS_strnlen(word, MAX_LETTERS_PER_WORD + 1) ==
                    GameStateMachine::getCurrentMaxLettersPerWord())
            {
                 _SYS_strlcpy(buffer, word, GameStateMachine::getCurrentMaxLettersPerWord() + 1);
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
        return _SYS_strncmp(*(const char **)a, word, GameStateMachine::getCurrentMaxLettersPerWord() + 1);
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
    _SYS_memset8((uint8_t*)word, 0, sizeof(word));
    const unsigned LTR_MASK = 0x1f; // 5 bits per letter
    const unsigned BITS_PER_LETTER = 5;
    // TODO store dictionary differently to allow for longer than 6 letter words
    for (unsigned j = 0; j < MAX_LETTERS_PER_WORD && j < 32/BITS_PER_LETTER; ++j)
    {
        char letter = 'A' - 1 + ((bits >> (j * BITS_PER_LETTER)) & LTR_MASK);
        if (letter < 'A' || letter > 'Z')
        {
            break;
        }
        word[j] = letter;
    }
    _SYS_strlcpy(buffer, word, arraysize(word));
    return buffer[0] != '\0';
}
