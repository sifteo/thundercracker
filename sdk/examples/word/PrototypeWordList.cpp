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
    0x1461,		// ACE
    0x21461,		// ACED
    0x99461,		// ACES
    0x2b461,		// ACME
    0x2c861,		// ACRE
    0x5061,		// ACT
    0xa49098c1L,		// AFFAIR, seed word (533D: 6)
    0x50c1,		// AFT
    0x8122d0c1L,		// AFTER, seed word (533D: 5)
    0x80409501L,		// AHEAD, seed word (533D: 5)
    0x3121,		// AIL
    0x4921,		// AIR
    0x1581,		// ALE
    0x8055a581L,		// ALIKE, seed word (533D: 5)
    0x3181,		// ALL
    0x8177b181L,		// ALLOW, seed word (533D: 5)
    0xcb181,		// ALLY
    0xa937b581L,		// ALMOST, seed word (533D: 6)
    0x9b581,		// ALMS
    0x80573d81L,		// ALONE, seed word (533D: 5)
    0x122d181,		// ALTER
    0x9d181,		// ALTS
    0x725a1,		// AMIN
    0x41a1,		// AMP
    0x11c1,		// AND
    0x51c1,		// ANT
    0x21601,		// APED
    0x91601,		// APER
    0xe41,		// ARC
    0x40e41,		// ARCH
    0x1641,		// ARE
    0xa932ca41L,		// ARREST, seed word (533D: 6)
    0x5241,		// ART
    0x9d241,		// ARTS
    0x66641,		// ARYL
    0x261,		// AS
    0x1478e61,		// ASCOT
    0x2e61,		// ASK
    0x8042ae61L,		// ASKED, seed word (533D: 5)
    0x8b2ace61L,		// ASSURE, seed word (533D: 6)
    0x281,		// AT
    0x1681,		// ATE
    0x2e1,		// AW
    0x80ca9ae1L,		// AWFUL, seed word (533D: 5)
    0x32e1,		// AWL
    0x2b022,		// BALE
    0x8ad08ca2L,		// BECAME, seed word (533D: 6)
    0x8ad78ca2L,		// BECOME, seed word (533D: 6)
    0x1ca2,		// BEG
    0x80e49ca2L,		// BEGIN, seed word (533D: 5)
    0x80ea9ca2L,		// BEGUN, seed word (533D: 5)
    0x807724a2L,		// BEING, seed word (533D: 5)
    0x8ee7b0a2L,		// BELONG, seed word (533D: 6)
    0x8a44cca2L,		// BESIDE, seed word (533D: 6)
    0x3922,		// BIN
    0x53b922,		// BINGE
    0xa0582,		// BLAT
    0x5a0582,		// BLATE
    0xa05e2,		// BOAT
    0x344b9e2,		// BONITA
    0xd1e2,		// BOTA
    0xbbe42,		// BROW
    0x80ebbe42L,		// BROWN, seed word (533D: 5)
    0x814626a2L,		// BUILT, seed word (533D: 5)
    0x3aa2,		// BUN
    0x52a2,		// BUT
    0x29023,		// CADE
    0x63023,		// CALL
    0x88563023L,		// CALLED, seed word (533D: 6)
    0x2b423,		// CAME
    0x3823,		// CAN
    0x2c023,		// CAPE
    0x4823,		// CAR
    0x2c823,		// CARE
    0xa452c823L,		// CAREER, seed word (533D: 6)
    0x564823,		// CARLE
    0x81994823L,		// CARRY, seed word (533D: 5)
    0x2cc23,		// CASE
    0x80005023L,		// CAT, seed word (533D: 3)
    0x8081d023L,		// CATCH, seed word (533D: 5)
    0x8059d423L,		// CAUSE, seed word (533D: 5)
    0x30a3,		// CEL
    0xa38a3,		// CENT
    0x245a38a3,		// CENTER
    0xb2a38a3,		// CENTRE
    0x813a38a3L,		// CENTS, seed word (533D: 5)
    0x2c8a3,		// CERE
    0x7c8a3,		// CERO
    0x90503,		// CHAR
    0x81490503L,		// CHART, seed word (533D: 5)
    0xa0503,		// CHAT
    0x1409503,		// CHEAT
    0x80e09583L,		// CLEAN, seed word (533D: 5)
    0x81209583L,		// CLEAR, seed word (533D: 5)
    0x8059bd83L,		// CLOSE, seed word (533D: 5)
    0x804abd83L,		// CLOUD, seed word (533D: 5)
    0x605e3,		// COAL
    0x814985e3L,		// COAST, seed word (533D: 5)
    0x725e3,		// COIN
    0x31e3,		// COL
    0xb1e3,		// COLA
    0x132b1e3,		// COLES
    0x8127b1e3L,		// COLOR, seed word (533D: 5)
    0x2b5e3,		// COME
    0x4b9e3,		// CONI
    0x63de3,		// COOL
    0xcc1e3,		// COPY
    0x49e3,		// COR
    0x2c9e3,		// CORE
    0x749e3,		// CORN
    0xa45749e3L,		// CORNER, seed word (533D: 6)
    0x51e3,		// COT
    0x8042a643L,		// CRIED, seed word (533D: 5)
    0x6643,		// CRY
    0x12a3,		// CUD
    0x2024,		// DAH
    0x8051b824L,		// DANCE, seed word (533D: 5)
    0x9c024,		// DAPS
    0x2c824,		// DARE
    0x7d024,		// DATO
    0x6424,		// DAY
    0x604a4,		// DEAL
    0x904a4,		// DEAR
    0x808a04a4L,		// DEATH, seed word (533D: 5)
    0x814a4,		// DEEP
    0x13814a4,		// DEEPS
    0x914a4,		// DEER
    0x498a4,		// DEFI
    0x30a4,		// DEL
    0x8190b0a4L,		// DELAY, seed word (533D: 5)
    0x630a4,		// DELL
    0x38a4,		// DEN
    0x140c8a4,		// DERAT
    0x2c8a4,		// DERE
    0x9c74cca4L,		// DESIGN, seed word (533D: 6)
    0x8b24cca4L,		// DESIRE, seed word (533D: 6)
    0x5cca4,		// DESK
    0x64a4,		// DEY
    0x21524,		// DIED
    0x8a44d924L,		// DIVIDE, seed word (533D: 6)
    0x1e4,		// DO
    0xa4fa0de4L,		// DOCTOR, seed word (533D: 6)
    0x80001de4L,		// DOG, seed word (533D: 3)
    0x93de4,		// DOOR
    0x51e4,		// DOT
    0x814155e4L,		// DOUBT, seed word (533D: 5)
    0x80e2e9e4L,		// DOZEN, seed word (533D: 5)
    0xb8644,		// DRAW
    0x80ebbe44L,		// DROWN, seed word (533D: 5)
    0x3baa4,		// DUNG
    0x8ee4caa4L,		// DURING, seed word (533D: 6)
    0x40c25,		// EACH
    0x4825,		// EAR
    0x81964825L,		// EARLY, seed word (533D: 5)
    0x808a4825L,		// EARTH, seed word (533D: 5)
    0x5025,		// EAT
    0x29c85,		// EDGE
    0x2085,		// EDH
    0x9a085,		// EDHS
    0x81441d25L,		// EIGHT, seed word (533D: 5)
    0x70585,		// ELAN
    0x1185,		// ELD
    0x81419585L,		// ELECT, seed word (533D: 5)
    0x4d85,		// ELS
    0x2cd85,		// ELSE
    0x8b24c1a5L,		// EMPIRE, seed word (533D: 6)
    0x11c5,		// END
    0x2934b1c5,		// ENLIST
    0x4dc5,		// ENS
    0x8b24d1c5L,		// ENTIRE, seed word (533D: 6)
    0x39e5,		// EON
    0xa205,		// EPHA
    0x130a205,		// EPHAS
    0x645,		// ERA
    0x98645,		// ERAS
    0x1645,		// ERE
    0x3a45,		// ERN
    0x2ba45,		// ERNE
    0x4e45,		// ERS
    0xa4e45,		// ERST
    0x8b008e65L,		// ESCAPE, seed word (533D: 6)
    0x4e65,		// ESS
    0x685,		// ETA
    0x2285,		// ETH
    0x122a285,		// ETHER
    0x4d685,		// ETUI
    0x16c5,		// EVE
    0x716c5,		// EVEN
    0x13716c5,		// EVENS
    0x814716c5L,		// EVENT, seed word (533D: 5)
    0x916c5,		// EVER
    0x819916c5L,		// EVERY, seed word (533D: 5)
    0x80195305L,		// EXTRA, seed word (533D: 5)
    0x92426,		// FAIR
    0x8127d826L,		// FAVOR, seed word (533D: 5)
    0x48a6,		// FER
    0x2c8a6,		// FERE
    0x1126,		// FID
    0x80461526L,		// FIELD, seed word (533D: 5)
    0x81441d26L,		// FIGHT, seed word (533D: 5)
    0x8b2a9d26L,		// FIGURE, seed word (533D: 6)
    0x2b126,		// FILE
    0x42b126,		// FILED
    0x63126,		// FILL
    0x88563126L,		// FILLED, seed word (533D: 6)
    0x9134b926L,		// FINISH, seed word (533D: 6)
    0x2c926,		// FIRE
    0x44d26,		// FISH
    0x5126,		// FIT
    0x8127bd86L,		// FLOOR, seed word (533D: 5)
    0x725e6,		// FOIN
    0x39e6,		// FON
    0x49e6,		// FOR
    0x8051c9e6L,		// FORCE, seed word (533D: 5)
    0x2c9e6,		// FORE
    0xa932c9e6L,		// FOREST, seed word (533D: 6)
    0x6c9e6,		// FORM
    0x819a49e6L,		// FORTY, seed word (533D: 5)
    0x245a4de6,		// FOSTER
    0x955e6,		// FOUR
    0x914955e6L,		// FOURTH, seed word (533D: 6)
    0x3e46,		// FRO
    0x81473e46L,		// FRONT, seed word (533D: 5)
    0x8144d646L,		// FRUIT, seed word (533D: 5)
    0x4aa6,		// FUR
    0x214a7,		// GEED
    0x30a7,		// GEL
    0xab8a7,		// GENU
    0x9a507,		// GHIS
    0x1527,		// GIE
    0x3927,		// GIN
    0x55c7,		// GNU
    0x1e7,		// GO
    0x11e7,		// GOD
    0x65e7,		// GOY
    0x814996a7L,		// GUEST, seed word (533D: 5)
    0x3aa7,		// GUN
    0x52a7,		// GUT
    0x1028,		// HAD
    0x63028,		// HALL
    0x1363028,		// HALLS
    0x5b828,		// HANK
    0x84828,		// HARP
    0x1384828,		// HARPS
    0x4c28,		// HAS
    0x5028,		// HAT
    0x42d028,		// HATED
    0x904a8,		// HEAR
    0x804904a8L,		// HEARD, seed word (533D: 5)
    0x814904a8L,		// HEART, seed word (533D: 5)
    0x924a8,		// HEIR
    0x48a8,		// HER
    0x4ca8,		// HES
    0x50a8,		// HET
    0x58d28,		// HICK
    0x5128,		// HIT
    0x1e8,		// HO
    0x15e8,		// HOE
    0x995e8,		// HOES
    0x2b1e8,		// HOLE
    0x8059c9e8L,		// HORSE, seed word (533D: 5)
    0x2cde8,		// HOSE
    0x51e8,		// HOT
    0x955e8,		// HOUR
    0x813955e8L,		// HOURS, seed word (533D: 5)
    0x8059d5e8L,		// HOUSE, seed word (533D: 5)
    0x5de8,		// HOW
    0x80e0b6a8L,		// HUMAN, seed word (533D: 5)
    0x3aa8,		// HUN
    0x1469,		// ICE
    0x8ad78dc9L,		// INCOME, seed word (533D: 6)
    0x799c9,		// INFO
    0x9b2799c9L,		// INFORM, seed word (533D: 6)
    0x2dc9,		// INK
    0x4dc9,		// INS
    0x8a44cdc9L,		// INSIDE, seed word (533D: 6)
    0x21649,		// IRED
    0x269,		// IS
    0x88e0b269L,		// ISLAND, seed word (533D: 6)
    0x2b269,		// ISLE
    0x805ace69L,		// ISSUE, seed word (533D: 5)
    0x289,		// IT
    0x4e89,		// ITS
    0x4ab,		// KEA
    0x392b,		// KIN
    0xbbdcb,		// KNOW
    0x80ebbdcbL,		// KNOWN, seed word (533D: 5)
    0x2902c,		// LADE
    0x9b42c,		// LAMS
    0x2382c,		// LAND
    0x482c,		// LAR
    0xa4c2c,		// LAST
    0x502c,		// LAT
    0x2d02c,		// LATE
    0x8122d02cL,		// LATER, seed word (533D: 5)
    0x9d02c,		// LATS
    0x5c2c,		// LAW
    0x642c,		// LAY
    0x704ac,		// LEAN
    0x904ac,		// LEAR
    0x80e904acL,		// LEARN, seed word (533D: 5)
    0x984ac,		// LEAS
    0x814984acL,		// LEAST, seed word (533D: 5)
    0x10ac,		// LED
    0x805390acL,		// LEDGE, seed word (533D: 5)
    0x994ac,		// LEES
    0xa14ac,		// LEET
    0x13a14ac,		// LEETS
    0x1cac,		// LEG
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
    0x2ad2c,		// LIKE
    0x2b92c,		// LINE
    0x2c92c,		// LIRE
    0xa4d2c,		// LIST
    0x9c5a4d2cL,		// LISTEN, seed word (533D: 6)
    0x512c,		// LIT
    0x9d12c,		// LITS
    0x289ec,		// LOBE
    0x80c08decL,		// LOCAL, seed word (533D: 5)
    0x78dec,		// LOCO
    0x3b9ec,		// LONG
    0x2cdec,		// LOSE
    0x255ec,		// LOUD
    0x2d9ec,		// LOVE
    0x132d9ec,		// LOVES
    0x5dec,		// LOW
    0x172c,		// LYE
    0x2cb2c,		// LYRE
    0x7242d,		// MAIN
    0x382d,		// MAN
    0x402d,		// MAP
    0x2c82d,		// MARE
    0xa4c2d,		// MAST
    0x245a4c2d,		// MASTER
    0x9d02d,		// MATS
    0x642d,		// MAY
    0x8127e42dL,		// MAYOR, seed word (533D: 5)
    0x704ad,		// MEAN
    0x814704adL,		// MEANT, seed word (533D: 5)
    0xa04ad,		// MEAT
    0x30ad,		// MEL
    0xa45134adL,		// MEMBER, seed word (533D: 6)
    0x38ad,		// MEN
    0xbbcad,		// MEOW
    0x2c8ad,		// MERE
    0x50ad,		// MET
    0xd0ad,		// META
    0x80c0d0adL,		// METAL, seed word (533D: 5)
    0x8ac2112dL,		// MIDDLE, seed word (533D: 6)
    0x2b12d,		// MILE
    0xb1ed,		// MOLA
    0x7b9ed,		// MONO
    0x73ded,		// MOON
    0x49ed,		// MOR
    0x190c9ed,		// MORAY
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
    0x5b48ae,		// NERVE
    0xa4cae,		// NEST
    0x50ae,		// NET
    0x9d0ae,		// NETS
    0x2d8ae,		// NEVE
    0x8122d8aeL,		// NEVER, seed word (533D: 5)
    0x132d8ae,		// NEVES
    0x92e,		// NIB
    0x28d2e,		// NICE
    0x2912e,		// NIDE
    0x4cd2e,		// NISI
    0x512e,		// NIT
    0x1ee,		// NO
    0x615ee,		// NOEL
    0x49ee,		// NOR
    0x808a49eeL,		// NORTH, seed word (533D: 5)
    0x51ee,		// NOT
    0x8a34d1eeL,		// NOTICE, seed word (533D: 6)
    0x80005deeL,		// NOW, seed word (533D: 3)
    0x228e,		// NTH
    0xaae,		// NUB
    0x292ae,		// NUDE
    0x12292ae,		// NUDER
    0x9c90d04fL,		// OBTAIN, seed word (533D: 6)
    0x9846f,		// OCAS
    0xb30a8c6fL,		// OCCUPY, seed word (533D: 6)
    0x8f,		// OD
    0x93c8f,		// ODOR
    0x812298cfL,		// OFFER, seed word (533D: 5)
    0x80e2d0cfL,		// OFTEN, seed word (533D: 5)
    0x10f,		// OH
    0x4d0f,		// OHS
    0x9958f,		// OLES
    0xb18f,		// OLLA
    0x915af,		// OMER
    0xa25af,		// OMIT
    0x1cf,		// ON
    0x28dcf,		// ONCE
    0x15cf,		// ONE
    0x8122924fL,		// ORDER, seed word (533D: 5)
    0x7924f,		// ORDO
    0x9964f,		// ORES
    0x8122a28fL,		// OTHER, seed word (533D: 5)
    0x40eaf,		// OUCH
    0x52af,		// OUT
    0x2ef,		// OW
    0x3aef,		// OWN
    0x49030,		// PADI
    0x22430,		// PAID
    0x92430,		// PAIR
    0x3430,		// PAM
    0x4030,		// PAP
    0x8122c030L,		// PAPER, seed word (533D: 5)
    0x4830,		// PAR
    0x2c830,		// PARE
    0xa4830,		// PART
    0x819a4830L,		// PARTY, seed word (533D: 5)
    0x9cc30,		// PASS
    0x8859cc30L,		// PASSED, seed word (533D: 6)
    0x6430,		// PAY
    0x265184b0,		// PEACES
    0x13290b0,		// PEDES
    0x214b0,		// PEED
    0x614b0,		// PEEL
    0x994b0,		// PEES
    0x2b0b0,		// PELE
    0x8ac83cb0L,		// PEOPLE, seed word (533D: 6)
    0x7c0b0,		// PEPO
    0x48b0,		// PER
    0x4cb0,		// PES
    0x91530,		// PIER
    0x2b1f0,		// POLE
    0x2c1f0,		// POPE
    0x81399650L,		// PRESS, seed word (533D: 5)
    0x44eb0,		// PUSH
    0x88544eb0L,		// PUSHED, seed word (533D: 6)
    0x8142a6b1L,		// QUIET, seed word (533D: 5)
    0x5a26b1,		// QUITE
    0x58c32,		// RACK
    0x1032,		// RAD
    0x72432,		// RAIN
    0x8059a432L,		// RAISE, seed word (533D: 5)
    0x2b032,		// RALE
    0x3832,		// RAN
    0x4b832,		// RANI
    0x4032,		// RAP
    0x8044c032L,		// RAPID, seed word (533D: 5)
    0x2c832,		// RARE
    0x2932c832,		// RAREST
    0x245a4c32,		// RASTER
    0x5032,		// RAT
    0x45032,		// RATH
    0x545032,		// RATHE
    0x9d032,		// RATS
    0x5c32,		// RAW
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
    0x14b2,		// REE
    0x214b2,		// REED
    0x314b2,		// REEF
    0x18b2,		// REF
    0xa18b2,		// REFT
    0x324b2,		// REIF
    0x724b2,		// REIN
    0x9a4b2,		// REIS
    0x8c54b0b2L,		// RELIEF, seed word (533D: 6)
    0xcb0b2,		// RELY
    0x9c90b4b2L,		// REMAIN, seed word (533D: 6)
    0xa38b2,		// RENT
    0x40b2,		// REP
    0xa490c0b2L,		// REPAIR, seed word (533D: 6)
    0x819640b2L,		// REPLY, seed word (533D: 5)
    0x9c0b2,		// REPS
    0x4cb2,		// RES
    0xa44ccb2,		// RESIDE
    0x2934ccb2,		// RESIST
    0xa4cb2,		// REST
    0xa8caccb2L,		// RESULT, seed word (533D: 6)
    0x50b2,		// RET
    0x2d0b2,		// RETE
    0x2932d0b2,		// RETEST
    0xae4d0b2,		// RETINE
    0x8b24d0b2L,		// RETIRE, seed word (533D: 6)
    0x9d0b2,		// RETS
    0x9d2ad0b2L,		// RETURN, seed word (533D: 6)
    0x532,		// RIA
    0x1132,		// RID
    0x29132,		// RIDE
    0x3b932,		// RING
    0x9c132,		// RIPS
    0xdf2,		// ROC
    0x291f2,		// RODE
    0x995f2,		// ROES
    0x35f2,		// ROM
    0x23df2,		// ROOD
    0x2cdf2,		// ROSE
    0x51f2,		// ROT
    0x2d1f2,		// ROTE
    0x5df2,		// ROW
    0x80c0e5f2L,		// ROYAL, seed word (533D: 5)
    0x3ab2,		// RUN
    0x452b2,		// RUTH
    0x1732,		// RYE
    0x2b033,		// SALE
    0x5033,		// SAT
    0x80005c33L,		// SAW, seed word (533D: 3)
    0x1471473,		// SCENT
    0xcb3,		// SEC
    0x88e78cb3L,		// SECOND, seed word (533D: 6)
    0xa0cb3,		// SECT
    0x14b3,		// SEE
    0x614b3,		// SEEL
    0x714b3,		// SEEN
    0x814b3,		// SEEP
    0x30b3,		// SEL
    0xa832b0b3L,		// SELECT, seed word (533D: 6)
    0x38b3,		// SEN
    0x238b3,		// SEND
    0x2b8b3,		// SENE
    0x8059b8b3L,		// SENSE, seed word (533D: 5)
    0x48b3,		// SER
    0x805b48b3L,		// SERVE, seed word (533D: 5)
    0x50b3,		// SET
    0x80e2d8b3L,		// SEVEN, seed word (533D: 5)
    0x122d8b3,		// SEVER
    0x5cb3,		// SEW
    0x513,		// SHA
    0x80c60513L,		// SHALL, seed word (533D: 5)
    0x80580513L,		// SHAPE, seed word (533D: 5)
    0x81090513L,		// SHARP, seed word (533D: 5)
    0x21513,		// SHED
    0x72513,		// SHIN
    0xbbd13,		// SHOW
    0x80ebbd13L,		// SHOWN, seed word (533D: 5)
    0x133,		// SI
    0x60533,		// SIAL
    0x29133,		// SIDE
    0x41d33,		// SIGH
    0x81441d33L,		// SIGHT, seed word (533D: 5)
    0x71d33,		// SIGN
    0x8571d33,		// SIGNED
    0x3933,		// SIN
    0x8051b933L,		// SINCE, seed word (533D: 5)
    0x3b933,		// SING
    0x853b933,		// SINGED
    0x8ac3b933L,		// SINGLE, seed word (533D: 6)
    0x2c933,		// SIRE
    0xa45a4d33L,		// SISTER, seed word (533D: 6)
    0x80005133L,		// SIT, seed word (533D: 3)
    0x9d133,		// SITS
    0x573,		// SKA
    0x1429593,		// SLEET
    0x80c605b3L,		// SMALL, seed word (533D: 5)
    0x31f3,		// SOL
    0x805b31f3L,		// SOLVE, seed word (533D: 5)
    0x51f3,		// SOT
    0x5df3,		// SOW
    0x24520613,		// SPADER
    0x80429613L,		// SPEED, seed word (533D: 5)
    0x8812ca13L,		// SPREAD, seed word (533D: 6)
    0x8ee4ca13L,		// SPRING, seed word (533D: 6)
    0x81068693L,		// STAMP, seed word (533D: 5)
    0x80470693L,		// STAND, seed word (533D: 5)
    0x90693,		// STAR
    0x24590693,		// STARER
    0x81490693L,		// START, seed word (533D: 5)
    0x80c29693L,		// STEEL, seed word (533D: 5)
    0x561693,		// STELE
    0xa1693,		// STET
    0x80b1a693L,		// STICK, seed word (533D: 5)
    0x80c62693L,		// STILL, seed word (533D: 5)
    0x92693,		// STIR
    0x1816be93,		// STOMAL
    0x80573e93L,		// STONE, seed word (533D: 5)
    0x9a12ca93L,		// STREAM, seed word (533D: 6)
    0xa852ca93L,		// STREET, seed word (533D: 6)
    0x8ee4ca93L,		// STRING, seed word (533D: 6)
    0x16b3,		// SUE
    0x916b3,		// SUER
    0xa4531ab3L,		// SUFFER, seed word (533D: 6)
    0xa456b6b3L,		// SUMMER, seed word (533D: 6)
    0x2cab3,		// SURE
    0x245652b3,		// SUTLER
    0x493ef3,		// SWORD
    0x34,		// TA
    0x80560834L,		// TABLE, seed word (533D: 5)
    0x40c34,		// TACH
    0x1434,		// TAE
    0x72434,		// TAIN
    0x2b434,		// TAME
    0x3834,		// TAN
    0x4834,		// TAR
    0x42c834,		// TARED
    0x13a4834,		// TARTS
    0x4c34,		// TAS
    0x4b4,		// TEA
    0x808184b4L,		// TEACH, seed word (533D: 5)
    0x604b4,		// TEAL
    0x10b4,		// TED
    0x614b4,		// TEEL
    0x13614b4,		// TEELS
    0x30b4,		// TEL
    0xb0b4,		// TELA
    0x2b0b4,		// TELE
    0x132b0b4,		// TELES
    0x38b4,		// TEN
    0x6c8b4,		// TERM
    0x8136c8b4L,		// TERMS, seed word (533D: 5)
    0xa4cb4,		// TEST
    0x245a4cb4,		// TESTER
    0x9d0b4,		// TETS
    0x5cb4,		// TEW
    0x80b70514L,		// THANK, seed word (533D: 5)
    0x1514,		// THE
    0x81249514L,		// THEIR, seed word (533D: 5)
    0x591514,		// THERE
    0x80599514L,		// THESE, seed word (533D: 5)
    0x80b1a514L,		// THICK, seed word (533D: 5)
    0x72514,		// THIN
    0x80772514L,		// THING, seed word (533D: 5)
    0x80b72514L,		// THINK, seed word (533D: 5)
    0x3d14,		// THO
    0xe93d14,		// THORN
    0x8059bd14L,		// THOSE, seed word (533D: 5)
    0x8052c914L,		// THREE, seed word (533D: 5)
    0x7c914,		// THRO
    0x177c914,		// THROW
    0xac914,		// THRU
    0x134,		// TI
    0x58d34,		// TICK
    0x1358d34,		// TICKS
    0x91534,		// TIER
    0x3134,		// TIL
    0x63134,		// TILL
    0x1363134,		// TILLS
    0x3934,		// TIN
    0x2b934,		// TINE
    0x2c934,		// TIRE
    0x4d34,		// TIS
    0x205f4,		// TOAD
    0x19205f4,		// TOADY
    0x11f4,		// TOD
    0x819091f4L,		// TODAY, seed word (533D: 5)
    0x39f4,		// TON
    0x2b9f4,		// TONE
    0x132b9f4,		// TONES
    0x49f4,		// TOR
    0x8081d5f4L,		// TOUCH, seed word (533D: 5)
    0x8920ddf4L,		// TOWARD, seed word (533D: 6)
    0x65f4,		// TOY
    0x80b18654L,		// TRACK, seed word (533D: 5)
    0x80520654L,		// TRADE, seed word (533D: 5)
    0x80e48654L,		// TRAIN, seed word (533D: 5)
    0x80654,		// TRAP
    0x6654,		// TRY
    0x90674,		// TSAR
    0xab4,		// TUB
    0x1eb4,		// TUG
    0x74ab4,		// TURN
    0x88574ab4L,		// TURNED, seed word (533D: 6)
    0x24574ab4,		// TURNER
    0x7cb34,		// TYRO
    0x8ac105d5L,		// UNABLE, seed word (533D: 6)
    0x291d5,		// UNDE
    0x812291d5L,		// UNDER, seed word (533D: 5)
    0xa25d5,		// UNIT
    0xa732b1d5L,		// UNLESS, seed word (533D: 6)
    0x80c4d1d5L,		// UNTIL, seed word (533D: 5)
    0x1675,		// USE
    0x91675,		// USER
    0x14b6,		// VEE
    0x914b6,		// VEER
    0x13914b6,		// VEERS
    0xcc8b6,		// VERY
    0x9859ccb6L,		// VESSEL, seed word (533D: 6)
    0x50b6,		// VET
    0x29136,		// VIDE
    0x21536,		// VIED
    0x8144cd36L,		// VISIT, seed word (533D: 5)
    0x15f6,		// VOE
    0x8051a5f6L,		// VOICE, seed word (533D: 5)
    0x4837,		// WAR
    0x24837,		// WARD
    0x4c37,		// WAS
    0x8122d037L,		// WATER, seed word (533D: 5)
    0x50b7,		// WET
    0x80003d17L,		// WHO, seed word (533D: 3)
    0x80563d17L,		// WHOLE, seed word (533D: 5)
    0x8059bd17L,		// WHOSE, seed word (533D: 5)
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
    0x813249f7L,		// WORDS, seed word (533D: 5)
    0x808a49f7L,		// WORTH, seed word (533D: 5)
    0x4df7,		// WOS
    0x80773e57L,		// WRONG, seed word (533D: 5)
    0x805a3e57L,		// WROTE, seed word (533D: 5)
    0x8a3e57,		// WROTH
    0x3439,		// YAM
    0x4039,		// YAP
    0x807755f9L,		// YOUNG, seed word (533D: 5)
    0x2b9fa,		// ZONE
    0x42b9fa,		// ZONED



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
