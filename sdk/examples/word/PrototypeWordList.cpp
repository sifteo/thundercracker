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
    0x21,		// AA
    0x3021,		// AAL
    0x9b021,		// AALS
    0x4c21,		// AAS
    0x41,		// AB
    0x21441,		// ABED
    0xa1441,		// ABET
    0x8002b041L,		// ABLE, seed word (533D: 4)
    0x3c41,		// ABO
    0xa14abc41L,		// ABOUT, seed word (533D: 5)
    0xa5441,		// ABUT
    0x6441,		// ABY
    0x2e441,		// ABYE
    0xe9028c61L,		// ACCEPT, seed word (533D: 6)
    0x1461,		// ACE
    0x21461,		// ACED
    0x99461,		// ACES
    0x2a061,		// ACHE
    0x2b861,		// ACNE
    0x2c861,		// ACRE
    0x5061,		// ACT
    0xe4d061,		// ACTIN
    0xdcf4d061L,		// ACTION, seed word (533D: 6)
    0x9d061,		// ACTS
    0x81,		// AD
    0x3c81,		// ADO
    0xe93c81,		// ADORN
    0x4c81,		// ADS
    0xa1,		// AE
    0x73ca1,		// AEON
    0xcc8a1,		// AERY
    0x904c1,		// AFAR
    0xc890c8c1L,		// AFRAID, seed word (533D: 6)
    0xe1,		// AG
    0x600014e1,		// AGE, seed word (533D: 3)
    0x214e1,		// AGED
    0x294e1,		// AGEE
    0x5714e1,		// AGENE
    0x914e1,		// AGER
    0x52b0e1,		// AGLEE
    0x60003ce1,		// AGO, seed word (533D: 3)
    0x73ce1,		// AGON
    0x52c8e1,		// AGREE
    0xc852c8e1L,		// AGREED, seed word (533D: 6)
    0x101,		// AH
    0x121,		// AI
    0x1121,		// AID
    0x99121,		// AIDS
    0x3121,		// AIL
    0x9b121,		// AILS
    0x3521,		// AIM
    0x122b521,		// AIMER
    0x3921,		// AIN
    0x9b921,		// AINS
    0x60004921,		// AIR, seed word (533D: 3)
    0x122c921,		// AIRER
    0x1c56c921,		// AIRMEN
    0x74921,		// AIRN
    0xa4921,		// AIRT
    0xcc921,		// AIRY
    0x4d21,		// AIS
    0x5121,		// AIT
    0x122d921,		// AIVER
    0x181,		// AL
    0x581,		// ALA
    0x90581,		// ALAR
    0x1990581,		// ALARY
    0x98581,		// ALAS
    0x981,		// ALB
    0x1581,		// ALE
    0x19581,		// ALEC
    0x1319581,		// ALECS
    0x29581,		// ALEE
    0x1491581,		// ALERT
    0x99581,		// ALES
    0x32581,		// ALIF
    0xa2581,		// ALIT
    0x3181,		// ALL
    0x52b181,		// ALLEE
    0xca72b181L,		// ALLEGE, seed word (533D: 6)
    0x192b181,		// ALLEY
    0xcb181,		// ALLY
    0x2b581,		// ALME
    0xe937b581L,		// ALMOST, seed word (533D: 6)
    0x9b581,		// ALMS
    0x2bd81,		// ALOE
    0xa0573d81L,		// ALONE, seed word (533D: 5)
    0x4181,		// ALP
    0x9c181,		// ALPS
    0x4d81,		// ALS
    0x7cd81,		// ALSO
    0x5181,		// ALT
    0x122d181,		// ALTER
    0x7d181,		// ALTO
    0x137d181,		// ALTOS
    0x9d181,		// ALTS
    0x190dd81,		// ALWAY
    0xe790dd81L,		// ALWAYS, seed word (533D: 6)
    0x400001a1,		// AM, seed word (533D: 2)
    0x715a1,		// AMEN
    0x14715a1,		// AMENT
    0x25a1,		// AMI
    0x2a5a1,		// AMIE
    0x725a1,		// AMIN
    0x5725a1,		// AMINE
    0x925a1,		// AMIR
    0xa0773da1L,		// AMONG, seed word (533D: 5)
    0xe8eabda1L,		// AMOUNT, seed word (533D: 6)
    0x41a1,		// AMP
    0x9c1a1,		// AMPS
    0x55a1,		// AMU
    0x9d5a1,		// AMUS
    0x665a1,		// AMYL
    0x400001c1,		// AN, seed word (533D: 2)
    0xe78dc1,		// ANCON
    0x600011c1,		// AND, seed word (533D: 3)
    0x991c1,		// ANDS
    0x15c1,		// ANE
    0x995c1,		// ANES
    0xc29dc1,		// ANGEL
    0x1229dc1,		// ANGER
    0xa0561dc1L,		// ANGLE, seed word (533D: 5)
    0x25c1,		// ANI
    0x625c1,		// ANIL
    0x13625c1,		// ANILS
    0x56a5c1,		// ANIME
    0xe7a5c1,		// ANION
    0x9a5c1,		// ANIS
    0x28e4bdc1,		// ANOINT
    0x563dc1,		// ANOLE
    0x73dc1,		// ANON
    0x51c1,		// ANT
    0x2d1c1,		// ANTE
    0x132d1c1,		// ANTES
    0x4d1c1,		// ANTI
    0x34d1c1,		// ANTIC
    0x9d1c1,		// ANTS
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
    0xa641,		// ARIA
    0x22641,		// ARID
    0x62641,		// ARIL
    0x3641,		// ARM
    0x800cb641L,		// ARMY, seed word (533D: 4)
    0xc8eabe41L,		// AROUND, seed word (533D: 6)
    0xcb64ca41L,		// ARRIVE, seed word (533D: 6)
    0x4e41,		// ARS
    0x60005241,		// ART, seed word (533D: 3)
    0xc2d241,		// ARTEL
    0xcd241,		// ARTY
    0x66641,		// ARYL
    0x1366641,		// ARYLS
    0x261,		// AS
    0x1478e61,		// ASCOT
    0x2261,		// ASH
    0x60002e61,		// ASK, seed word (533D: 3)
    0x4261,		// ASP
    0x166661,		// ASYLA
    0x40000281,		// AT, seed word (533D: 2)
    0x1681,		// ATE
    0x99681,		// ATES
    0x6be81,		// ATOM
    0x136be81,		// ATOMS
    0x6973e81,		// ATONIC
    0xe93a9ea1L,		// AUGUST, seed word (533D: 6)
    0x800a3aa1L,		// AUNT, seed word (533D: 4)
    0x7d2a1,		// AUTO
    0x16c1,		// AVE
    0xcb5716c1L,		// AVENUE, seed word (533D: 6)
    0x916c1,		// AVER
    0x14916c1,		// AVERT
    0x2e1,		// AW
    0x6e1,		// AWA
    0xc86e1,		// AWAY
    0x16e1,		// AWE
    0x32e1,		// AWL
    0x9b2e1,		// AWLS
    0x3ae1,		// AWN
    0x301,		// AX
    0x1701,		// AXE
    0x321,		// AY
    0x1721,		// AYE
    0x99721,		// AYES
    0x4f21,		// AYS
    0x22,		// BA
    0x60001022,		// BAD, seed word (533D: 3)
    0x29022,		// BADE
    0xa2422,		// BAIT
    0x3022,		// BAL
    0x2b022,		// BALE
    0x3422,		// BAM
    0x3822,		// BAN
    0x4b822,		// BANI
    0x5022,		// BAT
    0x2d022,		// BATE
    0x42d022,		// BATED
    0xe7d022,		// BATON
    0x6422,		// BAY
    0xa2,		// BE
    0x204a2,		// BEAD
    0x684a2,		// BEAM
    0x19684a2,		// BEAMY
    0x800a04a2L,		// BEAT, seed word (533D: 4)
    0x600010a2,		// BED, seed word (533D: 3)
    0x990a2,		// BEDS
    0x14a2,		// BEE
    0x994a2,		// BEES
    0xa14a2,		// BEET
    0x1ca2,		// BEG
    0x9249ca2,		// BEGIRD
    0xc8e4a0a2L,		// BEHIND, seed word (533D: 6)
    0x30a2,		// BEL
    0xa30a2,		// BELT
    0xb4a2,		// BEMA
    0x38a2,		// BEN
    0x238a2,		// BEND
    0x3c8a2,		// BERG
    0xca44cca2L,		// BESIDE, seed word (533D: 6)
    0x50a2,		// BET
    0xd0a2,		// BETA
    0x64a2,		// BEY
    0x122,		// BI
    0x1122,		// BID
    0x29122,		// BIDE
    0x1229122,		// BIDER
    0x1329122,		// BIDES
    0x99122,		// BIDS
    0x91522,		// BIER
    0x1d22,		// BIG
    0x2b122,		// BILE
    0x53b122,		// BILGE
    0x3922,		// BIN
    0x23922,		// BIND
    0x2b922,		// BINE
    0xa3922,		// BINT
    0x3d22,		// BIO
    0x1473d22,		// BIONT
    0x1a3d22,		// BIOTA
    0x24922,		// BIRD
    0x4d22,		// BIS
    0x2cd22,		// BISE
    0x60005122,		// BIT, seed word (533D: 3)
    0x28582,		// BLAE
    0xa0582,		// BLAT
    0x5a0582,		// BLATE
    0x1409582,		// BLEAT
    0xa1582,		// BLET
    0x1e2,		// BO
    0x5e2,		// BOA
    0x800a05e2L,		// BOAT, seed word (533D: 4)
    0x1de2,		// BOG
    0x549de2,		// BOGIE
    0x561de2,		// BOGLE
    0x625e2,		// BOIL
    0x2b1e2,		// BOLE
    0x344b9e2,		// BONITA
    0x51e2,		// BOT
    0xd1e2,		// BOTA
    0xa55e2,		// BOUT
    0x600065e2,		// BOY, seed word (533D: 3)
    0x21642,		// BRED
    0x522642,		// BRIDE
    0xca722642L,		// BRIDGE, seed word (533D: 6)
    0x2a642,		// BRIE
    0x3a642,		// BRIG
    0x52a2,		// BUT
    0x322,		// BY
    0x1722,		// BYE
    0x1023,		// CAD
    0x29023,		// CADE
    0x29c23,		// CAGE
    0x1229c23,		// CAGER
    0x72423,		// CAIN
    0x63023,		// CALL
    0xc8563023L,		// CALLED, seed word (533D: 6)
    0x3823,		// CAN
    0x2b823,		// CANE
    0xe8f73823L,		// CANNOT, seed word (533D: 6)
    0xe7b823,		// CANON
    0xa3823,		// CANT
    0xfa3823,		// CANTO
    0x1cfa3823,		// CANTON
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
    0x1cf4d023,		// CATION
    0x9d023,		// CATS
    0x8ca3,		// CECA
    0x490a3,		// CEDI
    0x14a3,		// CEE
    0x994a3,		// CEES
    0x624a3,		// CEIL
    0x30a3,		// CEL
    0x630a3,		// CELL
    0x1630a3,		// CELLA
    0x9b0a3,		// CELS
    0xa30a3,		// CELT
    0x13a30a3,		// CELTS
    0xa38a3,		// CENT
    0xfa38a3,		// CENTO
    0x40a3,		// CEP
    0x9c0a3,		// CEPS
    0x2c8a3,		// CERE
    0x132c8a3,		// CERES
    0x2752c8a3,		// CEREUS
    0x7c8a3,		// CERO
    0x137c8a3,		// CEROS
    0x2757c8a3,		// CEROUS
    0xb3ac8a3,		// CERUSE
    0x9a4ca3,		// CESTI
    0x2d0a3,		// CETE
    0x132d0a3,		// CETES
    0x770503,		// CHANG
    0xca770503L,		// CHANGE, seed word (533D: 6)
    0x90503,		// CHAR
    0x590503,		// CHARE
    0xca790503L,		// CHARGE, seed word (533D: 6)
    0xa0503,		// CHAT
    0x1409503,		// CHEAT
    0x2503,		// CHI
    0x72503,		// CHIN
    0x572503,		// CHINE
    0x26572503,		// CHINES
    0x1372503,		// CHINS
    0x9a503,		// CHIS
    0xf63d03,		// CHOLO
    0x26f63d03,		// CHOLOS
    0xcb37bd03L,		// CHOOSE, seed word (533D: 6)
    0x59bd03,		// CHOSE
    0x78523,		// CIAO
    0x1229123,		// CIDER
    0x2b923,		// CINE
    0x132b923,		// CINES
    0x73d23,		// CION
    0x2c923,		// CIRE
    0x4d23,		// CIS
    0xa4d23,		// CIST
    0x2d123,		// CITE
    0x42d123,		// CITED
    0x122d123,		// CITER
    0x132d123,		// CITES
    0xe654d123L,		// CITIES, seed word (533D: 6)
    0x20583,		// CLAD
    0x520583,		// CLADE
    0x70583,		// CLAN
    0x80583,		// CLAP
    0xa0e09583L,		// CLEAN, seed word (533D: 5)
    0xa1209583L,		// CLEAR, seed word (533D: 5)
    0x82583,		// CLIP
    0x83d83,		// CLOP
    0xa059bd83L,		// CLOSE, seed word (533D: 5)
    0xa14985e3L,		// COAST, seed word (533D: 5)
    0xa05e3,		// COAT
    0x9a05e3,		// COATI
    0x13a05e3,		// COATS
    0x11e3,		// COD
    0x291e3,		// CODE
    0xe291e3,		// CODEN
    0x26e291e3,		// CODENS
    0x12291e3,		// CODER
    0x13291e3,		// CODES
    0x991e3,		// CODS
    0x215e3,		// COED
    0x13215e3,		// COEDS
    0x1de3,		// COG
    0x7a1e3,		// COHO
    0x137a1e3,		// COHOS
    0xe3a5e3,		// COIGN
    0x625e3,		// COIL
    0x725e3,		// COIN
    0x31e3,		// COL
    0x2b1e3,		// COLE
    0x132b1e3,		// COLES
    0x9b1e3,		// COLS
    0x2b5e3,		// COME
    0xcee4b5e3L,		// COMING, seed word (533D: 6)
    0x39e3,		// CON
    0x2b9e3,		// CONE
    0x42b9e3,		// CONED
    0x132b9e3,		// CONES
    0x4b9e3,		// CONI
    0x739e3,		// CONN
    0x9b9e3,		// CONS
    0x5a39e3,		// CONTE
    0x3de3,		// COO
    0x63de3,		// COOL
    0x1363de3,		// COOLS
    0x9bde3,		// COOS
    0xa3de3,		// COOT
    0x41e3,		// COP
    0x2c1e3,		// COPE
    0x49e3,		// COR
    0x249e3,		// CORD
    0x245249e3,		// CORDER
    0x2c9e3,		// CORE
    0x42c9e3,		// CORED
    0x122c9e3,		// CORER
    0x132c9e3,		// CORES
    0x749e3,		// CORN
    0xe45749e3L,		// CORNER, seed word (533D: 6)
    0x59c9e3,		// CORSE
    0x4de3,		// COS
    0x44de3,		// COSH
    0xa4de3,		// COST
    0x1a4de3,		// COSTA
    0x51e3,		// COT
    0xe0d1e3,		// COTAN
    0x2d1e3,		// COTE
    0x9d1e3,		// COTS
    0xcb3955e3L,		// COURSE, seed word (533D: 6)
    0x38643,		// CRAG
    0x28921643,		// CREDIT
    0xf21643,		// CREDO
    0x42a643,		// CRIED
    0x573e43,		// CRONE
    0x593e43,		// CRORE
    0xb3abe43,		// CROUSE
    0x9d643,		// CRUS
    0x59d643,		// CRUSE
    0x16a3,		// CUE
    0x996a3,		// CUES
    0x4aa3,		// CUR
    0x2caa3,		// CURE
    0x132caa3,		// CURES
    0x9caa3,		// CURS
    0x59caa3,		// CURSE
    0x824,		// DAB
    0x28c24,		// DACE
    0x1c24,		// DAG
    0x2024,		// DAH
    0x9a024,		// DAHS
    0x1992424,		// DAIRY
    0x9a424,		// DAIS
    0x3024,		// DAL
    0x2b024,		// DALE
    0x9b024,		// DALS
    0x3424,		// DAM
    0x2b424,		// DAME
    0x3b824,		// DANG
    0x2453b824,		// DANGER
    0x4024,		// DAP
    0x2c824,		// DARE
    0x122c824,		// DARER
    0x74824,		// DARN
    0xa4824,		// DART
    0x80044c24L,		// DASH, seed word (533D: 4)
    0x8002d024L,		// DATE, seed word (533D: 4)
    0x122d024,		// DATER
    0x7d024,		// DATO
    0x60006424,		// DAY, seed word (533D: 3)
    0xa4,		// DE
    0x604a4,		// DEAL
    0x704a4,		// DEAN
    0x904a4,		// DEAR
    0x19904a4,		// DEARY
    0xa08a04a4L,		// DEATH, seed word (533D: 5)
    0x8a4,		// DEB
    0xcb4088a4L,		// DEBATE, seed word (533D: 6)
    0x988a4,		// DEBS
    0xa08a4,		// DEBT
    0xc08ca4,		// DECAL
    0x78ca4,		// DECO
    0x1278ca4,		// DECOR
    0x1378ca4,		// DECOS
    0x14a4,		// DEE
    0x214a4,		// DEED
    0x914a4,		// DEER
    0x13914a4,		// DEERS
    0x994a4,		// DEES
    0xa14a4,		// DEET
    0x13a14a4,		// DEETS
    0x498a4,		// DEFI
    0xa18a4,		// DEFT
    0xe3a4a4,		// DEIGN
    0x26e3a4a4,		// DEIGNS
    0x624a4,		// DEIL
    0x13624a4,		// DEILS
    0xd9a4a4,		// DEISM
    0x30a4,		// DEL
    0xa190b0a4L,		// DELAY, seed word (533D: 5)
    0x330a4,		// DELF
    0x14330a4,		// DELFT
    0x4b0a4,		// DELI
    0x134b0a4,		// DELIS
    0x630a4,		// DELL
    0x9b0a4,		// DELS
    0x7b4a4,		// DEMO
    0xe7b4a4,		// DEMON
    0xcb4a4,		// DEMY
    0x38a4,		// DEN
    0x2b8a4,		// DENE
    0x854b8a4,		// DENIED
    0x9b8a4,		// DENS
    0xa38a4,		// DENT
    0x1c9a38a4,		// DENTIN
    0x140c8a4,		// DERAT
    0x190c8a4,		// DERAY
    0x2c8a4,		// DERE
    0x6c8a4,		// DERM
    0xe922cca4L,		// DESERT, seed word (533D: 6)
    0xdc74cca4L,		// DESIGN, seed word (533D: 6)
    0xcb24cca4L,		// DESIRE, seed word (533D: 6)
    0x122d0a4,		// DETER
    0x2722d0a4,		// DETERS
    0x58a4,		// DEV
    0x5ca4,		// DEW
    0x64a4,		// DEY
    0x60524,		// DIAL
    0x1360524,		// DIALS
    0x1990524,		// DIARY
    0x924,		// DIB
    0x98924,		// DIBS
    0x28d24,		// DICE
    0x1228d24,		// DICER
    0x1124,		// DID
    0x1524,		// DIE
    0x21524,		// DIED
    0x61524,		// DIEL
    0x571524,		// DIENE
    0x99524,		// DIES
    0xa1524,		// DIET
    0x1d24,		// DIG
    0x99d24,		// DIGS
    0x63124,		// DILL
    0x3524,		// DIM
    0x2b524,		// DIME
    0x132b524,		// DIMES
    0x9b524,		// DIMS
    0x3924,		// DIN
    0x2b924,		// DINE
    0x42b924,		// DINED
    0x122b924,		// DINER
    0x132b924,		// DINES
    0x3b924,		// DING
    0x53b924,		// DINGE
    0x2653b924,		// DINGES
    0x133b924,		// DINGS
    0x9b924,		// DINS
    0xa3924,		// DINT
    0x4124,		// DIP
    0x2c924,		// DIRE
    0xe832c924L,		// DIRECT, seed word (533D: 6)
    0x53c924,		// DIRGE
    0xa4924,		// DIRT
    0x4d24,		// DIS
    0x56cd24,		// DISME
    0x5124,		// DIT
    0x2d124,		// DITE
    0x2d924,		// DIVE
    0x122d924,		// DIVER
    0x72544,		// DJIN
    0x400001e4,		// DO, seed word (533D: 2)
    0xa05e4,		// DOAT
    0xde4,		// DOC
    0x98de4,		// DOCS
    0xe4fa0de4L,		// DOCTOR, seed word (533D: 6)
    0x15e4,		// DOE
    0x915e4,		// DOER
    0x800995e4L,		// DOES, seed word (533D: 4)
    0x60001de4,		// DOG, seed word (533D: 3)
    0x31e4,		// DOL
    0x2b1e4,		// DOLE
    0x631e4,		// DOLL
    0x9b1e4,		// DOLS
    0x35e4,		// DOM
    0x2b5e4,		// DOME
    0x39e4,		// DON
    0xb9e4,		// DONA
    0x8002b9e4L,		// DONE, seed word (533D: 4)
    0x9b9e4,		// DONS
    0x93de4,		// DOOR
    0x2c1e4,		// DOPE
    0x122c1e4,		// DOPER
    0x2454c1e4,		// DOPIER
    0x49e4,		// DOR
    0x2c9e4,		// DORE
    0x6c9e4,		// DORM
    0x849e4,		// DORP
    0x949e4,		// DORR
    0x4de4,		// DOS
    0x2cde4,		// DOSE
    0x51e4,		// DOT
    0xcd1e4,		// DOTY
    0x955e4,		// DOUR
    0x1955e4,		// DOURA
    0x5de4,		// DOW
    0x122dde4,		// DOWER
    0x75de4,		// DOWN
    0x24575de4,		// DOWNER
    0x38644,		// DRAG
    0xa538644,		// DRAGEE
    0xa0644,		// DRAT
    0xc8644,		// DRAY
    0x1209644,		// DREAR
    0x29644,		// DREE
    0x1329644,		// DREES
    0x39644,		// DREG
    0x1499644,		// DREST
    0xb9644,		// DREW
    0x12644,		// DRIB
    0x132a644,		// DRIES
    0x82644,		// DRIP
    0x5b2644,		// DRIVE
    0xdc5b2644L,		// DRIVEN, seed word (533D: 6)
    0xc63e44,		// DROLL
    0x573e44,		// DRONE
    0x83e44,		// DROP
    0xebbe44,		// DROWN
    0x3d644,		// DRUG
    0x6644,		// DRY
    0x12a4,		// DUD
    0x292a4,		// DUDE
    0x13292a4,		// DUDES
    0x992a4,		// DUDS
    0x16a4,		// DUE
    0x616a4,		// DUEL
    0x996a4,		// DUES
    0x1ea4,		// DUG
    0x26a4,		// DUI
    0x632a4,		// DULL
    0x3aa4,		// DUN
    0x2baa4,		// DUNE
    0x132baa4,		// DUNES
    0x3baa4,		// DUNG
    0x9baa4,		// DUNS
    0x3ea4,		// DUO
    0x9bea4,		// DUOS
    0x42a4,		// DUP
    0x2c2a4,		// DUPE
    0x132c2a4,		// DUPES
    0x5642a4,		// DUPLE
    0x9c2a4,		// DUPS
    0xcaa4,		// DURA
    0x2caa4,		// DURE
    0xcee4caa4L,		// DURING, seed word (533D: 6)
    0x74aa4,		// DURN
    0x7caa4,		// DURO
    0x1724,		// DYE
    0x91724,		// DYER
    0x40c25,		// EACH
    0x1229c25,		// EAGER
    0x561c25,		// EAGLE
    0x591c25,		// EAGRE
    0x4825,		// EAR
    0x42c825,		// EARED
    0x64825,		// EARL
    0xa1964825L,		// EARLY, seed word (533D: 5)
    0x74825,		// EARN
    0x8a4825,		// EARTH
    0x2cc25,		// EASE
    0xa4c25,		// EAST
    0x800ccc25L,		// EASY, seed word (533D: 4)
    0x60005025,		// EAT, seed word (533D: 3)
    0xe2d025,		// EATEN
    0x45025,		// EATH
    0x9d025,		// EATS
    0x5425,		// EAU
    0x2d825,		// EAVE
    0x7a065,		// ECHO
    0x137a065,		// ECHOS
    0xac865,		// ECRU
    0x13ac865,		// ECRUS
    0x5465,		// ECU
    0x9d465,		// ECUS
    0x85,		// ED
    0x29c85,		// EDGE
    0x1229c85,		// EDGER
    0x2085,		// EDH
    0x9a085,		// EDHS
    0x141a485,		// EDICT
    0xa2485,		// EDIT
    0x30a5,		// EEL
    0x9b0a5,		// EELS
    0xcc8a5,		// EERY
    0xc5,		// EF
    0x18c5,		// EFF
    0xe92798c5L,		// EFFORT, seed word (533D: 6)
    0x998c5,		// EFFS
    0x4cc5,		// EFS
    0x50c5,		// EFT
    0x9d0c5,		// EFTS
    0x204e5,		// EGAD
    0x604e5,		// EGAL
    0x914e5,		// EGER
    0x1ce5,		// EGG
    0x9a4e5,		// EGIS
    0x3ce5,		// EGO
    0x105,		// EH
    0x29125,		// EIDE
    0x1229125,		// EIDER
    0x27229125,		// EIDERS
    0x1441d25,		// EIGHT
    0x11441d25,		// EIGHTH
    0xe4545125L,		// EITHER, seed word (533D: 6)
    0x185,		// EL
    0x70585,		// ELAN
    0x1185,		// ELD
    0x99185,		// ELDS
    0x1419585,		// ELECT
    0x27419585,		// ELECTS
    0x1985,		// ELF
    0x1472585,		// ELINT
    0x27472585,		// ELINTS
    0x3185,		// ELL
    0x3585,		// ELM
    0x9b585,		// ELMS
    0xcb585,		// ELMY
    0x4d85,		// ELS
    0x2cd85,		// ELSE
    0x1a5,		// EM
    0x19089a5,		// EMBAY
    0x1a5a5,		// EMIC
    0x925a5,		// EMIR
    0xa25a5,		// EMIT
    0xf2f641a5L,		// EMPLOY, seed word (533D: 6)
    0x4da5,		// EMS
    0x55a5,		// EMU
    0x265a5,		// EMYD
    0x1c5,		// EN
    0x5a05c5,		// ENATE
    0x265a05c5,		// ENATES
    0x600011c5,		// END, seed word (533D: 3)
    0x4291c5,		// ENDED
    0x17791c5,		// ENDOW
    0x991c5,		// ENDS
    0xf27915c5L,		// ENERGY, seed word (533D: 6)
    0x1dc5,		// ENG
    0xca709dc5L,		// ENGAGE, seed word (533D: 6)
    0x99dc5,		// ENGS
    0x2934b1c5,		// ENLIST
    0x63dc5,		// ENOL
    0x1363dc5,		// ENOLS
    0xd93dc5,		// ENORM
    0xd07abdc5L,		// ENOUGH, seed word (533D: 6)
    0xbbdc5,		// ENOW
    0x4dc5,		// ENS
    0x122d1c5,		// ENTER
    0xcb24d1c5L,		// ENTIRE, seed word (533D: 6)
    0x39e5,		// EON
    0x9b9e5,		// EONS
    0x1418605,		// EPACT
    0xa205,		// EPHA
    0x130a205,		// EPHAS
    0x1a605,		// EPIC
    0x245,		// ER
    0x645,		// ERA
    0x1645,		// ERE
    0x1e45,		// ERG
    0x79e45,		// ERGO
    0x1479e45,		// ERGOT
    0x1e772645,		// ERINGO
    0x3a45,		// ERN
    0x2ba45,		// ERNE
    0x9be45,		// EROS
    0x4a45,		// ERR
    0x4e45,		// ERS
    0xa4e45,		// ERST
    0x265,		// ES
    0x4e65,		// ESS
    0x122d265,		// ESTER
    0x285,		// ET
    0x685,		// ETA
    0x98685,		// ETAS
    0x40e85,		// ETCH
    0x2285,		// ETH
    0x122a285,		// ETHER
    0x137a285,		// ETHOS
    0x9a285,		// ETHS
    0x1a685,		// ETIC
    0xba85,		// ETNA
    0x130ba85,		// ETNAS
    0x4d685,		// ETUI
    0x7caa5,		// EURO
    0x137caa5,		// EUROS
    0x16c5,		// EVE
    0x716c5,		// EVEN
    0x16e5,		// EWE
    0x996e5,		// EWES
    0x305,		// EX
    0x73f05,		// EXON
    0xa0195305L,		// EXTRA, seed word (533D: 5)
    0x98725,		// EYAS
    0x1725,		// EYE
    0x71725,		// EYEN
    0x91725,		// EYER
    0x2bb25,		// EYNE
    0xcb25,		// EYRA
    0x2cb25,		// EYRE
    0x26,		// FA
    0x1026,		// FAD
    0x62426,		// FAIL
    0x72426,		// FAIN
    0x92426,		// FAIR
    0x1992426,		// FAIRY
    0xf2c4b426L,		// FAMILY, seed word (533D: 6)
    0xe757b426L,		// FAMOUS, seed word (533D: 6)
    0x3826,		// FAN
    0x60004826,		// FAR, seed word (533D: 3)
    0x40c826,		// FARAD
    0x24826,		// FARD
    0x2c826,		// FARE
    0x4c26,		// FAS
    0x6426,		// FAY
    0x800904a6L,		// FEAR, seed word (533D: 4)
    0x10a6,		// FED
    0x14a6,		// FEE
    0x614a6,		// FEEL
    0x994a6,		// FEES
    0x149a4a6,		// FEIST
    0x44b0a6,		// FELID
    0x630a6,		// FELL
    0xa30a6,		// FELT
    0x13a30a6,		// FELTS
    0x38a6,		// FEN
    0x238a6,		// FEND
    0x48a6,		// FER
    0x2c8a6,		// FERE
    0x132c8a6,		// FERES
    0xa9648a6,		// FERLIE
    0x748a6,		// FERN
    0x50a6,		// FET
    0x44d0a6,		// FETID
    0x127d0a6,		// FETOR
    0x2727d0a6,		// FETORS
    0x9d0a6,		// FETS
    0x54a6,		// FEU
    0x9d4a6,		// FEUS
    0x90526,		// FIAR
    0x1126,		// FID
    0x1526,		// FIE
    0xa0461526L,		// FIELD, seed word (533D: 5)
    0x471526,		// FIEND
    0x1d26,		// FIG
    0xcb2a9d26L,		// FIGURE, seed word (533D: 6)
    0x3126,		// FIL
    0xb126,		// FILA
    0x2b126,		// FILE
    0x42b126,		// FILED
    0x122b126,		// FILER
    0x132b126,		// FILES
    0x142b126,		// FILET
    0x2742b126,		// FILETS
    0x63126,		// FILL
    0x563126,		// FILLE
    0xc8563126L,		// FILLED, seed word (533D: 6)
    0x6b126,		// FILM
    0x196b126,		// FILMY
    0x9b126,		// FILS
    0x3926,		// FIN
    0xa0c0b926L,		// FINAL, seed word (533D: 5)
    0x23926,		// FIND
    0x24523926,		// FINDER
    0x2b926,		// FINE
    0x42b926,		// FINED
    0x122b926,		// FINER
    0x7b926,		// FINO
    0x4926,		// FIR
    0x2c926,		// FIRE
    0x42c926,		// FIRED
    0x6c926,		// FIRM
    0x74926,		// FIRN
    0xa4d26,		// FIST
    0x60005126,		// FIT, seed word (533D: 3)
    0x9d126,		// FITS
    0x68586,		// FLAM
    0x1968586,		// FLAMY
    0x70586,		// FLAN
    0xc8586,		// FLAY
    0x21586,		// FLED
    0x29586,		// FLEE
    0x1229586,		// FLEER
    0x42a586,		// FLIED
    0x122a586,		// FLIER
    0x132a586,		// FLIES
    0x2932a586,		// FLIEST
    0xa2586,		// FLIT
    0x5a2586,		// FLITE
    0x85a2586,		// FLITED
    0x265a2586,		// FLITES
    0x13a2586,		// FLITS
    0x6586,		// FLY
    0x685e6,		// FOAM
    0x13685e6,		// FOAMS
    0x15e6,		// FOE
    0x995e6,		// FOES
    0x1de6,		// FOG
    0x21e6,		// FOH
    0x725e6,		// FOIN
    0x39e6,		// FON
    0xa39e6,		// FONT
    0x600049e6,		// FOR, seed word (533D: 3)
    0x2c9e6,		// FORE
    0x132c9e6,		// FORES
    0xe932c9e6L,		// FOREST, seed word (533D: 6)
    0x53c9e6,		// FORGE
    0xe853c9e6L,		// FORGET, seed word (533D: 6)
    0x6c9e6,		// FORM
    0xa49e6,		// FORT
    0x5a49e6,		// FORTE
    0x265a49e6,		// FORTES
    0x8a49e6,		// FORTH
    0x13a49e6,		// FORTS
    0x245a4de6,		// FOSTER
    0x55e6,		// FOU
    0x955e6,		// FOUR
    0xd14955e6L,		// FOURTH, seed word (533D: 6)
    0x28646,		// FRAE
    0xc8646,		// FRAY
    0x29646,		// FREE
    0x1329646,		// FREES
    0xa1646,		// FRET
    0x13a1646,		// FRETS
    0xf2122646L,		// FRIDAY, seed word (533D: 6)
    0x42a646,		// FRIED
    0xc8e2a646L,		// FRIEND, seed word (533D: 6)
    0x3a646,		// FRIG
    0x3e46,		// FRO
    0x2be46,		// FROE
    0x132be46,		// FROES
    0x3be46,		// FROG
    0x6be46,		// FROM
    0x149be46,		// FROST
    0x8a3e46,		// FROTH
    0x3d646,		// FRUG
    0x6646,		// FRY
    0x1ea6,		// FUG
    0x4aa6,		// FUR
    0x9caa6,		// FURS
    0x2cea6,		// FUSE
    0x52cea6,		// FUSEE
    0x1027,		// GAD
    0x1427,		// GAE
    0x21427,		// GAED
    0x71427,		// GAEN
    0x1c27,		// GAG
    0x29c27,		// GAGE
    0x3027,		// GAL
    0x2b027,		// GALE
    0x63027,		// GALL
    0x3427,		// GAM
    0x3827,		// GAN
    0x24523827,		// GANDER
    0x2b827,		// GANE
    0x3b827,		// GANG
    0x4827,		// GAR
    0xdc524827L,		// GARDEN, seed word (533D: 6)
    0x8594827,		// GARRED
    0x60004c27,		// GAS, seed word (533D: 3)
    0xa4c27,		// GAST
    0x5027,		// GAT
    0x2d027,		// GATE
    0x9d027,		// GATS
    0x904a7,		// GEAR
    0x85904a7,		// GEARED
    0x10a7,		// GED
    0x990a7,		// GEDS
    0x14a7,		// GEE
    0x214a7,		// GEED
    0x30a7,		// GEL
    0x9b0a7,		// GELS
    0xa30a7,		// GELT
    0x38a7,		// GEN
    0x2b8a7,		// GENE
    0x5938a7,		// GENRE
    0xf938a7,		// GENRO
    0x9b8a7,		// GENS
    0xa38a7,		// GENT
    0xab8a7,		// GENU
    0x80c8a7,		// GERAH
    0x50a7,		// GET
    0xd0a7,		// GETA
    0x64a7,		// GEY
    0x2507,		// GHI
    0x927,		// GIB
    0x28927,		// GIBE
    0x428927,		// GIBED
    0x1228927,		// GIBER
    0x1127,		// GID
    0x99127,		// GIDS
    0x1527,		// GIE
    0x21527,		// GIED
    0x71527,		// GIEN
    0x99527,		// GIES
    0x3927,		// GIN
    0x9b927,		// GINS
    0x24927,		// GIRD
    0x74927,		// GIRN
    0x1374927,		// GIRNS
    0x7c927,		// GIRO
    0xe7c927,		// GIRON
    0xa4927,		// GIRT
    0x13a4927,		// GIRTS
    0xa4d27,		// GIST
    0x5127,		// GIT
    0x9d127,		// GITS
    0x590587,		// GLARE
    0xe09587,		// GLEAN
    0x29587,		// GLEE
    0x71587,		// GLEN
    0x1371587,		// GLENS
    0x12587,		// GLIB
    0x13d87,		// GLOB
    0x513d87,		// GLOBE
    0x905c7,		// GNAR
    0xa05c7,		// GNAT
    0x696bdc7,		// GNOMIC
    0x55c7,		// GNU
    0x1e7,		// GO
    0x5e7,		// GOA
    0x9e7,		// GOB
    0x11e7,		// GOD
    0x915e7,		// GOER
    0x12299e7,		// GOFER
    0x2b9e7,		// GONE
    0x122b9e7,		// GONER
    0x49e7,		// GOR
    0x2c9e7,		// GORE
    0x600051e7,		// GOT, seed word (533D: 3)
    0x61e7,		// GOX
    0x65e7,		// GOY
    0x518647,		// GRACE
    0x20647,		// GRAD
    0x520647,		// GRADE
    0x24520647,		// GRADER
    0x70647,		// GRAN
    0x470647,		// GRAND
    0xa1470647L,		// GRANT, seed word (533D: 5)
    0xa0647,		// GRAT
    0x5a0647,		// GRATE
    0xa1409647L,		// GREAT, seed word (533D: 5)
    0x29647,		// GREE
    0x429647,		// GREED
    0xe29647,		// GREEN
    0x32e29647,		// GREENY
    0xc9647,		// GREY
    0x22647,		// GRID
    0x522647,		// GRIDE
    0x62a647,		// GRIEF
    0x72647,		// GRIN
    0x472647,		// GRIND
    0x1372647,		// GRINS
    0x149a647,		// GRIST
    0xa2647,		// GRIT
    0x13a2647,		// GRITS
    0xe4be47,		// GROIN
    0xa3e47,		// GROT
    0x13a3e47,		// GROTS
    0x2d647,		// GRUE
    0x226a7,		// GUID
    0x60003aa7,		// GUN, seed word (533D: 3)
    0xa4ea7,		// GUST
    0x52a7,		// GUT
    0x9d2a7,		// GUTS
    0x2cb27,		// GYRE
    0xae2cb27,		// GYRENE
    0x28,		// HA
    0x60001028,		// HAD, seed word (533D: 3)
    0x29028,		// HADE
    0x1428,		// HAE
    0x21428,		// HAED
    0x71428,		// HAEN
    0x99428,		// HAES
    0xa1428,		// HAET
    0x1c28,		// HAG
    0x51b828,		// HANCE
    0x3b828,		// HANG
    0x4028,		// HAP
    0xdc584028L,		// HAPPEN, seed word (533D: 6)
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
    0x42d028,		// HATED
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
    0x614a8,		// HEEL
    0x13614a8,		// HEELS
    0x20a8,		// HEH
    0x83a4a8,		// HEIGH
    0xe883a4a8L,		// HEIGHT, seed word (533D: 6)
    0x924a8,		// HEIR
    0x34a8,		// HEM
    0x38a8,		// HEN
    0x9b8a8,		// HENS
    0xa38a8,		// HENT
    0x40a8,		// HEP
    0x600048a8,		// HER, seed word (533D: 3)
    0x248a8,		// HERD
    0x2c8a8,		// HERE
    0x7c8a8,		// HERO
    0x137c8a8,		// HEROS
    0x9c8a8,		// HERS
    0x4ca8,		// HES
    0xa4ca8,		// HEST
    0x50a8,		// HET
    0x450a8,		// HETH
    0x9d0a8,		// HETS
    0x5ca8,		// HEW
    0x9dca8,		// HEWS
    0x128,		// HI
    0xd28,		// HIC
    0x1128,		// HID
    0x29128,		// HIDE
    0x1528,		// HIE
    0x21528,		// HIED
    0x99528,		// HIES
    0x41d28,		// HIGH
    0x1441d28,		// HIGHT
    0x60003528,		// HIM, seed word (533D: 3)
    0x3928,		// HIN
    0x23928,		// HIND
    0x9b928,		// HINS
    0x4128,		// HIP
    0x9c128,		// HIPS
    0x2c928,		// HIRE
    0x60004d28,		// HIS, seed word (533D: 3)
    0x74d28,		// HISN
    0x60005128,		// HIT, seed word (533D: 3)
    0x1a8,		// HM
    0x1e8,		// HO
    0x11e8,		// HOD
    0x991e8,		// HODS
    0x15e8,		// HOE
    0x915e8,		// HOER
    0x13915e8,		// HOERS
    0x995e8,		// HOES
    0x1de8,		// HOG
    0x231e8,		// HOLD
    0x13231e8,		// HOLDS
    0x9b1e8,		// HOLS
    0x8002b5e8L,		// HOME, seed word (533D: 4)
    0x39e8,		// HON
    0x2b9e8,		// HONE
    0x3b9e8,		// HONG
    0x9b9e8,		// HONS
    0x41e8,		// HOP
    0x9c1e8,		// HOPS
    0xa059c9e8L,		// HORSE, seed word (533D: 5)
    0x149c9e8,		// HORST
    0x2cde8,		// HOSE
    0xa4de8,		// HOST
    0x600051e8,		// HOT, seed word (533D: 3)
    0x9d1e8,		// HOTS
    0x955e8,		// HOUR
    0xa059d5e8L,		// HOUSE, seed word (533D: 5)
    0x5de8,		// HOW
    0x2dde8,		// HOWE
    0x132dde8,		// HOWES
    0x9dde8,		// HOWS
    0x16a8,		// HUE
    0x216a8,		// HUED
    0x996a8,		// HUES
    0x1ea8,		// HUG
    0x29ea8,		// HUGE
    0x3aa8,		// HUN
    0x3baa8,		// HUNG
    0x42a8,		// HUP
    0xa4aa8,		// HURT
    0x52a8,		// HUT
    0x1469,		// ICE
    0x21469,		// ICED
    0x99469,		// ICES
    0x2069,		// ICH
    0x9a069,		// ICHS
    0x2932a469,		// ICIEST
    0x73c69,		// ICON
    0x89,		// ID
    0x69489,		// IDEM
    0x99489,		// IDES
    0x2b089,		// IDLE
    0x42b089,		// IDLED
    0x132b089,		// IDLES
    0x4c89,		// IDS
    0xc9,		// IF
    0x4cc9,		// IFS
    0xb27b8e9,		// IGNORE
    0x3189,		// ILL
    0x41a9,		// IMP
    0xc2c1a9,		// IMPEL
    0x26c2c1a9,		// IMPELS
    0x9c1a9,		// IMPS
    0x1c9,		// IN
    0x14805c9,		// INAPT
    0xd905c9,		// INARM
    0x40dc9,		// INCH
    0xe6540dc9L,		// INCHES, seed word (533D: 6)
    0x778dc9,		// INCOG
    0xcad78dc9L,		// INCOME, seed word (533D: 6)
    0xc85291c9L,		// INDEED, seed word (533D: 6)
    0x28e291c9,		// INDENT
    0x5491c9,		// INDIE
    0x265491c9,		// INDIES
    0x14915c9,		// INERT
    0x12299c9,		// INFER
    0x799c9,		// INFO
    0xdb2799c9L,		// INFORM, seed word (533D: 6)
    0x561dc9,		// INGLE
    0x26561dc9,		// INGLES
    0x142b1c9,		// INLET
    0x2742b1c9,		// INLETS
    0x39c9,		// INN
    0x42b9c9,		// INNED
    0x7c9c9,		// INRO
    0x4dc9,		// INS
    0x142cdc9,		// INSET
    0xca44cdc9L,		// INSIDE, seed word (533D: 6)
    0xc8e2d1c9L,		// INTEND, seed word (533D: 6)
    0x122d1c9,		// INTER
    0x7d1c9,		// INTO
    0x39e9,		// ION
    0xd1e9,		// IOTA
    0x1649,		// IRE
    0x21649,		// IRED
    0x99649,		// IRES
    0x73e49,		// IRON
    0x573e49,		// IRONE
    0x40000269,		// IS, seed word (533D: 2)
    0xc8e0b269L,		// ISLAND, seed word (533D: 6)
    0x2b269,		// ISLE
    0x42b269,		// ISLED
    0x142b269,		// ISLET
    0x3669,		// ISM
    0x565269,		// ISTLE
    0x40000289,		// IT, seed word (533D: 2)
    0x69689,		// ITEM
    0x122a289,		// ITHER
    0x4e89,		// ITS
    0xccc2ce89L,		// ITSELF, seed word (533D: 6)
    0x73caa,		// JEON
    0x392a,		// JIN
    0x1ea,		// JO
    0x15ea,		// JOE
    0x725ea,		// JOIN
    0xc85725eaL,		// JOINED, seed word (533D: 6)
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
    0x428c2c,		// LACED
    0x1228c2c,		// LACER
    0x1328c2c,		// LACES
    0x98c2c,		// LACS
    0x102c,		// LAD
    0x2902c,		// LADE
    0x56102c,		// LADLE
    0x9902c,		// LADS
    0xc902c,		// LADY
    0x1c2c,		// LAG
    0x1229c2c,		// LAGER
    0x2242c,		// LAID
    0x7242c,		// LAIN
    0x9242c,		// LAIR
    0x342c,		// LAM
    0x2b42c,		// LAME
    0x9b42c,		// LAMS
    0x51b82c,		// LANCE
    0x2382c,		// LAND
    0x132382c,		// LANDS
    0x2b82c,		// LANE
    0x3b82c,		// LANG
    0x402c,		// LAP
    0xe4c02c,		// LAPIN
    0x26e4c02c,		// LAPINS
    0x134c02c,		// LAPIS
    0x9c02c,		// LAPS
    0x482c,		// LAR
    0xa053c82cL,		// LARGE, seed word (533D: 5)
    0x4c82c,		// LARI
    0x9c82c,		// LARS
    0x4c2c,		// LAS
    0x2cc2c,		// LASE
    0x800a4c2cL,		// LAST, seed word (533D: 4)
    0x502c,		// LAT
    0x8002d02cL,		// LATE, seed word (533D: 4)
    0xa122d02cL,		// LATER, seed word (533D: 5)
    0x4d02c,		// LATI
    0x9d02c,		// LATS
    0x582c,		// LAV
    0x2d82c,		// LAVE
    0x122d82c,		// LAVER
    0x60005c2c,		// LAW, seed word (533D: 3)
    0x9dc2c,		// LAWS
    0x6000642c,		// LAY, seed word (533D: 3)
    0x42e42c,		// LAYED
    0x122e42c,		// LAYER
    0x9e42c,		// LAYS
    0x4ac,		// LEA
    0x800204acL,		// LEAD, seed word (533D: 4)
    0x19204ac,		// LEADY
    0x604ac,		// LEAL
    0x704ac,		// LEAN
    0x804ac,		// LEAP
    0x904ac,		// LEAR
    0xa0e904acL,		// LEARN, seed word (533D: 5)
    0x19904ac,		// LEARY
    0x984ac,		// LEAS
    0xa14984acL,		// LEAST, seed word (533D: 5)
    0x19b04ac,		// LEAVY
    0x600010ac,		// LED, seed word (533D: 3)
    0x14ac,		// LEE
    0x914ac,		// LEER
    0x994ac,		// LEES
    0xa14ac,		// LEET
    0x13a14ac,		// LEETS
    0xa18ac,		// LEFT
    0x13a18ac,		// LEFTS
    0x1cac,		// LEG
    0xc09cac,		// LEGAL
    0x99cac,		// LEGS
    0x24ac,		// LEI
    0x9a4ac,		// LEIS
    0xd143b8acL,		// LENGTH, seed word (533D: 6)
    0x134b8ac,		// LENIS
    0x7b8ac,		// LENO
    0x137b8ac,		// LENOS
    0x9b8ac,		// LENS
    0xa38ac,		// LENT
    0x9ccac,		// LESS
    0xdcf9ccacL,		// LESSON, seed word (533D: 6)
    0xa4cac,		// LEST
    0x600050ac,		// LET, seed word (533D: 3)
    0x9d0ac,		// LETS
    0x54ac,		// LEU
    0x254ac,		// LEUD
    0x58ac,		// LEV
    0xd8ac,		// LEVA
    0xcd8ac,		// LEVY
    0x64ac,		// LEY
    0x12c,		// LI
    0x9052c,		// LIAR
    0x92c,		// LIB
    0x28d2c,		// LICE
    0x112c,		// LID
    0x9912c,		// LIDS
    0x6000152c,		// LIE, seed word (533D: 3)
    0x2152c,		// LIED
    0x3152c,		// LIEF
    0x2453152c,		// LIEFER
    0x7152c,		// LIEN
    0x137152c,		// LIENS
    0x9152c,		// LIER
    0x9952c,		// LIES
    0x2992c,		// LIFE
    0x122992c,		// LIFER
    0xa192c,		// LIFT
    0xc85a192cL,		// LIFTED, seed word (533D: 6)
    0x13a192c,		// LIFTS
    0xb52c,		// LIMA
    0x2b52c,		// LIME
    0x42b52c,		// LIMED
    0x132b52c,		// LIMES
    0x8352c,		// LIMP
    0x138352c,		// LIMPS
    0xcb52c,		// LIMY
    0x392c,		// LIN
    0x2b92c,		// LINE
    0x132b92c,		// LINES
    0x3b92c,		// LING
    0x133b92c,		// LINGS
    0x9b92c,		// LINS
    0xa392c,		// LINT
    0x13a392c,		// LINTS
    0x412c,		// LIP
    0x9c12c,		// LIPS
    0xc92c,		// LIRA
    0x2c92c,		// LIRE
    0x4d2c,		// LIS
    0x84d2c,		// LISP
    0x800a4d2cL,		// LIST, seed word (533D: 4)
    0xdc5a4d2cL,		// LISTEN, seed word (533D: 6)
    0x512c,		// LIT
    0x2d12c,		// LITE
    0x9d12c,		// LITS
    0x1ec,		// LO
    0x685ec,		// LOAM
    0x13685ec,		// LOAMS
    0x705ec,		// LOAN
    0x9ec,		// LOB
    0x289ec,		// LOBE
    0x40dec,		// LOCH
    0x1340dec,		// LOCHS
    0x48dec,		// LOCI
    0x78dec,		// LOCO
    0x1378dec,		// LOCOS
    0x291ec,		// LODE
    0x13995ec,		// LOESS
    0x1dec,		// LOG
    0x29dec,		// LOGE
    0x2b9ec,		// LONE
    0x3dec,		// LOO
    0x9bdec,		// LOOS
    0x41ec,		// LOP
    0x2c1ec,		// LOPE
    0x249ec,		// LORD
    0x2c9ec,		// LORE
    0x132c9ec,		// LORES
    0x8002cdecL,		// LOSE, seed word (533D: 4)
    0x122cdec,		// LOSER
    0x132cdec,		// LOSES
    0x9cdec,		// LOSS
    0xa4dec,		// LOST
    0x51ec,		// LOT
    0xd1ec,		// LOTA
    0x130d1ec,		// LOTAS
    0x9d1ec,		// LOTS
    0x255ec,		// LOUD
    0x60005dec,		// LOW, seed word (533D: 3)
    0x2ddec,		// LOWE
    0x122ddec,		// LOWER
    0x2722ddec,		// LOWERS
    0x132ddec,		// LOWES
    0x1965dec,		// LOWLY
    0x9ddec,		// LOWS
    0x59ddec,		// LOWSE
    0x292ac,		// LUDE
    0x996ac,		// LUES
    0x2baac,		// LUNE
    0x132baac,		// LUNES
    0x44eac,		// LUSH
    0x172c,		// LYE
    0x2cb2c,		// LYRE
    0x2d,		// MA
    0x2882d,		// MABE
    0x102d,		// MAD
    0x8002902dL,		// MADE, seed word (533D: 4)
    0x142d,		// MAE
    0x9942d,		// MAES
    0x1c2d,		// MAG
    0x6242d,		// MAIL
    0x8007242dL,		// MAIN, seed word (533D: 4)
    0x9242d,		// MAIR
    0x2b02d,		// MALE
    0xa302d,		// MALT
    0x13a302d,		// MALTS
    0x6000382d,		// MAN, seed word (533D: 3)
    0x2b82d,		// MANE
    0xf3b82d,		// MANGO
    0xe457382dL,		// MANNER, seed word (533D: 6)
    0x7b82d,		// MANO
    0x800cb82dL,		// MANY, seed word (533D: 4)
    0x6000402d,		// MAP, seed word (533D: 3)
    0x9c02d,		// MAPS
    0x482d,		// MAR
    0x2c82d,		// MARE
    0xae4c82d,		// MARINE
    0x4c2d,		// MAS
    0xa4c2d,		// MAST
    0x502d,		// MAT
    0x2d02d,		// MATE
    0x9d02d,		// MATS
    0x7542d,		// MAUN
    0xa542d,		// MAUT
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
    0x230ad,		// MELD
    0x13230ad,		// MELDS
    0xf247b0adL,		// MELODY, seed word (533D: 6)
    0x9b0ad,		// MELS
    0xa30ad,		// MELT
    0x34ad,		// MEM
    0x7b4ad,		// MEMO
    0x600038ad,		// MEN, seed word (533D: 3)
    0x238ad,		// MEND
    0x7b8ad,		// MENO
    0x1a38ad,		// MENTA
    0xab8ad,		// MENU
    0xbbcad,		// MEOW
    0xccad,		// MESA
    0x50ad,		// MET
    0xd0ad,		// META
    0xa0c0d0adL,		// METAL, seed word (533D: 5)
    0x5cad,		// MEW
    0x3d0d,		// MHO
    0x9bd0d,		// MHOS
    0x12d,		// MI
    0x28d2d,		// MICE
    0x112d,		// MID
    0xcac2112dL,		// MIDDLE, seed word (533D: 6)
    0x9912d,		// MIDS
    0x7152d,		// MIEN
    0x1d2d,		// MIG
    0x312d,		// MIL
    0x2312d,		// MILD
    0x2b12d,		// MILE
    0x132b12d,		// MILES
    0x9b12d,		// MILS
    0xb92d,		// MINA
    0x50b92d,		// MINAE
    0x51b92d,		// MINCE
    0x2b92d,		// MINE
    0x122b92d,		// MINER
    0x127b92d,		// MINOR
    0xa392d,		// MINT
    0x285ab92d,		// MINUET
    0xcb4ab92dL,		// MINUTE, seed word (533D: 6)
    0x492d,		// MIR
    0x2c92d,		// MIRE
    0x4d2d,		// MIS
    0x2cd2d,		// MISE
    0x8564d2d,		// MISLED
    0x2d12d,		// MITE
    0x1ad,		// MM
    0x1ed,		// MO
    0x5ed,		// MOA
    0x705ed,		// MOAN
    0x985ed,		// MOAS
    0xa05ed,		// MOAT
    0x13a05ed,		// MOATS
    0xded,		// MOC
    0x11ed,		// MOD
    0x291ed,		// MODE
    0xc291ed,		// MODEL
    0xdd2291edL,		// MODERN, seed word (533D: 6)
    0x1ded,		// MOG
    0x31ed,		// MOL
    0xb1ed,		// MOLA
    0x130b1ed,		// MOLAS
    0x231ed,		// MOLD
    0x19231ed,		// MOLDY
    0x2b1ed,		// MOLE
    0x9b1ed,		// MOLS
    0xa31ed,		// MOLT
    0x13a31ed,		// MOLTS
    0xcb1ed,		// MOLY
    0x35ed,		// MOM
    0x2b5ed,		// MOME
    0xe8e2b5edL,		// MOMENT, seed word (533D: 6)
    0x9b5ed,		// MOMS
    0x13ab5ed,		// MOMUS
    0x39ed,		// MON
    0x5239ed,		// MONDE
    0xa192b9edL,		// MONEY, seed word (533D: 5)
    0x54b9ed,		// MONIE
    0x7b9ed,		// MONO
    0x9b9ed,		// MONS
    0x5a39ed,		// MONTE
    0x8a39ed,		// MONTH
    0xe68a39edL,		// MONTHS, seed word (533D: 6)
    0xcb9ed,		// MONY
    0x3ded,		// MOO
    0x73ded,		// MOON
    0xa3ded,		// MOOT
    0x41ed,		// MOP
    0x2c1ed,		// MOPE
    0x192c1ed,		// MOPEY
    0xcc1ed,		// MOPY
    0x49ed,		// MOR
    0xc9ed,		// MORA
    0x190c9ed,		// MORAY
    0x8002c9edL,		// MORE, seed word (533D: 4)
    0x749ed,		// MORN
    0x4ded,		// MOS
    0xa4ded,		// MOST
    0x51ed,		// MOT
    0x2d1ed,		// MOTE
    0x451ed,		// MOTH
    0x13451ed,		// MOTHS
    0xdcf4d1edL,		// MOTION, seed word (533D: 6)
    0x9d1ed,		// MOTS
    0x14755ed,		// MOUNT
    0x5ded,		// MOW
    0x75ded,		// MOWN
    0x2ad,		// MU
    0x36ad,		// MUM
    0x9b6ad,		// MUMS
    0x3aad,		// MUN
    0x4baad,		// MUNI
    0x9baad,		// MUNS
    0x73ead,		// MUON
    0x1373ead,		// MUONS
    0x4ead,		// MUS
    0x52ad,		// MUT
    0x2d2ad,		// MUTE
    0xae4d2ad,		// MUTINE
    0xe7d2ad,		// MUTON
    0x32d,		// MY
    0xbb2d,		// MYNA
    0x583f2d,		// MYOPE
    0x2e,		// NA
    0x82e,		// NAB
    0x142e,		// NAE
    0x1c2e,		// NAG
    0x202e,		// NAH
    0x3242e,		// NAIF
    0x6242e,		// NAIL
    0x136242e,		// NAILS
    0x342e,		// NAM
    0x8002b42eL,		// NAME, seed word (533D: 4)
    0x122b42e,		// NAMER
    0x382e,		// NAN
    0x4bc2e,		// NAOI
    0x402e,		// NAP
    0x2c02e,		// NAPE
    0x58402e,		// NAPPE
    0x9c02e,		// NAPS
    0x2482e,		// NARD
    0xcc82e,		// NARY
    0x132d02e,		// NATES
    0xdcf4d02eL,		// NATION, seed word (533D: 6)
    0x2d82e,		// NAVE
    0x5c2e,		// NAW
    0x642e,		// NAY
    0xae,		// NE
    0x804ae,		// NEAP
    0x800904aeL,		// NEAR, seed word (533D: 4)
    0xf2c904aeL,		// NEARLY, seed word (533D: 6)
    0xa04ae,		// NEAT
    0x13a04ae,		// NEATS
    0x8ae,		// NEB
    0x14ae,		// NEE
    0x214ae,		// NEED
    0x324ae,		// NEIF
    0x149a4ae,		// NEIST
    0xb4ae,		// NEMA
    0x248ae,		// NERD
    0x9ccae,		// NESS
    0xa4cae,		// NEST
    0x50ae,		// NET
    0x9d0ae,		// NETS
    0x6d4ae,		// NEUM
    0x2d8ae,		// NEVE
    0x4d8ae,		// NEVI
    0x60005cae,		// NEW, seed word (533D: 3)
    0xa5cae,		// NEWT
    0x92e,		// NIB
    0x28d2e,		// NICE
    0x540d2e,		// NICHE
    0x26540d2e,		// NICHES
    0xc0912e,		// NIDAL
    0x2912e,		// NIDE
    0x42912e,		// NIDED
    0x132912e,		// NIDES
    0x4912e,		// NIDI
    0x312e,		// NIL
    0x9b12e,		// NILS
    0x352e,		// NIM
    0x2b92e,		// NINE
    0x412e,		// NIP
    0xc12e,		// NIPA
    0x130c12e,		// NIPAS
    0x9c12e,		// NIPS
    0x92cd2e,		// NISEI
    0x4cd2e,		// NISI
    0x512e,		// NIT
    0x2d12e,		// NITE
    0x122d12e,		// NITER
    0x132d12e,		// NITES
    0xe7d12e,		// NITON
    0x59512e,		// NITRE
    0x9d12e,		// NITS
    0x400001ee,		// NO, seed word (533D: 2)
    0x9ee,		// NOB
    0x11ee,		// NOD
    0x291ee,		// NODE
    0x13291ee,		// NODES
    0x491ee,		// NODI
    0x991ee,		// NODS
    0x13a91ee,		// NODUS
    0x615ee,		// NOEL
    0x13615ee,		// NOELS
    0x995ee,		// NOES
    0x69a15ee,		// NOETIC
    0x1dee,		// NOG
    0x99dee,		// NOGS
    0x21ee,		// NOH
    0x925ee,		// NOIR
    0x35ee,		// NOM
    0xb5ee,		// NOMA
    0x2b5ee,		// NOME
    0x97b5ee,		// NOMOI
    0x9b5ee,		// NOMS
    0xb9ee,		// NONA
    0x3dee,		// NOO
    0x2c1ee,		// NOPE
    0x600049ee,		// NOR, seed word (533D: 3)
    0x4c9ee,		// NORI
    0x6c9ee,		// NORM
    0x856c9ee,		// NORMED
    0x4dee,		// NOS
    0x8002cdeeL,		// NOSE, seed word (533D: 4)
    0x42cdee,		// NOSED
    0x132cdee,		// NOSES
    0x44dee,		// NOSH
    0x600051ee,		// NOT, seed word (533D: 3)
    0xd1ee,		// NOTA
    0x2d1ee,		// NOTE
    0x132d1ee,		// NOTES
    0xca34d1eeL,		// NOTICE, seed word (533D: 6)
    0xdad1ee,		// NOTUM
    0x9d5ee,		// NOUS
    0x60005dee,		// NOW, seed word (533D: 3)
    0x9ddee,		// NOWS
    0xa5dee,		// NOWT
    0x228e,		// NTH
    0x2ae,		// NU
    0x292ae,		// NUDE
    0x12292ae,		// NUDER
    0x13292ae,		// NUDES
    0x24aae,		// NURD
    0x4eae,		// NUS
    0x52ae,		// NUT
    0x182f,		// OAF
    0x9982f,		// OAFS
    0x482f,		// OAR
    0xa4c2f,		// OAST
    0x502f,		// OAT
    0x9d02f,		// OATS
    0x144f,		// OBE
    0x96144f,		// OBELI
    0x244f,		// OBI
    0xa44f,		// OBIA
    0xa244f,		// OBIT
    0xca74b04fL,		// OBLIGE, seed word (533D: 6)
    0xdc90d04fL,		// OBTAIN, seed word (533D: 6)
    0x46f,		// OCA
    0x9846f,		// OCAS
    0xe0d06f,		// OCTAN
    0x8f,		// OD
    0x148f,		// ODE
    0x9948f,		// ODES
    0x93c8f,		// ODOR
    0x4c8f,		// ODS
    0x6648f,		// ODYL
    0x56648f,		// ODYLE
    0xaf,		// OE
    0x4caf,		// OES
    0xcf,		// OF
    0x18cf,		// OFF
    0x12298cf,		// OFFER
    0x50cf,		// OFT
    0xa0e2d0cfL,		// OFTEN, seed word (533D: 5)
    0x122d0cf,		// OFTER
    0x684ef,		// OGAM
    0x2b0ef,		// OGLE
    0x2c8ef,		// OGRE
    0x4000010f,		// OH, seed word (533D: 2)
    0x350f,		// OHM
    0x9b50f,		// OHMS
    0x3d0f,		// OHO
    0x4d0f,		// OHS
    0x312f,		// OIL
    0x6000118f,		// OLD, seed word (533D: 3)
    0x122918f,		// OLDER
    0x9918f,		// OLDS
    0xc918f,		// OLDY
    0x158f,		// OLE
    0x958f,		// OLEA
    0x34958f,		// OLEIC
    0x9958f,		// OLES
    0x1af,		// OM
    0x715af,		// OMEN
    0x915af,		// OMER
    0xa25af,		// OMIT
    0x4daf,		// OMS
    0x1cf,		// ON
    0x28dcf,		// ONCE
    0x600015cf,		// ONE, seed word (533D: 3)
    0x995cf,		// ONES
    0x4dcf,		// ONS
    0x142cdcf,		// ONSET
    0x34d1cf,		// ONTIC
    0x7d1cf,		// ONTO
    0x9d5cf,		// ONUS
    0xc65cf,		// ONYX
    0x21ef,		// OOH
    0x9a1ef,		// OOHS
    0x51ef,		// OOT
    0x20f,		// OP
    0x160f,		// OPE
    0x2160f,		// OPED
    0x8007160fL,		// OPEN, seed word (533D: 4)
    0x4e0f,		// OPS
    0x9d60f,		// OPUS
    0x24f,		// OR
    0x64f,		// ORA
    0x2064f,		// ORAD
    0xe4f,		// ORC
    0x98e4f,		// ORCS
    0x122924f,		// ORDER
    0x7924f,		// ORDO
    0x164f,		// ORE
    0x9964f,		// ORES
    0x2b24f,		// ORLE
    0x132b24f,		// ORLES
    0x4e4f,		// ORS
    0x524f,		// ORT
    0x9d24f,		// ORTS
    0x26f,		// OS
    0x166f,		// OSE
    0x9966f,		// OSES
    0xa122a28fL,		// OTHER, seed word (533D: 5)
    0x1a68f,		// OTIC
    0x12af,		// OUD
    0x992af,		// OUDS
    0x4aaf,		// OUR
    0x9caaf,		// OURS
    0x52af,		// OUT
    0x1c16d2af,		// OUTMAN
    0x2ef,		// OW
    0x16ef,		// OWE
    0x216ef,		// OWED
    0x996ef,		// OWES
    0x32ef,		// OWL
    0x9b2ef,		// OWLS
    0x3aef,		// OWN
    0x42baef,		// OWNED
    0x122baef,		// OWNER
    0x9baef,		// OWNS
    0x2ceef,		// OWSE
    0x30f,		// OX
    0x7170f,		// OXEN
    0x670f,		// OXY
    0xdc53e70fL,		// OXYGEN, seed word (533D: 6)
    0x32f,		// OY
    0x30,		// PA
    0xc30,		// PAC
    0x28c30,		// PACE
    0x1328c30,		// PACES
    0x98c30,		// PACS
    0xa0c30,		// PACT
    0x1030,		// PAD
    0x49030,		// PADI
    0x991030,		// PADRI
    0x2030,		// PAH
    0x22430,		// PAID
    0x62430,		// PAIL
    0x1362430,		// PAILS
    0x72430,		// PAIN
    0x1372430,		// PAINS
    0xa1472430L,		// PAINT, seed word (533D: 5)
    0x92430,		// PAIR
    0x3030,		// PAL
    0x2b030,		// PALE
    0x9b030,		// PALS
    0xcb030,		// PALY
    0x3430,		// PAM
    0x9b430,		// PAMS
    0x3830,		// PAN
    0x2b830,		// PANE
    0xc2b830,		// PANEL
    0x9b830,		// PANS
    0xa3830,		// PANT
    0x4030,		// PAP
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
    0x2d030,		// PATE
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
    0xa04b0,		// PEAT
    0xcb0,		// PEC
    0x98cb0,		// PECS
    0x10b0,		// PED
    0xf910b0,		// PEDRO
    0x990b0,		// PEDS
    0x20b0,		// PEH
    0x9a0b0,		// PEHS
    0x38b0,		// PEN
    0xc0b8b0,		// PENAL
    0x73cb0,		// PEON
    0x40b0,		// PEP
    0x7c0b0,		// PEPO
    0x600048b0,		// PER, seed word (533D: 3)
    0x4c8b0,		// PERI
    0xc8f4c8b0L,		// PERIOD, seed word (533D: 6)
    0xa48b0,		// PERT
    0x4cb0,		// PES
    0x50b0,		// PET
    0x19a50b0,		// PETTY
    0x598510,		// PHASE
    0x2510,		// PHI
    0x9a510,		// PHIS
    0x130,		// PI
    0x530,		// PIA
    0x60530,		// PIAL
    0x70530,		// PIAN
    0x1370530,		// PIANS
    0x98530,		// PIAS
    0xd30,		// PIC
    0x28d30,		// PICE
    0x1530,		// PIE
    0x21530,		// PIED
    0x91530,		// PIER
    0x99530,		// PIES
    0x120b130,		// PILAR
    0x2b130,		// PILE
    0x132b130,		// PILES
    0x3930,		// PIN
    0xb930,		// PINA
    0x130b930,		// PINAS
    0x9b930,		// PINS
    0xa3930,		// PINT
    0x1a3930,		// PINTA
    0x4d30,		// PIS
    0x44d30,		// PISH
    0x5130,		// PIT
    0xd130,		// PITA
    0xa0518590L,		// PLACE, seed word (533D: 5)
    0xe48590,		// PLAIN
    0xe6e48590L,		// PLAINS, seed word (533D: 6)
    0x70590,		// PLAN
    0xa0570590L,		// PLANE, seed word (533D: 5)
    0x1370590,		// PLANS
    0x800c8590L,		// PLAY, seed word (533D: 4)
    0x9590,		// PLEA
    0x21590,		// PLED
    0x171590,		// PLENA
    0x2a590,		// PLIE
    0x132a590,		// PLIES
    0xcbd90,		// PLOY
    0x6590,		// PLY
    0x11f0,		// POD
    0x991f0,		// PODS
    0x695f0,		// POEM
    0x21f0,		// POH
    0x25f0,		// POI
    0x31f0,		// POL
    0x2b1f0,		// POLE
    0xca34b1f0L,		// POLICE, seed word (533D: 6)
    0xcb1f0,		// POLY
    0x35f0,		// POM
    0x2b5f0,		// POME
    0x239f0,		// POND
    0x13239f0,		// PONDS
    0x2b9f0,		// PONE
    0x9b9f0,		// PONS
    0x41f0,		// POP
    0x2c1f0,		// POPE
    0x2c9f0,		// PORE
    0x42c9f0,		// PORED
    0x44df0,		// POSH
    0x4755f0,		// POUND
    0xe64755f0L,		// POUNDS, seed word (533D: 6)
    0xa0650,		// PRAT
    0xc8650,		// PRAY
    0x81650,		// PREP
    0xf34a1650L,		// PRETTY, seed word (533D: 6)
    0xc9650,		// PREY
    0x522650,		// PRIDE
    0x42a650,		// PRIED
    0x3e50,		// PRO
    0x23e50,		// PROD
    0x83e50,		// PROP
    0xe4583e50L,		// PROPER, seed word (533D: 6)
    0x6650,		// PRY
    0x4a9670,		// PSEUD
    0x2670,		// PSI
    0x12b0,		// PUD
    0x992b0,		// PUDS
    0x32b0,		// PUL
    0x2b2b0,		// PULE
    0x42b2b0,		// PULED
    0x632b0,		// PULL
    0xc85632b0L,		// PULLED, seed word (533D: 6)
    0x3ab0,		// PUN
    0x9bab0,		// PUNS
    0x4eb0,		// PUS
    0x44eb0,		// PUSH
    0xc8544eb0L,		// PUSHED, seed word (533D: 6)
    0x600052b0,		// PUT, seed word (533D: 3)
    0x730,		// PYA
    0x1730,		// PYE
    0x2cb30,		// PYRE
    0x28c32,		// RACE
    0x1032,		// RAD
    0xe79032,		// RADON
    0x1c32,		// RAG
    0x29c32,		// RAGE
    0x429c32,		// RAGED
    0x529c32,		// RAGEE
    0x2032,		// RAH
    0xa432,		// RAIA
    0x22432,		// RAID
    0x62432,		// RAIL
    0x72432,		// RAIN
    0x2b032,		// RALE
    0x1963032,		// RALLY
    0xb963032,		// RALLYE
    0x3432,		// RAM
    0x4b432,		// RAMI
    0x54b432,		// RAMIE
    0x60003832,		// RAN, seed word (533D: 3)
    0x23832,		// RAND
    0x3b832,		// RANG
    0x53b832,		// RANGE
    0x853b832,		// RANGED
    0x4b832,		// RANI
    0xa3832,		// RANT
    0x4032,		// RAP
    0xa044c032L,		// RAPID, seed word (533D: 5)
    0x9c032,		// RAPS
    0xa4032,		// RAPT
    0x2c832,		// RARE
    0x42c832,		// RARED
    0x4c32,		// RAS
    0x44c32,		// RASH
    0x84c32,		// RASP
    0x5032,		// RAT
    0x8002d032L,		// RATE, seed word (533D: 4)
    0x42d032,		// RATED
    0xc2d032,		// RATEL
    0x122d032,		// RATER
    0x45032,		// RATH
    0x545032,		// RATHE
    0xe4545032L,		// RATHER, seed word (533D: 6)
    0x2d832,		// RAVE
    0xc2d832,		// RAVEL
    0x122d832,		// RAVER
    0x5c32,		// RAW
    0x6032,		// RAX
    0x6432,		// RAY
    0xe432,		// RAYA
    0x130e432,		// RAYAS
    0x42e432,		// RAYED
    0x9e432,		// RAYS
    0xb2,		// RE
    0xa08184b2L,		// REACH, seed word (533D: 5)
    0x800204b2L,		// READ, seed word (533D: 4)
    0xa19204b2L,		// READY, seed word (533D: 5)
    0x800604b2L,		// REAL, seed word (533D: 4)
    0xf2c604b2L,		// REALLY, seed word (533D: 6)
    0x684b2,		// REAM
    0x904b2,		// REAR
    0x8b2,		// REB
    0x4488b2,		// REBID
    0xcb2,		// REC
    0xe78cb2,		// RECON
    0xc9278cb2L,		// RECORD, seed word (533D: 6)
    0x98cb2,		// RECS
    0x9a0cb2,		// RECTI
    0xb3a8cb2,		// RECUSE
    0x600010b2,		// RED, seed word (533D: 3)
    0xe090b2,		// REDAN
    0x290b2,		// REDE
    0x13290b2,		// REDES
    0x1c9310b2,		// REDFIN
    0x10490b2,		// REDIP
    0x790b2,		// REDO
    0xe790b2,		// REDON
    0x990b2,		// REDS
    0x14b2,		// REE
    0x214b2,		// REED
    0x13214b2,		// REEDS
    0x314b2,		// REEF
    0x13314b2,		// REEFS
    0x614b2,		// REEL
    0x994b2,		// REES
    0x14994b2,		// REEST
    0x18b2,		// REF
    0xc298b2,		// REFEL
    0xac498b2,		// REFILE
    0x8e498b2,		// REFIND
    0x998b2,		// REFS
    0xa18b2,		// REFT
    0xcb3a98b2L,		// REFUSE, seed word (533D: 6)
    0x1cb2,		// REG
    0xc09cb2,		// REGAL
    0xc9209cb2L,		// REGARD, seed word (533D: 6)
    0xdcf49cb2L,		// REGION, seed word (533D: 6)
    0x171cb2,		// REGNA
    0x24b2,		// REI
    0x324b2,		// REIF
    0xe3a4b2,		// REIGN
    0x724b2,		// REIN
    0x9a4b2,		// REIS
    0x190b0b2,		// RELAY
    0xcc54b0b2L,		// RELIEF, seed word (533D: 6)
    0xcb0b2,		// RELY
    0x34b2,		// REM
    0xdc90b4b2L,		// REMAIN, seed word (533D: 6)
    0xe0b4b2,		// REMAN
    0xc0b8b2,		// RENAL
    0x238b2,		// REND
    0x74b8b2,		// RENIG
    0xa38b2,		// RENT
    0x5a38b2,		// RENTE
    0x40b2,		// REP
    0x7c0b2,		// REPO
    0x840b2,		// REPP
    0xf940b2,		// REPRO
    0xeac8b2,		// RERUN
    0x4cb2,		// RES
    0xb51ccb2,		// RESCUE
    0x142ccb2,		// RESET
    0x44cb2,		// RESH
    0x44ccb2,		// RESID
    0xa44ccb2,		// RESIDE
    0x2934ccb2,		// RESIST
    0x177ccb2,		// RESOW
    0xa4cb2,		// REST
    0x85a4cb2,		// RESTED
    0x13a4cb2,		// RESTS
    0x50b2,		// RET
    0x70d0b2,		// RETAG
    0x180d0b2,		// RETAX
    0x2d0b2,		// RETE
    0x2932d0b2,		// RETEST
    0x54d0b2,		// RETIE
    0xae4d0b2,		// RETINE
    0x9d0b2,		// RETS
    0xdd2ad0b2L,		// RETURN, seed word (533D: 6)
    0x59d4b2,		// REUSE
    0x58b2,		// REV
    0xe4dcb2,		// REWIN
    0xe7dcb2,		// REWON
    0x60b2,		// REX
    0x9512,		// RHEA
    0x3d12,		// RHO
    0x9bd12,		// RHOS
    0x532,		// RIA
    0x60532,		// RIAL
    0x1470532,		// RIANT
    0x932,		// RIB
    0x28d32,		// RICE
    0x428d32,		// RICED
    0x1132,		// RID
    0x29132,		// RIDE
    0x1329132,		// RIDES
    0x539132,		// RIDGE
    0x99132,		// RIDS
    0x61532,		// RIEL
    0x1932,		// RIF
    0x29932,		// RIFE
    0x561932,		// RIFLE
    0x1d32,		// RIG
    0x99d32,		// RIGS
    0x2b132,		// RILE
    0x3532,		// RIM
    0x2b532,		// RIME
    0x3932,		// RIN
    0x23932,		// RIND
    0x3b932,		// RING
    0x133b932,		// RINGS
    0x9b932,		// RINS
    0x4132,		// RIP
    0x2c132,		// RIPE
    0x42c132,		// RIPED
    0x8002cd32L,		// RISE, seed word (533D: 4)
    0x132cd32,		// RISES
    0x2d132,		// RITE
    0x132d132,		// RITES
    0x2d932,		// RIVE
    0x42d932,		// RIVED
    0xe2d932,		// RIVEN
    0x122d932,		// RIVER
    0x205f2,		// ROAD
    0x685f2,		// ROAM
    0x705f2,		// ROAN
    0xdf2,		// ROC
    0x98df2,		// ROCS
    0x11f2,		// ROD
    0x291f2,		// RODE
    0x1c5691f2,		// RODMEN
    0x15f2,		// ROE
    0x995f2,		// ROES
    0x2b1f2,		// ROLE
    0x132b1f2,		// ROLES
    0x631f2,		// ROLL
    0xc85631f2L,		// ROLLED, seed word (533D: 6)
    0x35f2,		// ROM
    0x23df2,		// ROOD
    0xa3df2,		// ROOT
    0x8002c1f2L,		// ROPE, seed word (533D: 4)
    0x42c1f2,		// ROPED
    0x122c1f2,		// ROPER
    0x8002cdf2L,		// ROSE, seed word (533D: 4)
    0x142cdf2,		// ROSET
    0xccdf2,		// ROSY
    0x51f2,		// ROT
    0x2d1f2,		// ROTE
    0x132d1f2,		// ROTES
    0x7d1f2,		// ROTO
    0x9d1f2,		// ROTS
    0x2d5f2,		// ROUE
    0x132d5f2,		// ROUES
    0xa04755f2L,		// ROUND, seed word (533D: 5)
    0x59d5f2,		// ROUSE
    0xa55f2,		// ROUT
    0x8a55f2,		// ROUTH
    0x60005df2,		// ROW, seed word (533D: 3)
    0x42ddf2,		// ROWED
    0xc2ddf2,		// ROWEL
    0x26c2ddf2,		// ROWELS
    0xe2ddf2,		// ROWEN
    0x9ddf2,		// ROWS
    0x8a5df2,		// ROWTH
    0x292b2,		// RUDE
    0x16b2,		// RUE
    0x216b2,		// RUED
    0x916b2,		// RUER
    0x996b2,		// RUES
    0x31ab2,		// RUFF
    0x531ab2,		// RUFFE
    0x26531ab2,		// RUFFES
    0x1331ab2,		// RUFFS
    0x1eb2,		// RUG
    0x726b2,		// RUIN
    0x7726b2,		// RUING
    0x60003ab2,		// RUN, seed word (533D: 3)
    0x2bab2,		// RUNE
    0x3bab2,		// RUNG
    0xa3ab2,		// RUNT
    0x2ceb2,		// RUSE
    0x52b2,		// RUT
    0x452b2,		// RUTH
    0x732,		// RYA
    0x98732,		// RYAS
    0x1732,		// RYE
    0xa3f32,		// RYOT
    0x13a3f32,		// RYOTS
    0xc33,		// SAC
    0x1033,		// SAD
    0x49033,		// SADI
    0x1433,		// SAE
    0x1c33,		// SAG
    0x80022433L,		// SAID, seed word (533D: 4)
    0x80062433L,		// SAIL, seed word (533D: 4)
    0x72433,		// SAIN
    0x2ac33,		// SAKE
    0x3033,		// SAL
    0xf320b033L,		// SALARY, seed word (533D: 6)
    0x2b033,		// SALE
    0x83033,		// SALP
    0xa3033,		// SALT
    0x8002b433L,		// SAME, seed word (533D: 4)
    0x83433,		// SAMP
    0x23833,		// SAND
    0x2b833,		// SANE
    0x4033,		// SAP
    0x60005033,		// SAT, seed word (533D: 3)
    0x2d033,		// SATE
    0x1c52d033,		// SATEEN
    0x5433,		// SAU
    0x60005c33,		// SAW, seed word (533D: 3)
    0x60006433,		// SAY, seed word (533D: 3)
    0xa0560473L,		// SCALE, seed word (533D: 5)
    0x580473,		// SCAPE
    0xa0473,		// SCAT
    0x471473,		// SCEND
    0xd8f7a073L,		// SCHOOL, seed word (533D: 6)
    0x573c73,		// SCONE
    0xa0593c73L,		// SCORE, seed word (533D: 5)
    0xa3c73,		// SCOT
    0x12abc73,		// SCOUR
    0x52c873,		// SCREE
    0x600004b3,		// SEA, seed word (533D: 3)
    0x604b3,		// SEAL
    0x684b3,		// SEAM
    0x800a04b3L,		// SEAT, seed word (533D: 4)
    0xcb3,		// SEC
    0xc8e78cb3L,		// SECOND, seed word (533D: 6)
    0xa0cb3,		// SECT
    0xcb2a8cb3L,		// SECURE, seed word (533D: 6)
    0x12290b3,		// SEDER
    0x14b3,		// SEE
    0x214b3,		// SEED
    0x614b3,		// SEEL
    0x714b3,		// SEEN
    0x914b3,		// SEER
    0x1cb3,		// SEG
    0x971cb3,		// SEGNI
    0x24b3,		// SEI
    0x324b3,		// SEIF
    0x9a4b3,		// SEIS
    0x30b3,		// SEL
    0xe832b0b3L,		// SELECT, seed word (533D: 6)
    0x330b3,		// SELF
    0x9b0b3,		// SELS
    0x4b4b3,		// SEMI
    0x38b3,		// SEN
    0xcb40b8b3L,		// SENATE, seed word (533D: 6)
    0x238b3,		// SEND
    0x2b8b3,		// SENE
    0x93b8b3,		// SENGI
    0xa38b3,		// SENT
    0x5a38b3,		// SENTE
    0x9a38b3,		// SENTI
    0x48b3,		// SER
    0x2c8b3,		// SERE
    0x42c8b3,		// SERED
    0x348b3,		// SERF
    0x177c8b3,		// SEROW
    0x9c8b3,		// SERS
    0x50b3,		// SET
    0xd0b3,		// SETA
    0x50d0b3,		// SETAE
    0xc0d0b3,		// SETAL
    0xe7d0b3,		// SETON
    0x9d0b3,		// SETS
    0xa50b3,		// SETT
    0x245a50b3,		// SETTER
    0x5cb3,		// SEW
    0x113,		// SH
    0x513,		// SHA
    0x20513,		// SHAD
    0xa0580513L,		// SHAPE, seed word (533D: 5)
    0xa1090513L,		// SHARP, seed word (533D: 5)
    0xb8513,		// SHAW
    0x60001513,		// SHE, seed word (533D: 3)
    0x9513,		// SHEA
    0x21513,		// SHED
    0xb9513,		// SHEW
    0x72513,		// SHIN
    0x572513,		// SHINE
    0x80082513L,		// SHIP, seed word (533D: 4)
    0x7b513,		// SHMO
    0x23d13,		// SHOD
    0x2bd13,		// SHOE
    0x122bd13,		// SHOER
    0x7bd13,		// SHOO
    0xc7bd13,		// SHOOL
    0x80083d13L,		// SHOP, seed word (533D: 4)
    0x593d13,		// SHORE
    0xa1493d13L,		// SHORT, seed word (533D: 5)
    0xa3d13,		// SHOT
    0x5a3d13,		// SHOTE
    0xc8cabd13L,		// SHOULD, seed word (533D: 6)
    0xbbd13,		// SHOW
    0xa0ebbd13L,		// SHOWN, seed word (533D: 5)
    0x65513,		// SHUL
    0x133,		// SI
    0x60533,		// SIAL
    0x933,		// SIB
    0xd33,		// SIC
    0x28d33,		// SICE
    0x29133,		// SIDE
    0x561133,		// SIDLE
    0xa1933,		// SIFT
    0x71d33,		// SIGN
    0x8571d33,		// SIGNED
    0x23133,		// SILD
    0x28e2b133,		// SILENT
    0xa3133,		// SILT
    0x3533,		// SIM
    0x83533,		// SIMP
    0xcac83533L,		// SIMPLE, seed word (533D: 6)
    0x3933,		// SIN
    0x51b933,		// SINCE
    0x2b933,		// SINE
    0x3b933,		// SING
    0x53b933,		// SINGE
    0x853b933,		// SINGED
    0xcac3b933L,		// SINGLE, seed word (533D: 6)
    0x43933,		// SINH
    0x4133,		// SIP
    0x2c133,		// SIPE
    0x60004933,		// SIR, seed word (533D: 3)
    0x2c933,		// SIRE
    0x42c933,		// SIRED
    0x52c933,		// SIREE
    0x132c933,		// SIRES
    0x9c933,		// SIRS
    0x4d33,		// SIS
    0xe45a4d33L,		// SISTER, seed word (533D: 6)
    0x60005133,		// SIT, seed word (533D: 3)
    0x2d133,		// SITE
    0x132d133,		// SITES
    0x9d133,		// SITS
    0x60006133,		// SIX, seed word (533D: 3)
    0x573,		// SKA
    0x81573,		// SKEP
    0xe48593,		// SLAIN
    0x68593,		// SLAM
    0x80593,		// SLAP
    0xa0593,		// SLAT
    0x5a0593,		// SLATE
    0xb8593,		// SLAW
    0xc8593,		// SLAY
    0x21593,		// SLED
    0x1429593,		// SLEET
    0xb9593,		// SLEW
    0x22593,		// SLID
    0xa0522593L,		// SLIDE, seed word (533D: 5)
    0x6a593,		// SLIM
    0x56a593,		// SLIME
    0x856a593,		// SLIMED
    0x772593,		// SLING
    0x82593,		// SLIP
    0x582593,		// SLIPE
    0xa2593,		// SLIT
    0x2bd93,		// SLOE
    0x132bd93,		// SLOES
    0xa3d93,		// SLOT
    0xbbd93,		// SLOW
    0xe45bbd93L,		// SLOWER, seed word (533D: 6)
    0x2d593,		// SLUE
    0x132d593,		// SLUES
    0x6593,		// SLY
    0x14605b3,		// SMALT
    0x1f4605b3,		// SMALTO
    0x5625b3,		// SMILE
    0xc85625b3L,		// SMILED, seed word (533D: 6)
    0x1463db3,		// SMOLT
    0xc485d3,		// SNAIL
    0x805d3,		// SNAP
    0x215d3,		// SNED
    0x5225d3,		// SNIDE
    0x825d3,		// SNIP
    0xa25d3,		// SNIT
    0x3bdd3,		// SNOG
    0x1493dd3,		// SNORT
    0xa3dd3,		// SNOT
    0x800bbdd3L,		// SNOW, seed word (533D: 4)
    0x400001f3,		// SO, seed word (533D: 2)
    0x560df3,		// SOCLE
    0x11f3,		// SOD
    0x99f3,		// SOFA
    0xa19f3,		// SOFT
    0x245a19f3,		// SOFTER
    0x31f3,		// SOL
    0xb1f3,		// SOLA
    0x231f3,		// SOLD
    0x2b1f3,		// SOLE
    0x132b1f3,		// SOLES
    0x7b1f3,		// SOLO
    0x9b1f3,		// SOLS
    0xb5f3,		// SOMA
    0x600039f3,		// SON, seed word (533D: 3)
    0x5239f3,		// SONDE
    0x2b9f3,		// SONE
    0x132b9f3,		// SONES
    0x3b9f3,		// SONG
    0x9b9f3,		// SONS
    0x41f3,		// SOP
    0x441f3,		// SOPH
    0x2c9f3,		// SORE
    0xc2c9f3,		// SOREL
    0x749f3,		// SORN
    0xa49f3,		// SORT
    0x4df3,		// SOS
    0x51f3,		// SOT
    0x451f3,		// SOTH
    0x55f3,		// SOU
    0x655f3,		// SOUL
    0xa04755f3L,		// SOUND, seed word (533D: 5)
    0x855f3,		// SOUP
    0x955f3,		// SOUR
    0xa3955f3,		// SOURCE
    0x5df3,		// SOW
    0x122ddf3,		// SOWER
    0x75df3,		// SOWN
    0x65f3,		// SOY
    0x613,		// SPA
    0xa0518613L,		// SPACE, seed word (533D: 5)
    0x28613,		// SPAE
    0xc48613,		// SPAIL
    0x558613,		// SPAKE
    0x70613,		// SPAN
    0x90613,		// SPAR
    0xa0613,		// SPAT
    0xa0b09613L,		// SPEAK, seed word (533D: 5)
    0x19613,		// SPEC
    0x21613,		// SPED
    0xc49613,		// SPEIL
    0xc2a613,		// SPIEL
    0x562613,		// SPILE
    0x72613,		// SPIN
    0x18172613,		// SPINAL
    0x25613,		// SPUD
    0x2d613,		// SPUE
    0x42d613,		// SPUED
    0x75613,		// SPUN
    0x2653,		// SRI
    0x9a653,		// SRIS
    0x38693,		// STAG
    0x560693,		// STALE
    0xa1068693L,		// STAMP, seed word (533D: 5)
    0x570693,		// STANE
    0xc09693,		// STEAL
    0x429693,		// STEED
    0xa0c29693L,		// STEEL, seed word (533D: 5)
    0x1229693,		// STEER
    0xe49693,		// STEIN
    0x161693,		// STELA
    0x561693,		// STELE
    0xf71693,		// STENO
    0x591693,		// STERE
    0xa1693,		// STET
    0x132a693,		// STIES
    0xac32693,		// STIFLE
    0x562693,		// STILE
    0x772693,		// STING
    0x92693,		// STIR
    0x1392693,		// STIRS
    0xbe93,		// STOA
    0xa0563e93L,		// STOLE, seed word (533D: 5)
    0x16be93,		// STOMA
    0x1816be93,		// STOMAL
    0xa0573e93L,		// STONE, seed word (533D: 5)
    0xa0593e93L,		// STORE, seed word (533D: 5)
    0xa1993e93L,		// STORY, seed word (533D: 5)
    0xe852ca93L,		// STREET, seed word (533D: 6)
    0xcee4ca93L,		// STRING, seed word (533D: 6)
    0xcee7ca93L,		// STRONG, seed word (533D: 6)
    0x197ca93,		// STROY
    0x6693,		// STY
    0x590eb3,		// SUCRE
    0x212b3,		// SUDD
    0xdc5212b3L,		// SUDDEN, seed word (533D: 6)
    0x16b3,		// SUE
    0x216b3,		// SUED
    0x916b3,		// SUER
    0x996b3,		// SUES
    0xe4531ab3L,		// SUFFER, seed word (533D: 6)
    0x600036b3,		// SUM, seed word (533D: 3)
    0xdcf6b6b3L,		// SUMMON, seed word (533D: 6)
    0x7b6b3,		// SUMO
    0x60003ab3,		// SUN, seed word (533D: 3)
    0x9bab3,		// SUNS
    0x42b3,		// SUP
    0x2c2b3,		// SUPE
    0x8002cab3L,		// SURE, seed word (533D: 4)
    0x34ab3,		// SURF
    0xc86f3,		// SWAY
    0x593ef3,		// SWORE
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
    0x99c34,		// TAGS
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
    0x9b834,		// TANS
    0x3c34,		// TAO
    0x9bc34,		// TAOS
    0x4034,		// TAP
    0x2c034,		// TAPE
    0x9c034,		// TAPS
    0x4834,		// TAR
    0x2c834,		// TARE
    0x42c834,		// TARED
    0x53c834,		// TARGE
    0x74834,		// TARN
    0x84834,		// TARP
    0x594834,		// TARRE
    0x4c34,		// TAS
    0x5434,		// TAU
    0x9d434,		// TAUS
    0x5834,		// TAV
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
    0x5984b4,		// TEASE
    0x10b4,		// TED
    0x990b4,		// TEDS
    0x14b4,		// TEE
    0x214b4,		// TEED
    0x614b4,		// TEEL
    0x13614b4,		// TEELS
    0x714b4,		// TEEN
    0x13714b4,		// TEENS
    0x994b4,		// TEES
    0x318b4,		// TEFF
    0x1cb4,		// TEG
    0x4724b4,		// TEIND
    0x30b4,		// TEL
    0xb0b4,		// TELA
    0x2b0b4,		// TELE
    0x132b0b4,		// TELES
    0x137b0b4,		// TELOS
    0x9b0b4,		// TELS
    0x600038b4,		// TEN, seed word (533D: 3)
    0x238b4,		// TEND
    0x9b8b4,		// TENS
    0x59b8b4,		// TENSE
    0xc0b4,		// TEPA
    0x13c8b4,		// TERGA
    0x748b4,		// TERN
    0x5748b4,		// TERNE
    0x1948b4,		// TERRA
    0x59c8b4,		// TERSE
    0x164cb4,		// TESLA
    0xa4cb4,		// TEST
    0x245a4cb4,		// TESTER
    0x50b4,		// TET
    0x9d0b4,		// TETS
    0x5cb4,		// TEW
    0x28514,		// THAE
    0x60001514,		// THE, seed word (533D: 3)
    0x119514,		// THECA
    0x29514,		// THEE
    0xe39514,		// THEGN
    0xa1249514L,		// THEIR, seed word (533D: 5)
    0x71514,		// THEN
    0x591514,		// THERE
    0xb9514,		// THEW
    0x83a514,		// THIGH
    0x92514,		// THIR
    0x3d14,		// THO
    0xa059bd14L,		// THOSE, seed word (533D: 5)
    0xabd14,		// THOU
    0x52c914,		// THREE
    0x7c914,		// THRO
    0x57c914,		// THROE
    0x177c914,		// THROW
    0xac914,		// THRU
    0x134,		// TI
    0xd34,		// TIC
    0x98d34,		// TICS
    0x29134,		// TIDE
    0x1534,		// TIE
    0x80021534L,		// TIED, seed word (533D: 4)
    0x91534,		// TIER
    0x1391534,		// TIERS
    0x99534,		// TIES
    0x3134,		// TIL
    0x523134,		// TILDE
    0x2b134,		// TILE
    0x42b134,		// TILED
    0x132b134,		// TILES
    0x9b134,		// TILS
    0x2b534,		// TIME
    0x3934,		// TIN
    0x2b934,		// TINE
    0x42b934,		// TINED
    0x132b934,		// TINES
    0x3b934,		// TING
    0x133b934,		// TINGS
    0x8573934,		// TINNED
    0x9b934,		// TINS
    0x1859b934,		// TINSEL
    0x4134,		// TIP
    0x2c934,		// TIRE
    0x42c934,		// TIRED
    0x132c934,		// TIRES
    0x4d34,		// TIS
    0x1f4,		// TO
    0x205f4,		// TOAD
    0x19205f4,		// TOADY
    0x11f4,		// TOD
    0xa19091f4L,		// TODAY, seed word (533D: 5)
    0xc91f4,		// TODY
    0x15f4,		// TOE
    0x995f4,		// TOES
    0x319f4,		// TOFF
    0xa99f4,		// TOFU
    0x1df4,		// TOG
    0x99df4,		// TOGS
    0xb1f4,		// TOLA
    0x130b1f4,		// TOLAS
    0x2b1f4,		// TOLE
    0x132b1f4,		// TOLES
    0x35f4,		// TOM
    0xe0b5f4,		// TOMAN
    0x2b5f4,		// TOME
    0x9b5f4,		// TOMS
    0x39f4,		// TON
    0x8002b9f4L,		// TONE, seed word (533D: 4)
    0x132b9f4,		// TONES
    0x3b9f4,		// TONG
    0x133b9f4,		// TONGS
    0x34b9f4,		// TONIC
    0x9b9f4,		// TONS
    0x3df4,		// TOO
    0x6bdf4,		// TOOM
    0x73df4,		// TOON
    0x49f4,		// TOR
    0x1c9f4,		// TORC
    0x2c9f4,		// TORE
    0x132c9f4,		// TORES
    0x749f4,		// TORN
    0x7c9f4,		// TORO
    0x9c9f4,		// TORS
    0x59c9f4,		// TORSE
    0xcc9f4,		// TORY
    0x44df4,		// TOSH
    0x955f4,		// TOUR
    0x5df4,		// TOW
    0x122ddf4,		// TOWER
    0x80075df4L,		// TOWN, seed word (533D: 4)
    0x65f4,		// TOY
    0x9e5f4,		// TOYS
    0x20654,		// TRAD
    0xa0520654L,		// TRADE, seed word (533D: 5)
    0xa0e48654L,		// TRAIN, seed word (533D: 5)
    0x80654,		// TRAP
    0x5b0654,		// TRAVE
    0xd85b0654L,		// TRAVEL, seed word (533D: 6)
    0xc8654,		// TRAY
    0x409654,		// TREAD
    0x29654,		// TREE
    0x429654,		// TREED
    0xe29654,		// TREEN
    0x1329654,		// TREES
    0x31654,		// TREF
    0x1399654,		// TRESS
    0xa1654,		// TRET
    0x13a1654,		// TRETS
    0xc9654,		// TREY
    0x51a654,		// TRICE
    0x851a654,		// TRICED
    0x42a654,		// TRIED
    0xae2a654,		// TRIENE
    0x132a654,		// TRIES
    0x3a654,		// TRIG
    0x133a654,		// TRIGS
    0x572654,		// TRINE
    0x23e54,		// TROD
    0xbbe54,		// TROW
    0xcbe54,		// TROY
    0x13cbe54,		// TROYS
    0x2d654,		// TRUE
    0x122d654,		// TRUER
    0x6654,		// TRY
    0xab4,		// TUB
    0x8ab4,		// TUBA
    0x1eb4,		// TUG
    0x99eb4,		// TUGS
    0x26b4,		// TUI
    0x3ab4,		// TUN
    0xbab4,		// TUNA
    0x2bab4,		// TUNE
    0x122bab4,		// TUNER
    0x42b4,		// TUP
    0x34ab4,		// TURF
    0x74ab4,		// TURN
    0x24574ab4,		// TURNER
    0x6f4,		// TWA
    0x286f4,		// TWAE
    0x122a6f4,		// TWIER
    0x3a6f4,		// TWIG
    0x726f4,		// TWIN
    0x5726f4,		// TWINE
    0x245726f4,		// TWINER
    0x60003ef4,		// TWO, seed word (533D: 3)
    0x1734,		// TYE
    0x91734,		// TYER
    0x2c334,		// TYPE
    0x2cb34,		// TYRE
    0x7cb34,		// TYRO
    0x137cb34,		// TYROS
    0x3c95,		// UDO
    0x9bc95,		// UDOS
    0x20f5,		// UGH
    0x115,		// UH
    0x1b5,		// UM
    0x35b5,		// UMM
    0x1d5,		// UN
    0x291d5,		// UNDE
    0xa12291d5L,		// UNDER, seed word (533D: 5)
    0x791d5,		// UNDO
    0x9249dd5,		// UNGIRD
    0xa25d5,		// UNIT
    0x5a25d5,		// UNITE
    0xe732b1d5L,		// UNLESS, seed word (533D: 6)
    0x142b5d5,		// UNMET
    0x74c9d5,		// UNRIG
    0x4dd5,		// UNS
    0x54d1d5,		// UNTIE
    0x7d1d5,		// UNTO
    0x215,		// UP
    0x79215,		// UPDO
    0x1379215,		// UPDOS
    0x3e15,		// UPO
    0x73e15,		// UPON
    0x4e15,		// UPS
    0x1255,		// URD
    0x29e55,		// URGE
    0x3a55,		// URN
    0x275,		// US
    0x60001675,		// USE, seed word (533D: 3)
    0x21675,		// USED
    0x91675,		// USER
    0x99675,		// USES
    0x295,		// UT
    0x695,		// UTA
    0x98695,		// UTAS
    0x4e95,		// UTS
    0x96d5,		// UVEA
    0x92436,		// VAIR
    0x2b036,		// VALE
    0x142b036,		// VALET
    0xf2563036L,		// VALLEY, seed word (533D: 6)
    0x3836,		// VAN
    0x2b836,		// VANE
    0x4836,		// VAR
    0x2454c836,		// VARIER
    0x28564836,		// VARLET
    0x5036,		// VAT
    0x5436,		// VAU
    0x604b6,		// VEAL
    0x19604b6,		// VEALY
    0x14b6,		// VEE
    0x1714b6,		// VEENA
    0x724b6,		// VEIN
    0xb0b6,		// VELA
    0x120b0b6,		// VELAR
    0xb8b6,		// VENA
    0x50b8b6,		// VENAE
    0x238b6,		// VEND
    0x5ab8b6,		// VENUE
    0xc8b6,		// VERA
    0x1c9248b6,		// VERDIN
    0xa48b6,		// VERT
    0x50b6,		// VET
    0x536,		// VIA
    0x29136,		// VIDE
    0x1536,		// VIE
    0x21536,		// VIED
    0x91536,		// VIER
    0x2b936,		// VINE
    0x42b936,		// VINED
    0x1437,		// WAE
    0xcb037,		// WALY
    0x3837,		// WAN
    0x60004837,		// WAR, seed word (533D: 3)
    0x2c837,		// WARE
    0xa4837,		// WART
    0x4c37,		// WAS
    0x80044c37L,		// WASH, seed word (533D: 4)
    0x5037,		// WAT
    0xa122d037L,		// WATER, seed word (533D: 5)
    0x60006437,		// WAY, seed word (533D: 3)
    0x9e437,		// WAYS
    0xb7,		// WE
    0x800904b7L,		// WEAR, seed word (533D: 4)
    0x10b7,		// WED
    0x14b7,		// WEE
    0x614b7,		// WEEL
    0x994b7,		// WEES
    0x83a4b7,		// WEIGH
    0xe883a4b7L,		// WEIGHT, seed word (533D: 6)
    0x924b7,		// WEIR
    0x630b7,		// WELL
    0x19630b7,		// WELLY
    0x89b0b7,		// WELSH
    0x38b7,		// WEN
    0x238b7,		// WEND
    0xa38b7,		// WENT
    0xa48b7,		// WERT
    0x50b7,		// WET
    0x517,		// WHA
    0x29517,		// WHEE
    0xc29517,		// WHEEL
    0xe6c29517L,		// WHEELS, seed word (533D: 6)
    0xa1517,		// WHET
    0x3a517,		// WHIG
    0xa2517,		// WHIT
    0x5a2517,		// WHITE
    0x60003d17,		// WHO, seed word (533D: 3)
    0x1493d17,		// WHORT
    0xa059bd17L,		// WHOSE, seed word (533D: 5)
    0x1d37,		// WIG
    0x1441d37,		// WIGHT
    0x3937,		// WIN
    0x2b937,		// WINE
    0xe45a3937L,		// WINTER, seed word (533D: 6)
    0x2c937,		// WIRE
    0x5137,		// WIT
    0x2d137,		// WITE
    0x45137,		// WITH
    0x545137,		// WITHE
    0x1f7,		// WO
    0x15f7,		// WOE
    0x995f7,		// WOES
    0xa0e0b5f7L,		// WOMAN, seed word (533D: 5)
    0xa0e2b5f7L,		// WOMEN, seed word (533D: 5)
    0x39f7,		// WON
    0xe45239f7L,		// WONDER, seed word (533D: 6)
    0x9b9f7,		// WONS
    0xa39f7,		// WONT
    0x249f7,		// WORD
    0x2c9f7,		// WORE
    0x749f7,		// WORN
    0x59c9f7,		// WORSE
    0xa49f7,		// WORT
    0xa08a49f7L,		// WORTH, seed word (533D: 5)
    0x4df7,		// WOS
    0x51f7,		// WOT
    0x71657,		// WREN
    0xa2657,		// WRIT
    0x5a2657,		// WRITE
    0xa05a3e57L,		// WROTE, seed word (533D: 5)
    0x8a3e57,		// WROTH
    0x1737,		// WYE
    0x2b337,		// WYLE
    0x138,		// XI
    0x4d38,		// XIS
    0x39,		// YA
    0x492439,		// YAIRD
    0x23039,		// YALD
    0x3439,		// YAM
    0x4039,		// YAP
    0x4839,		// YAR
    0x24839,		// YARD
    0x2c839,		// YARE
    0x74839,		// YARN
    0x5c39,		// YAW
    0x65c39,		// YAWL
    0x1365c39,		// YAWLS
    0x9dc39,		// YAWS
    0xb9,		// YE
    0x4b9,		// YEA
    0x704b9,		// YEAN
    0x800904b9L,		// YEAR, seed word (533D: 4)
    0xe904b9,		// YEARN
    0x984b9,		// YEAS
    0x230b9,		// YELD
    0x630b9,		// YELL
    0xeef630b9L,		// YELLOW, seed word (533D: 6)
    0x830b9,		// YELP
    0x38b9,		// YEN
    0x40b9,		// YEP
    0x4cb9,		// YES
    0x600050b9,		// YET, seed word (533D: 3)
    0xa50b9,		// YETT
    0x5cb9,		// YEW
    0x1139,		// YID
    0x24939,		// YIRD
    0x69599,		// YLEM
    0x1f9,		// YO
    0x9f9,		// YOB
    0x11f9,		// YOD
    0xc291f9,		// YODEL
    0x5611f9,		// YODLE
    0x35f9,		// YOM
    0x39f9,		// YON
    0x5df9,		// YOW
    0x2ddf9,		// YOWE
    0x65df9,		// YOWL

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
        if (((sList[i] >> 29) & 0x7) == NUM_CUBES)
        {
            char word[MAX_LETTERS_PER_WORD + 1];
            if (!bitsToString(sList[i], word))
            {
                ASSERT(0);
                return false;
            }

            ASSERT(strlen(word) == MAX_LETTERS_PER_WORD);
            strcpy(buffer, word);
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
