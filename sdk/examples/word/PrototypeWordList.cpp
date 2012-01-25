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

// compressed 5 bit letters
static const uint32_t sList[] =
{
    0x41,		// AB
    0xa1441,		// ABET
    0x8002b041L,		// ABLE, seed word (533D: 4)
    0x3c41,		// ABO
    0xa14abc41L,		// ABOUT, seed word (533D: 5)
    0xa5441,		// ABUT
    0x6441,		// ABY
    0x2e441,		// ABYE
    0x1461,		// ACE
    0x99461,		// ACES
    0x2a061,		// ACHE
    0x2b861,		// ACNE
    0x2c861,		// ACRE
    0x5061,		// ACT
    0x9d061,		// ACTS
    0x81,		// AD
    0x3c81,		// ADO
    0x4c81,		// ADS
    0xa1,		// AE
    0x73ca1,		// AEON
    0xcc8a1,		// AERY
    0xe1,		// AG
    0x600014e1,		// AGE, seed word (533D: 3)
    0x914e1,		// AGER
    0x60003ce1,		// AGO, seed word (533D: 3)
    0x73ce1,		// AGON
    0x101,		// AH
    0x121,		// AI
    0x1121,		// AID
    0x99121,		// AIDS
    0x3121,		// AIL
    0x9b121,		// AILS
    0x3521,		// AIM
    0x3921,		// AIN
    0x60004921,		// AIR, seed word (533D: 3)
    0x74921,		// AIRN
    0xa4921,		// AIRT
    0x4d21,		// AIS
    0x5121,		// AIT
    0x181,		// AL
    0x981,		// ALB
    0x1581,		// ALE
    0x19581,		// ALEC
    0x1319581,		// ALECS
    0x1491581,		// ALERT
    0x99581,		// ALES
    0x32581,		// ALIF
    0xa2581,		// ALIT
    0x2b581,		// ALME
    0x2bd81,		// ALOE
    0xa0573d81L,		// ALONE, seed word (533D: 5)
    0x4181,		// ALP
    0x4d81,		// ALS
    0x5181,		// ALT
    0x122d181,		// ALTER
    0x9d181,		// ALTS
    0x400001a1,		// AM, seed word (533D: 2)
    0x715a1,		// AMEN
    0x14715a1,		// AMENT
    0x25a1,		// AMI
    0x725a1,		// AMIN
    0xa0773da1L,		// AMONG, seed word (533D: 5)
    0x41a1,		// AMP
    0x9c1a1,		// AMPS
    0x400001c1,		// AN, seed word (533D: 2)
    0x600011c1,		// AND, seed word (533D: 3)
    0x15c1,		// ANE
    0xc29dc1,		// ANGEL
    0xa0561dc1L,		// ANGLE, seed word (533D: 5)
    0x25c1,		// ANI
    0x625c1,		// ANIL
    0x563dc1,		// ANOLE
    0x51c1,		// ANT
    0x2d1c1,		// ANTE
    0x4d1c1,		// ANTI
    0x600065c1,		// ANY, seed word (533D: 3)
    0x1601,		// APE
    0x99601,		// APES
    0xa0c4ca01L,		// APRIL, seed word (533D: 5)
    0x2ce01,		// APSE
    0x5201,		// APT
    0x241,		// AR
    0xe41,		// ARC
    0x40e41,		// ARCH
    0x60001641,		// ARE, seed word (533D: 3)
    0x1a41,		// ARF
    0x561e41,		// ARGLE
    0x22641,		// ARID
    0x62641,		// ARIL
    0x3641,		// ARM
    0x800cb641L,		// ARMY, seed word (533D: 4)
    0x4e41,		// ARS
    0x60005241,		// ART, seed word (533D: 3)
    0xc2d241,		// ARTEL
    0xcd241,		// ARTY
    0x66641,		// ARYL
    0x261,		// AS
    0x1478e61,		// ASCOT
    0x2261,		// ASH
    0x60002e61,		// ASK, seed word (533D: 3)
    0x4261,		// ASP
    0x40000281,		// AT, seed word (533D: 2)
    0x1681,		// ATE
    0x99681,		// ATES
    0x800a3aa1L,		// AUNT, seed word (533D: 4)
    0x7d2a1,		// AUTO
    0x2e1,		// AW
    0x16e1,		// AWE
    0x32e1,		// AWL
    0x3ae1,		// AWN
    0x301,		// AX
    0x1701,		// AXE
    0x321,		// AY
    0x1721,		// AYE
    0x99721,		// AYES
    0x4f21,		// AYS
    0x22,		// BA
    0x60001022,		// BAD, seed word (533D: 3)
    0x3022,		// BAL
    0x2b022,		// BALE
    0x3422,		// BAM
    0x5022,		// BAT
    0x2d022,		// BATE
    0x6422,		// BAY
    0xa2,		// BE
    0x684a2,		// BEAM
    0x19684a2,		// BEAMY
    0x800a04a2L,		// BEAT, seed word (533D: 4)
    0x600010a2,		// BED, seed word (533D: 3)
    0x30a2,		// BEL
    0xa30a2,		// BELT
    0xb4a2,		// BEMA
    0x50a2,		// BET
    0xd0a2,		// BETA
    0x64a2,		// BEY
    0x122,		// BI
    0x60005122,		// BIT, seed word (533D: 3)
    0x28582,		// BLAE
    0xa0582,		// BLAT
    0x5a0582,		// BLATE
    0x1409582,		// BLEAT
    0xa1582,		// BLET
    0x1e2,		// BO
    0x5e2,		// BOA
    0x800a05e2L,		// BOAT, seed word (533D: 4)
    0x51e2,		// BOT
    0xd1e2,		// BOTA
    0xa55e2,		// BOUT
    0x600065e2,		// BOY, seed word (533D: 3)
    0x52a2,		// BUT
    0x322,		// BY
    0x1722,		// BYE
    0x3823,		// CAN
    0x2b823,		// CANE
    0x4023,		// CAP
    0x2c023,		// CAPE
    0x132c023,		// CAPES
    0x9c023,		// CAPS
    0x4823,		// CAR
    0x2c823,		// CARE
    0x64823,		// CARL
    0x564823,		// CARLE
    0x2cc23,		// CASE
    0xa4c23,		// CAST
    0x60005023,		// CAT, seed word (533D: 3)
    0x2d023,		// CATE
    0x9d023,		// CATS
    0x30a3,		// CEL
    0x9b0a3,		// CELS
    0x40a3,		// CEP
    0x9c0a3,		// CEPS
    0x7c8a3,		// CERO
    0x137c8a3,		// CEROS
    0x90503,		// CHAR
    0x590503,		// CHARE
    0xa0503,		// CHAT
    0x1409503,		// CHEAT
    0x70583,		// CLAN
    0x80583,		// CLAP
    0xa0e09583L,		// CLEAN, seed word (533D: 5)
    0xa1209583L,		// CLEAR, seed word (533D: 5)
    0xa059bd83L,		// CLOSE, seed word (533D: 5)
    0xa14985e3L,		// COAST, seed word (533D: 5)
    0xa05e3,		// COAT
    0x13a05e3,		// COATS
    0x31e3,		// COL
    0x2b1e3,		// COLE
    0x132b1e3,		// COLES
    0x9b1e3,		// COLS
    0x49e3,		// COR
    0x2c9e3,		// CORE
    0x132c9e3,		// CORES
    0x59c9e3,		// CORSE
    0x4de3,		// COS
    0xa4de3,		// COST
    0x1a4de3,		// COSTA
    0x51e3,		// COT
    0x9d1e3,		// COTS
    0x824,		// DAB
    0x2024,		// DAH
    0x9a024,		// DAHS
    0x9a424,		// DAIS
    0x3024,		// DAL
    0x2b024,		// DALE
    0x3424,		// DAM
    0x2b424,		// DAME
    0x4024,		// DAP
    0x2c824,		// DARE
    0xa4824,		// DART
    0x80044c24L,		// DASH, seed word (533D: 4)
    0x8002d024L,		// DATE, seed word (533D: 4)
    0x122d024,		// DATER
    0x7d024,		// DATO
    0x60006424,		// DAY, seed word (533D: 3)
    0xa4,		// DE
    0x604a4,		// DEAL
    0x904a4,		// DEAR
    0x19904a4,		// DEARY
    0x8a4,		// DEB
    0x498a4,		// DEFI
    0x624a4,		// DEIL
    0x13624a4,		// DEILS
    0x30a4,		// DEL
    0xa190b0a4L,		// DELAY, seed word (533D: 5)
    0x330a4,		// DELF
    0x4b0a4,		// DELI
    0x134b0a4,		// DELIS
    0x9b0a4,		// DELS
    0x38a4,		// DEN
    0x140c8a4,		// DERAT
    0x190c8a4,		// DERAY
    0x64a4,		// DEY
    0x1524,		// DIE
    0x61524,		// DIEL
    0x99524,		// DIES
    0xa1524,		// DIET
    0x4124,		// DIP
    0x4d24,		// DIS
    0x5124,		// DIT
    0x2d124,		// DITE
    0x400001e4,		// DO, seed word (533D: 2)
    0xa05e4,		// DOAT
    0x15e4,		// DOE
    0x800995e4L,		// DOES, seed word (533D: 4)
    0x60001de4,		// DOG, seed word (533D: 3)
    0x31e4,		// DOL
    0x39e4,		// DON
    0x8002b9e4L,		// DONE, seed word (533D: 4)
    0x9b9e4,		// DONS
    0x49e4,		// DOR
    0x4de4,		// DOS
    0x2cde4,		// DOSE
    0x51e4,		// DOT
    0xcd1e4,		// DOTY
    0x955e4,		// DOUR
    0xa0644,		// DRAT
    0xc8644,		// DRAY
    0x82644,		// DRIP
    0x6644,		// DRY
    0x16a4,		// DUE
    0x3aa4,		// DUN
    0x2baa4,		// DUNE
    0x9baa4,		// DUNS
    0x3ea4,		// DUO
    0x9bea4,		// DUOS
    0x2caa4,		// DURE
    0x74aa4,		// DURN
    0x7caa4,		// DURO
    0x1724,		// DYE
    0x91724,		// DYER
    0x40c25,		// EACH
    0x4825,		// EAR
    0x64825,		// EARL
    0xa1964825L,		// EARLY, seed word (533D: 5)
    0x74825,		// EARN
    0x8a4825,		// EARTH
    0xa4c25,		// EAST
    0x800ccc25L,		// EASY, seed word (533D: 4)
    0x60005025,		// EAT, seed word (533D: 3)
    0x45025,		// EATH
    0x9d025,		// EATS
    0x85,		// ED
    0x2085,		// EDH
    0xa2485,		// EDIT
    0x30a5,		// EEL
    0x9b0a5,		// EELS
    0xc5,		// EF
    0x50c5,		// EFT
    0x604e5,		// EGAL
    0x105,		// EH
    0x185,		// EL
    0x70585,		// ELAN
    0x1185,		// ELD
    0x99185,		// ELDS
    0x1985,		// ELF
    0x3585,		// ELM
    0x4d85,		// ELS
    0x2cd85,		// ELSE
    0x1a5,		// EM
    0x19089a5,		// EMBAY
    0x4da5,		// EMS
    0x1c5,		// EN
    0x600011c5,		// END, seed word (533D: 3)
    0x1dc5,		// ENG
    0x63dc5,		// ENOL
    0xbbdc5,		// ENOW
    0x4dc5,		// ENS
    0x39e5,		// EON
    0x9b9e5,		// EONS
    0xa205,		// EPHA
    0x130a205,		// EPHAS
    0x245,		// ER
    0x645,		// ERA
    0x1e45,		// ERG
    0x3a45,		// ERN
    0x9be45,		// EROS
    0x4e45,		// ERS
    0xa4e45,		// ERST
    0x265,		// ES
    0x285,		// ET
    0x685,		// ETA
    0x98685,		// ETAS
    0x40e85,		// ETCH
    0x2285,		// ETH
    0x137a285,		// ETHOS
    0x9a285,		// ETHS
    0xba85,		// ETNA
    0x305,		// EX
    0xa0195305L,		// EXTRA, seed word (533D: 5)
    0x98725,		// EYAS
    0xcb25,		// EYRA
    0x26,		// FA
    0x62426,		// FAIL
    0x72426,		// FAIN
    0x3826,		// FAN
    0x60004826,		// FAR, seed word (533D: 3)
    0x2c826,		// FARE
    0x800904a6L,		// FEAR, seed word (533D: 4)
    0x10a6,		// FED
    0x44b0a6,		// FELID
    0x38a6,		// FEN
    0x48a6,		// FER
    0x50a6,		// FET
    0x1126,		// FID
    0x1526,		// FIE
    0xa0461526L,		// FIELD, seed word (533D: 5)
    0x3126,		// FIL
    0xb126,		// FILA
    0x2b126,		// FILE
    0x42b126,		// FILED
    0x3926,		// FIN
    0xa0c0b926L,		// FINAL, seed word (533D: 5)
    0x60005126,		// FIT, seed word (533D: 3)
    0x70586,		// FLAN
    0x21586,		// FLED
    0x42a586,		// FLIED
    0x15e6,		// FOE
    0x39e6,		// FON
    0xa39e6,		// FONT
    0x600049e6,		// FOR, seed word (533D: 3)
    0x28646,		// FRAE
    0x3e46,		// FRO
    0x1427,		// GAE
    0x71427,		// GAEN
    0x3027,		// GAL
    0x2b027,		// GALE
    0x3427,		// GAM
    0x3827,		// GAN
    0x2b827,		// GANE
    0x4827,		// GAR
    0x5027,		// GAT
    0x2d027,		// GATE
    0x904a7,		// GEAR
    0x30a7,		// GEL
    0x38a7,		// GEN
    0x50a7,		// GET
    0xd0a7,		// GETA
    0x590587,		// GLARE
    0xe09587,		// GLEAN
    0x71587,		// GLEN
    0x905c7,		// GNAR
    0xa05c7,		// GNAT
    0x55c7,		// GNU
    0x1e7,		// GO
    0x5e7,		// GOA
    0x11e7,		// GOD
    0x600051e7,		// GOT, seed word (533D: 3)
    0x70647,		// GRAN
    0xa1470647L,		// GRANT, seed word (533D: 5)
    0xa0647,		// GRAT
    0x5a0647,		// GRATE
    0xa1409647L,		// GREAT, seed word (533D: 5)
    0x60003aa7,		// GUN, seed word (533D: 3)
    0x28,		// HA
    0x60001028,		// HAD, seed word (533D: 3)
    0x29028,		// HADE
    0x1428,		// HAE
    0x21428,		// HAED
    0x99428,		// HAES
    0xa1428,		// HAET
    0x4028,		// HAP
    0x9c028,		// HAPS
    0x24828,		// HARD
    0x2c828,		// HARE
    0x42c828,		// HARED
    0x84828,		// HARP
    0x1384828,		// HARPS
    0xa4828,		// HART
    0x60004c28,		// HAS, seed word (533D: 3)
    0x84c28,		// HASP
    0x60005028,		// HAT, seed word (533D: 3)
    0x2d028,		// HATE
    0x122d028,		// HATER
    0x5c28,		// HAW
    0x9dc28,		// HAWS
    0x400000a8,		// HE, seed word (533D: 2)
    0x800204a8L,		// HEAD, seed word (533D: 4)
    0x804a8,		// HEAP
    0x13804a8,		// HEAPS
    0x800904a8L,		// HEAR, seed word (533D: 4)
    0xa04904a8L,		// HEARD, seed word (533D: 5)
    0xa14904a8L,		// HEART, seed word (533D: 5)
    0x800a04a8L,		// HEAT, seed word (533D: 4)
    0x924a8,		// HEIR
    0x34a8,		// HEM
    0x40a8,		// HEP
    0x600048a8,		// HER, seed word (533D: 3)
    0x248a8,		// HERD
    0x7c8a8,		// HERO
    0x137c8a8,		// HEROS
    0x9c8a8,		// HERS
    0x4ca8,		// HES
    0xa4ca8,		// HEST
    0x50a8,		// HET
    0x9d0a8,		// HETS
    0x5ca8,		// HEW
    0x9dca8,		// HEWS
    0x128,		// HI
    0x1528,		// HIE
    0x60003528,		// HIM, seed word (533D: 3)
    0x4128,		// HIP
    0x9c128,		// HIPS
    0x2c928,		// HIRE
    0x60004d28,		// HIS, seed word (533D: 3)
    0x60005128,		// HIT, seed word (533D: 3)
    0x1a8,		// HM
    0x1e8,		// HO
    0x15e8,		// HOE
    0x915e8,		// HOER
    0x13915e8,		// HOERS
    0x995e8,		// HOES
    0x8002b5e8L,		// HOME, seed word (533D: 4)
    0x39e8,		// HON
    0x9b9e8,		// HONS
    0x41e8,		// HOP
    0x9c1e8,		// HOPS
    0xa059c9e8L,		// HORSE, seed word (533D: 5)
    0x149c9e8,		// HORST
    0x2cde8,		// HOSE
    0xa4de8,		// HOST
    0x600051e8,		// HOT, seed word (533D: 3)
    0x9d1e8,		// HOTS
    0xa059d5e8L,		// HOUSE, seed word (533D: 5)
    0x5de8,		// HOW
    0x2dde8,		// HOWE
    0x132dde8,		// HOWES
    0x9dde8,		// HOWS
    0x16a8,		// HUE
    0x996a8,		// HUES
    0x89,		// ID
    0x99489,		// IDES
    0x2b089,		// IDLE
    0x132b089,		// IDLES
    0x4c89,		// IDS
    0xc9,		// IF
    0x1c9,		// IN
    0x14805c9,		// INAPT
    0x1649,		// IRE
    0x99649,		// IRES
    0x40000269,		// IS, seed word (533D: 2)
    0x2b269,		// ISLE
    0x42b269,		// ISLED
    0x40000289,		// IT, seed word (533D: 2)
    0x122a289,		// ITHER
    0x4e89,		// ITS
    0x2b,		// KA
    0x142b,		// KAE
    0x9942b,		// KAES
    0x4c2b,		// KAS
    0x502b,		// KAT
    0x4ab,		// KEA
    0x984ab,		// KEAS
    0x40ab,		// KEP
    0x9c0ab,		// KEPS
    0x2c,		// LA
    0x82c,		// LAB
    0xc2c,		// LAC
    0x28c2c,		// LACE
    0x1228c2c,		// LACER
    0x1328c2c,		// LACES
    0x98c2c,		// LACS
    0x102c,		// LAD
    0x2902c,		// LADE
    0xc902c,		// LADY
    0x1c2c,		// LAG
    0x1229c2c,		// LAGER
    0x7242c,		// LAIN
    0x9242c,		// LAIR
    0x342c,		// LAM
    0x2b42c,		// LAME
    0x51b82c,		// LANCE
    0x2b82c,		// LANE
    0x3b82c,		// LANG
    0x402c,		// LAP
    0x482c,		// LAR
    0xa053c82cL,		// LARGE, seed word (533D: 5)
    0x4c82c,		// LARI
    0x4c2c,		// LAS
    0x2cc2c,		// LASE
    0x800a4c2cL,		// LAST, seed word (533D: 4)
    0x502c,		// LAT
    0x8002d02cL,		// LATE, seed word (533D: 4)
    0xa122d02cL,		// LATER, seed word (533D: 5)
    0x4d02c,		// LATI
    0x9d02c,		// LATS
    0x60005c2c,		// LAW, seed word (533D: 3)
    0x6000642c,		// LAY, seed word (533D: 3)
    0x42e42c,		// LAYED
    0x122e42c,		// LAYER
    0x4ac,		// LEA
    0x800204acL,		// LEAD, seed word (533D: 4)
    0x19204ac,		// LEADY
    0x704ac,		// LEAN
    0x804ac,		// LEAP
    0x904ac,		// LEAR
    0xa0e904acL,		// LEARN, seed word (533D: 5)
    0x19904ac,		// LEARY
    0x984ac,		// LEAS
    0xa14984acL,		// LEAST, seed word (533D: 5)
    0x600010ac,		// LED, seed word (533D: 3)
    0x14ac,		// LEE
    0x994ac,		// LEES
    0xa14ac,		// LEET
    0x13a14ac,		// LEETS
    0x1cac,		// LEG
    0x24ac,		// LEI
    0x9a4ac,		// LEIS
    0x7b8ac,		// LENO
    0xa4cac,		// LEST
    0x600050ac,		// LET, seed word (533D: 3)
    0x9d0ac,		// LETS
    0x64ac,		// LEY
    0x12c,		// LI
    0x9052c,		// LIAR
    0x112c,		// LID
    0x9912c,		// LIDS
    0x6000152c,		// LIE, seed word (533D: 3)
    0x2152c,		// LIED
    0x3152c,		// LIEF
    0x9952c,		// LIES
    0x2992c,		// LIFE
    0x392c,		// LIN
    0x412c,		// LIP
    0xc92c,		// LIRA
    0x4d2c,		// LIS
    0x800a4d2cL,		// LIST, seed word (533D: 4)
    0x512c,		// LIT
    0x9d12c,		// LITS
    0x1ec,		// LO
    0x705ec,		// LOAN
    0x2b9ec,		// LONE
    0x8002cdecL,		// LOSE, seed word (533D: 4)
    0xa4dec,		// LOST
    0x51ec,		// LOT
    0x9d1ec,		// LOTS
    0x60005dec,		// LOW, seed word (533D: 3)
    0x172c,		// LYE
    0x2cb2c,		// LYRE
    0x2d,		// MA
    0x2882d,		// MABE
    0x102d,		// MAD
    0x8002902dL,		// MADE, seed word (533D: 4)
    0x142d,		// MAE
    0x9942d,		// MAES
    0x1c2d,		// MAG
    0x8007242dL,		// MAIN, seed word (533D: 4)
    0x2b02d,		// MALE
    0xa302d,		// MALT
    0x6000382d,		// MAN, seed word (533D: 3)
    0x2b82d,		// MANE
    0xf3b82d,		// MANGO
    0x7b82d,		// MANO
    0x800cb82dL,		// MANY, seed word (533D: 4)
    0x6000402d,		// MAP, seed word (533D: 3)
    0x9c02d,		// MAPS
    0x482d,		// MAR
    0x4c2d,		// MAS
    0xa4c2d,		// MAST
    0x502d,		// MAT
    0x2d02d,		// MATE
    0x9d02d,		// MATS
    0x5c2d,		// MAW
    0x75c2d,		// MAWN
    0x6000642d,		// MAY, seed word (533D: 3)
    0xa051642dL,		// MAYBE, seed word (533D: 5)
    0x7e42d,		// MAYO
    0xa127e42dL,		// MAYOR, seed word (533D: 5)
    0x400000ad,		// ME, seed word (533D: 2)
    0x204ad,		// MEAD
    0x604ad,		// MEAL
    0x704ad,		// MEAN
    0xa14704adL,		// MEANT, seed word (533D: 5)
    0xa04ad,		// MEAT
    0x10ad,		// MED
    0x30ad,		// MEL
    0xa30ad,		// MELT
    0x600038ad,		// MEN, seed word (533D: 3)
    0x7b8ad,		// MENO
    0x1a38ad,		// MENTA
    0xbbcad,		// MEOW
    0xccad,		// MESA
    0x50ad,		// MET
    0xd0ad,		// META
    0xa0c0d0adL,		// METAL, seed word (533D: 5)
    0x5cad,		// MEW
    0x3d0d,		// MHO
    0x12d,		// MI
    0xb92d,		// MINA
    0x1ed,		// MO
    0x5ed,		// MOA
    0x705ed,		// MOAN
    0x1ded,		// MOG
    0x39ed,		// MON
    0xa192b9edL,		// MONEY, seed word (533D: 5)
    0xcb9ed,		// MONY
    0x49ed,		// MOR
    0xc9ed,		// MORA
    0x190c9ed,		// MORAY
    0x8002c9edL,		// MORE, seed word (533D: 4)
    0x5ded,		// MOW
    0x75ded,		// MOWN
    0x2ad,		// MU
    0x4ead,		// MUS
    0x32d,		// MY
    0xbb2d,		// MYNA
    0x2e,		// NA
    0x142e,		// NAE
    0x1c2e,		// NAG
    0x3242e,		// NAIF
    0x6242e,		// NAIL
    0x342e,		// NAM
    0x8002b42eL,		// NAME, seed word (533D: 4)
    0x402e,		// NAP
    0x2c02e,		// NAPE
    0x5c2e,		// NAW
    0x642e,		// NAY
    0xae,		// NE
    0x804ae,		// NEAP
    0x800904aeL,		// NEAR, seed word (533D: 4)
    0xa04ae,		// NEAT
    0xb4ae,		// NEMA
    0x248ae,		// NERD
    0xa4cae,		// NEST
    0x50ae,		// NET
    0x9d0ae,		// NETS
    0x60005cae,		// NEW, seed word (533D: 3)
    0x312e,		// NIL
    0x352e,		// NIM
    0x412e,		// NIP
    0xc12e,		// NIPA
    0x512e,		// NIT
    0x1ee,		// NO
    0x11ee,		// NOD
    0x291ee,		// NODE
    0x991ee,		// NODS
    0x13a91ee,		// NODUS
    0x615ee,		// NOEL
    0x995ee,		// NOES
    0x1dee,		// NOG
    0x21ee,		// NOH
    0x35ee,		// NOM
    0xb5ee,		// NOMA
    0x2b5ee,		// NOME
    0x2c1ee,		// NOPE
    0x600049ee,		// NOR, seed word (533D: 3)
    0x4dee,		// NOS
    0x8002cdeeL,		// NOSE, seed word (533D: 4)
    0x44dee,		// NOSH
    0x600051ee,		// NOT, seed word (533D: 3)
    0x2d1ee,		// NOTE
    0x132d1ee,		// NOTES
    0x9d5ee,		// NOUS
    0x60005dee,		// NOW, seed word (533D: 3)
    0x9ddee,		// NOWS
    0xa5dee,		// NOWT
    0x2ae,		// NU
    0x292ae,		// NUDE
    0x12292ae,		// NUDER
    0x24aae,		// NURD
    0x4eae,		// NUS
    0x52ae,		// NUT
    0x482f,		// OAR
    0xa4c2f,		// OAST
    0x502f,		// OAT
    0x9d02f,		// OATS
    0x46f,		// OCA
    0x9846f,		// OCAS
    0x8f,		// OD
    0x148f,		// ODE
    0x9948f,		// ODES
    0x4c8f,		// ODS
    0xaf,		// OE
    0x4caf,		// OES
    0xcf,		// OF
    0x50cf,		// OFT
    0xa0e2d0cfL,		// OFTEN, seed word (533D: 5)
    0x684ef,		// OGAM
    0x4000010f,		// OH, seed word (533D: 2)
    0x350f,		// OHM
    0x4d0f,		// OHS
    0x6000118f,		// OLD, seed word (533D: 3)
    0x158f,		// OLE
    0x958f,		// OLEA
    0x9958f,		// OLES
    0x1af,		// OM
    0x715af,		// OMEN
    0x915af,		// OMER
    0x400001cf,		// ON, seed word (533D: 2)
    0x600015cf,		// ONE, seed word (533D: 3)
    0x995cf,		// ONES
    0x4dcf,		// ONS
    0x142cdcf,		// ONSET
    0x9d5cf,		// ONUS
    0x20f,		// OP
    0x160f,		// OPE
    0x8007160fL,		// OPEN, seed word (533D: 4)
    0x4e0f,		// OPS
    0x24f,		// OR
    0x64f,		// ORA
    0xe4f,		// ORC
    0x98e4f,		// ORCS
    0x164f,		// ORE
    0x9964f,		// ORES
    0x4e4f,		// ORS
    0x524f,		// ORT
    0x9d24f,		// ORTS
    0x26f,		// OS
    0x166f,		// OSE
    0xa122a28fL,		// OTHER, seed word (533D: 5)
    0x12af,		// OUD
    0x992af,		// OUDS
    0x4aaf,		// OUR
    0x52af,		// OUT
    0x2ef,		// OW
    0x16ef,		// OWE
    0x996ef,		// OWES
    0x32ef,		// OWL
    0x3aef,		// OWN
    0x9baef,		// OWNS
    0x2ceef,		// OWSE
    0x32f,		// OY
    0x30,		// PA
    0xc30,		// PAC
    0x28c30,		// PACE
    0x1328c30,		// PACES
    0x98c30,		// PACS
    0x1030,		// PAD
    0x49030,		// PADI
    0x991030,		// PADRI
    0x2030,		// PAH
    0x22430,		// PAID
    0x62430,		// PAIL
    0x72430,		// PAIN
    0xa1472430L,		// PAINT, seed word (533D: 5)
    0x92430,		// PAIR
    0x3030,		// PAL
    0x2b030,		// PALE
    0xcb030,		// PALY
    0x3430,		// PAM
    0x9b430,		// PAMS
    0x3830,		// PAN
    0x2b830,		// PANE
    0xc2b830,		// PANEL
    0xa3830,		// PANT
    0x4830,		// PAR
    0x24830,		// PARD
    0x924830,		// PARDI
    0x9c830,		// PARS
    0x800a4830L,		// PART, seed word (533D: 4)
    0xa19a4830L,		// PARTY, seed word (533D: 5)
    0x4c30,		// PAS
    0x2cc30,		// PASE
    0x44c30,		// PASH
    0x800a4c30L,		// PAST, seed word (533D: 4)
    0x5030,		// PAT
    0xe4d030,		// PATIN
    0x9d030,		// PATS
    0xcd030,		// PATY
    0x60006430,		// PAY, seed word (533D: 3)
    0xb0,		// PE
    0x4b0,		// PEA
    0x584b0,		// PEAK
    0x13584b0,		// PEAKS
    0x604b0,		// PEAL
    0x704b0,		// PEAN
    0x984b0,		// PEAS
    0xcb0,		// PEC
    0x98cb0,		// PECS
    0x20b0,		// PEH
    0x9a0b0,		// PEHS
    0x38b0,		// PEN
    0xc0b8b0,		// PENAL
    0x73cb0,		// PEON
    0x48b0,		// PER
    0x4cb0,		// PES
    0x598510,		// PHASE
    0x2510,		// PHI
    0x9a510,		// PHIS
    0x130,		// PI
    0x530,		// PIA
    0x60530,		// PIAL
    0x70530,		// PIAN
    0x120b130,		// PILAR
    0x3930,		// PIN
    0xb930,		// PINA
    0xa3930,		// PINT
    0x1a3930,		// PINTA
    0x4d30,		// PIS
    0x44d30,		// PISH
    0x5130,		// PIT
    0xd130,		// PITA
    0xa0518590L,		// PLACE, seed word (533D: 5)
    0x70590,		// PLAN
    0xa0570590L,		// PLANE, seed word (533D: 5)
    0x800c8590L,		// PLAY, seed word (533D: 4)
    0x9590,		// PLEA
    0x171590,		// PLENA
    0x6590,		// PLY
    0x21f0,		// POH
    0x2b9f0,		// PONE
    0x2c9f0,		// PORE
    0x44df0,		// POSH
    0xa0650,		// PRAT
    0xc8650,		// PRAY
    0x3e50,		// PRO
    0x6650,		// PRY
    0x2670,		// PSI
    0x600052b0,		// PUT, seed word (533D: 3)
    0x730,		// PYA
    0x28c32,		// RACE
    0x1032,		// RAD
    0x1c32,		// RAG
    0x29c32,		// RAGE
    0x2032,		// RAH
    0x22432,		// RAID
    0x62432,		// RAIL
    0x72432,		// RAIN
    0x2b032,		// RALE
    0x3432,		// RAM
    0x60003832,		// RAN, seed word (533D: 3)
    0x3b832,		// RANG
    0x4b832,		// RANI
    0xa3832,		// RANT
    0x4032,		// RAP
    0xa044c032L,		// RAPID, seed word (533D: 5)
    0x9c032,		// RAPS
    0xa4032,		// RAPT
    0x4c32,		// RAS
    0x44c32,		// RASH
    0x84c32,		// RASP
    0x5032,		// RAT
    0x8002d032L,		// RATE, seed word (533D: 4)
    0x42d032,		// RATED
    0xc2d032,		// RATEL
    0x45032,		// RATH
    0x545032,		// RATHE
    0x5c32,		// RAW
    0x6032,		// RAX
    0x6432,		// RAY
    0x42e432,		// RAYED
    0xb2,		// RE
    0xa08184b2L,		// REACH, seed word (533D: 5)
    0x800204b2L,		// READ, seed word (533D: 4)
    0xa19204b2L,		// READY, seed word (533D: 5)
    0x800604b2L,		// REAL, seed word (533D: 4)
    0xcb2,		// REC
    0x98cb2,		// RECS
    0x600010b2,		// RED, seed word (533D: 3)
    0x18b2,		// REF
    0x1cb2,		// REG
    0xc09cb2,		// REGAL
    0x24b2,		// REI
    0x9a4b2,		// REIS
    0x190b0b2,		// RELAY
    0xcb0b2,		// RELY
    0x34b2,		// REM
    0xc0b8b2,		// RENAL
    0x238b2,		// REND
    0x40b2,		// REP
    0x7c0b2,		// REPO
    0x4cb2,		// RES
    0x44cb2,		// RESH
    0xa4cb2,		// REST
    0x50b2,		// RET
    0x70d0b2,		// RETAG
    0x180d0b2,		// RETAX
    0x9d0b2,		// RETS
    0x60b2,		// REX
    0x9512,		// RHEA
    0x3d12,		// RHO
    0x9bd12,		// RHOS
    0x532,		// RIA
    0x60532,		// RIAL
    0x1470532,		// RIANT
    0x1132,		// RID
    0x3932,		// RIN
    0x4132,		// RIP
    0x8002cd32L,		// RISE, seed word (533D: 4)
    0x2d132,		// RITE
    0x685f2,		// ROAM
    0xdf2,		// ROC
    0x98df2,		// ROCS
    0x11f2,		// ROD
    0x15f2,		// ROE
    0x995f2,		// ROES
    0x35f2,		// ROM
    0x8002c1f2L,		// ROPE, seed word (533D: 4)
    0x8002cdf2L,		// ROSE, seed word (533D: 4)
    0x142cdf2,		// ROSET
    0xccdf2,		// ROSY
    0x51f2,		// ROT
    0x2d1f2,		// ROTE
    0x132d1f2,		// ROTES
    0x9d1f2,		// ROTS
    0xa04755f2L,		// ROUND, seed word (533D: 5)
    0x60005df2,		// ROW, seed word (533D: 3)
    0x8a5df2,		// ROWTH
    0x292b2,		// RUDE
    0x16b2,		// RUE
    0x216b2,		// RUED
    0x996b2,		// RUES
    0x60003ab2,		// RUN, seed word (533D: 3)
    0x2bab2,		// RUNE
    0x2ceb2,		// RUSE
    0x732,		// RYA
    0x1732,		// RYE
    0xa3f32,		// RYOT
    0x13a3f32,		// RYOTS
    0xc33,		// SAC
    0x1033,		// SAD
    0x49033,		// SADI
    0x1433,		// SAE
    0x80022433L,		// SAID, seed word (533D: 4)
    0x80062433L,		// SAIL, seed word (533D: 4)
    0x2ac33,		// SAKE
    0x3033,		// SAL
    0x2b033,		// SALE
    0xa3033,		// SALT
    0x8002b433L,		// SAME, seed word (533D: 4)
    0x83433,		// SAMP
    0x4033,		// SAP
    0x60005033,		// SAT, seed word (533D: 3)
    0x2d033,		// SATE
    0x5c33,		// SAW
    0x60006433,		// SAY, seed word (533D: 3)
    0xa0560473L,		// SCALE, seed word (533D: 5)
    0x580473,		// SCAPE
    0xa0473,		// SCAT
    0xa0593c73L,		// SCORE, seed word (533D: 5)
    0xa3c73,		// SCOT
    0x600004b3,		// SEA, seed word (533D: 3)
    0x604b3,		// SEAL
    0x684b3,		// SEAM
    0x800a04b3L,		// SEAT, seed word (533D: 4)
    0xcb3,		// SEC
    0x14b3,		// SEE
    0x614b3,		// SEEL
    0x24b3,		// SEI
    0x30b3,		// SEL
    0x38b3,		// SEN
    0xa38b3,		// SENT
    0x48b3,		// SER
    0x50b3,		// SET
    0xd0b3,		// SETA
    0xc0d0b3,		// SETAL
    0xe7d0b3,		// SETON
    0x5cb3,		// SEW
    0x113,		// SH
    0x513,		// SHA
    0x20513,		// SHAD
    0xa0580513L,		// SHAPE, seed word (533D: 5)
    0xa1090513L,		// SHARP, seed word (533D: 5)
    0xb8513,		// SHAW
    0x60001513,		// SHE, seed word (533D: 3)
    0x9513,		// SHEA
    0xb9513,		// SHEW
    0x80082513L,		// SHIP, seed word (533D: 4)
    0x2bd13,		// SHOE
    0x122bd13,		// SHOER
    0x80083d13L,		// SHOP, seed word (533D: 4)
    0x593d13,		// SHORE
    0xa1493d13L,		// SHORT, seed word (533D: 5)
    0xa3d13,		// SHOT
    0x5a3d13,		// SHOTE
    0xbbd13,		// SHOW
    0xa0ebbd13L,		// SHOWN, seed word (533D: 5)
    0x133,		// SI
    0x60533,		// SIAL
    0x29133,		// SIDE
    0x561133,		// SIDLE
    0x23133,		// SILD
    0xa3133,		// SILT
    0x4133,		// SIP
    0x60004933,		// SIR, seed word (533D: 3)
    0x2c933,		// SIRE
    0x60005133,		// SIT, seed word (533D: 3)
    0x60006133,		// SIX, seed word (533D: 3)
    0x573,		// SKA
    0x81573,		// SKEP
    0xa0593,		// SLAT
    0x5a0593,		// SLATE
    0x21593,		// SLED
    0x1429593,		// SLEET
    0x22593,		// SLID
    0xa0522593L,		// SLIDE, seed word (533D: 5)
    0xa2593,		// SLIT
    0x2bd93,		// SLOE
    0xa3d93,		// SLOT
    0xa3dd3,		// SNOT
    0x800bbdd3L,		// SNOW, seed word (533D: 4)
    0x400001f3,		// SO, seed word (533D: 2)
    0x560df3,		// SOCLE
    0x11f3,		// SOD
    0x31f3,		// SOL
    0x2b1f3,		// SOLE
    0x600039f3,		// SON, seed word (533D: 3)
    0x2b9f3,		// SONE
    0x41f3,		// SOP
    0x441f3,		// SOPH
    0x2c9f3,		// SORE
    0xa49f3,		// SORT
    0x51f3,		// SOT
    0x451f3,		// SOTH
    0x55f3,		// SOU
    0xa04755f3L,		// SOUND, seed word (533D: 5)
    0x5df3,		// SOW
    0x75df3,		// SOWN
    0x65f3,		// SOY
    0x613,		// SPA
    0xa0518613L,		// SPACE, seed word (533D: 5)
    0x28613,		// SPAE
    0x558613,		// SPAKE
    0x90613,		// SPAR
    0xa0613,		// SPAT
    0xa0b09613L,		// SPEAK, seed word (533D: 5)
    0x19613,		// SPEC
    0x2653,		// SRI
    0x560693,		// STALE
    0xa1068693L,		// STAMP, seed word (533D: 5)
    0xc09693,		// STEAL
    0xa0c29693L,		// STEEL, seed word (533D: 5)
    0x161693,		// STELA
    0x561693,		// STELE
    0xf71693,		// STENO
    0xbe93,		// STOA
    0xa0563e93L,		// STOLE, seed word (533D: 5)
    0xa0573e93L,		// STONE, seed word (533D: 5)
    0xa0593e93L,		// STORE, seed word (533D: 5)
    0xa1993e93L,		// STORY, seed word (533D: 5)
    0x197ca93,		// STROY
    0x6693,		// STY
    0x16b3,		// SUE
    0x916b3,		// SUER
    0x600036b3,		// SUM, seed word (533D: 3)
    0x60003ab3,		// SUN, seed word (533D: 3)
    0x8002cab3L,		// SURE, seed word (533D: 4)
    0x34,		// TA
    0x834,		// TAB
    0xa0560834L,		// TABLE, seed word (533D: 5)
    0xa8834,		// TABU
    0x28c34,		// TACE
    0x40c34,		// TACH
    0x540c34,		// TACHE
    0x78c34,		// TACO
    0x1378c34,		// TACOS
    0x1034,		// TAD
    0x1434,		// TAE
    0x61434,		// TAEL
    0x1361434,		// TAELS
    0x1c34,		// TAG
    0x92034,		// TAHR
    0x80062434L,		// TAIL, seed word (533D: 4)
    0x72434,		// TAIN
    0x8002ac34L,		// TAKE, seed word (533D: 4)
    0x2b034,		// TALE
    0x122b034,		// TALER
    0x132b034,		// TALES
    0x4b034,		// TALI
    0x3434,		// TAM
    0x2b434,		// TAME
    0x83434,		// TAMP
    0x1383434,		// TAMPS
    0x9b434,		// TAMS
    0x3834,		// TAN
    0x3b834,		// TANG
    0x3c34,		// TAO
    0x9bc34,		// TAOS
    0x4034,		// TAP
    0x9c034,		// TAPS
    0x4834,		// TAR
    0x2c834,		// TARE
    0x42c834,		// TARED
    0x53c834,		// TARGE
    0x74834,		// TARN
    0x84834,		// TARP
    0x4c34,		// TAS
    0x5434,		// TAU
    0x5c34,		// TAW
    0x122dc34,		// TAWER
    0x60006034,		// TAX, seed word (533D: 3)
    0x122e034,		// TAXER
    0x4b4,		// TEA
    0xa08184b4L,		// TEACH, seed word (533D: 5)
    0x584b4,		// TEAK
    0x604b4,		// TEAL
    0x13604b4,		// TEALS
    0x800684b4L,		// TEAM, seed word (533D: 4)
    0x904b4,		// TEAR
    0x984b4,		// TEAS
    0x10b4,		// TED
    0x14b4,		// TEE
    0x614b4,		// TEEL
    0x13614b4,		// TEELS
    0x994b4,		// TEES
    0x1cb4,		// TEG
    0x30b4,		// TEL
    0xb0b4,		// TELA
    0x2b0b4,		// TELE
    0x132b0b4,		// TELES
    0x137b0b4,		// TELOS
    0x9b0b4,		// TELS
    0x600038b4,		// TEN, seed word (533D: 3)
    0x9b8b4,		// TENS
    0x13c8b4,		// TERGA
    0x164cb4,		// TESLA
    0x5cb4,		// TEW
    0x28514,		// THAE
    0x60001514,		// THE, seed word (533D: 3)
    0x119514,		// THECA
    0xa1249514L,		// THEIR, seed word (533D: 5)
    0x92514,		// THIR
    0x3d14,		// THO
    0xa059bd14L,		// THOSE, seed word (533D: 5)
    0x7c914,		// THRO
    0x57c914,		// THROE
    0x177c914,		// THROW
    0x134,		// TI
    0x29134,		// TIDE
    0x1534,		// TIE
    0x80021534L,		// TIED, seed word (533D: 4)
    0x91534,		// TIER
    0x3134,		// TIL
    0x9b134,		// TILS
    0x3934,		// TIN
    0x4134,		// TIP
    0x2c934,		// TIRE
    0x4d34,		// TIS
    0x1f4,		// TO
    0x205f4,		// TOAD
    0x19205f4,		// TOADY
    0x11f4,		// TOD
    0xa19091f4L,		// TODAY, seed word (533D: 5)
    0xc91f4,		// TODY
    0x15f4,		// TOE
    0x995f4,		// TOES
    0x1df4,		// TOG
    0x2b1f4,		// TOLE
    0x132b1f4,		// TOLES
    0x39f4,		// TON
    0x8002b9f4L,		// TONE, seed word (533D: 4)
    0x132b9f4,		// TONES
    0x9b9f4,		// TONS
    0x49f4,		// TOR
    0x2c9f4,		// TORE
    0x132c9f4,		// TORES
    0x9c9f4,		// TORS
    0x59c9f4,		// TORSE
    0xcc9f4,		// TORY
    0x44df4,		// TOSH
    0x5df4,		// TOW
    0x122ddf4,		// TOWER
    0x80075df4L,		// TOWN, seed word (533D: 4)
    0x65f4,		// TOY
    0x9e5f4,		// TOYS
    0x20654,		// TRAD
    0xa0520654L,		// TRADE, seed word (533D: 5)
    0xa0e48654L,		// TRAIN, seed word (533D: 5)
    0x80654,		// TRAP
    0xc8654,		// TRAY
    0x409654,		// TREAD
    0xbbe54,		// TROW
    0xcbe54,		// TROY
    0x13cbe54,		// TROYS
    0x6654,		// TRY
    0xab4,		// TUB
    0x8ab4,		// TUBA
    0x3ab4,		// TUN
    0xbab4,		// TUNA
    0x42b4,		// TUP
    0x6f4,		// TWA
    0x286f4,		// TWAE
    0x60003ef4,		// TWO, seed word (533D: 3)
    0x1734,		// TYE
    0x7cb34,		// TYRO
    0x137cb34,		// TYROS
    0x3c95,		// UDO
    0x9bc95,		// UDOS
    0x115,		// UH
    0x1b5,		// UM
    0x1d5,		// UN
    0x291d5,		// UNDE
    0xa12291d5L,		// UNDER, seed word (533D: 5)
    0x791d5,		// UNDO
    0x4dd5,		// UNS
    0x215,		// UP
    0x1255,		// URD
    0x3a55,		// URN
    0x275,		// US
    0x60001675,		// USE, seed word (533D: 3)
    0x91675,		// USER
    0x295,		// UT
    0x695,		// UTA
    0x1437,		// WAE
    0x3837,		// WAN
    0x60004837,		// WAR, seed word (533D: 3)
    0x2c837,		// WARE
    0xa4837,		// WART
    0x60004c37,		// WAS, seed word (533D: 3)
    0x80044c37L,		// WASH, seed word (533D: 4)
    0x5037,		// WAT
    0xa122d037L,		// WATER, seed word (533D: 5)
    0x60006437,		// WAY, seed word (533D: 3)
    0xb7,		// WE
    0x800904b7L,		// WEAR, seed word (533D: 4)
    0x38b7,		// WEN
    0xa48b7,		// WERT
    0x50b7,		// WET
    0x517,		// WHA
    0x60003d17,		// WHO, seed word (533D: 3)
    0x1493d17,		// WHORT
    0xa059bd17L,		// WHOSE, seed word (533D: 5)
    0x1f7,		// WO
    0x15f7,		// WOE
    0x995f7,		// WOES
    0xa0e0b5f7L,		// WOMAN, seed word (533D: 5)
    0xa0e2b5f7L,		// WOMEN, seed word (533D: 5)
    0x39f7,		// WON
    0x9b9f7,		// WONS
    0xa39f7,		// WONT
    0x2c9f7,		// WORE
    0xa49f7,		// WORT
    0xa08a49f7L,		// WORTH, seed word (533D: 5)
    0x4df7,		// WOS
    0x51f7,		// WOT
    0xa05a3e57L,		// WROTE, seed word (533D: 5)
    0x8a3e57,		// WROTH
    0x138,		// XI
    0x4d38,		// XIS
    0x39,		// YA
    0x23039,		// YALD
    0x3439,		// YAM
    0x4039,		// YAP
    0x4839,		// YAR
    0x24839,		// YARD
    0x2c839,		// YARE
    0x5c39,		// YAW
    0xb9,		// YE
    0x4b9,		// YEA
    0x800904b9L,		// YEAR, seed word (533D: 4)
    0x984b9,		// YEAS
    0x230b9,		// YELD
    0x38b9,		// YEN
    0x4cb9,		// YES
    0x600050b9,		// YET, seed word (533D: 3)
    0x1f9,		// YO
    0x9f9,		// YOB
    0x11f9,		// YOD
    0x35f9,		// YOM
    0x39f9,		// YON
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
        if (((sList[i] >> 29) & 0x7) == NUM_CUBES)
        {
            char word[MAX_LETTERS_PER_WORD + 1];
            if (!bitsToString(sList[i], word))
            {
                ASSERT(0);
                return false;
            }

            ASSERT(_SYS_strnlen(word, MAX_LETTERS_PER_WORD + 1) == MAX_LETTERS_PER_WORD);
            _SYS_strlcpy(buffer, word, MAX_LETTERS_PER_WORD + 1);
            return true;
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
        return _SYS_strncmp(*(const char **)a, word, MAX_LETTERS_PER_WORD + 1);
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
    for (unsigned j = 0; j < MAX_LETTERS_PER_WORD; ++j)
    {
        char letter = 'A' - 1 + ((bits >> (j * BITS_PER_LETTER)) & LTR_MASK);
        if (letter < 'A' || letter > 'Z')
        {
            break;
        }
        word[j] = letter;
    }
    _SYS_strlcpy(buffer, word, MAX_LETTERS_PER_WORD + 1);
    return buffer[0] != '\0';
}
