#include "Dictionary.h"
//#include "TrieNode.h"
#include "PrototypeWordList.h"
//extern TrieNode root;
#include "EventID.h"
#include "EventData.h"
#include "WordGame.h"

char Dictionary::sOldWords[MAX_OLD_WORDS][MAX_LETTERS_PER_WORD + 1];
unsigned Dictionary::sNumOldWords = 0;
unsigned Dictionary::sRandSeed = 0;
unsigned Dictionary::sRound = 0;
unsigned Dictionary::sPickIndex = 0;
const unsigned WORD_RAND_SEED_INCREMENT = 88;
const unsigned DEMO_MAX_DETERMINISTIC_ROUNDS = 5;

const static char* picks[] =
{
    "ROYAL",
    "TERMS",
    "TRULY",
    "PRICE",
    "WITHIN",
    "FAVOR",
    "AFFAIR",
    "TOUCH",
    "SERVE",
    "BESIDE",
    "UNLESS",
    "THANK",
    "MIDDLE",
    "VESSEL",
    "TURNED",
    "WHILE",
    "MEMBER",
    "SECOND",
    "FIGHT",
    "NOTICE",
    "OCCUPY",
    "BECOME",
    "WINDOW",
    "GRAND",
    "DURING",
    "VOICE",
    "SPEND",
    "OUGHT",
    "MOTION",
    "ISSUE",
    "FRONT",
    "TOTAL",
    "SHOWN",
    "FIGURE",
    "FIRST",
    "READY",
    "UNABLE",
    "RETURN",
    "STAND",
    "HONOR",
    "BELONG",
    "ORDER",
    "PASSED",
    "FINISH",
    "AHEAD",
    "VISIT",
    "SPENT",
    "EXTRA",
    "CAREER",
    "DROWN",
    "SENSE",
    "THESE",
    "SPRING",
    "STONE",
    "CORNER",
    "HEARD",
    "RETIRE",
    "PUSHED",
    "THING",
    "WINTER",
    "ISLAND",
    "SHALL",
    "WAVES",
    "AWFUL",
    "EMPIRE",
    "REACH",
    "HOURS",
    "BECAME",
    "STRING",
    "NEARLY",
    "ALLOW",
    "ELECT",
    "BEING",
    "TOWARD",
    "INFORM",
    "DELAY",
    "WOMAN",
    "EVERY",
    "HUMAN",
    "FORTY",
    "PARTY",
    "CAUSE",
    "HOUSE",
    "AFTER",
    "WATER",
    "BEGUN",
    "INCOME",
    "ABOUT",
    "STOLE",
    "GREAT",
    "DEATH",
    "GUEST",
    "DESIGN",
    "SUFFER",
    "CENTS",
    "FILLED",
    "BROWN",
    "UNTIL",
    "OFFER",
    "FOURTH",
    "ALONE",
    "SUMMER",
    "SMALL",
    "WORTH",
    "SHAPE",
    "KNOWN",
    "WOMEN",
    "LESSON",
    "DIVIDE",
    "YOUNG",
    "SINGLE",
    "LEARN",
    "OFTEN",
    "NEVER",
    "MOTHER",
    "BELOW",
    "OTHER",
    "THINK",
    "PLANT",
    "PLANE",
    "REPAIR",
    "OBTAIN",
    "ASSURE",
    "BUILT",
    "SELECT",
    "REALLY",
    "CENTER",
    "DOCTOR",
    "SIGHT",
    "RESULT",
    "UNDER",
    "PAPER",
    "RAISE",
    "HEART",
    "COLOR",
    "WORDS",
    "STILL",
    "EVENT",
    "DOUBT",
    "CALLED",
    "MEANT",
    "INSIDE",
    "PEOPLE",
    "REMAIN",
    "LEAST",
    "CHART",
    "THOSE",
    "STAMP",
    "TODAY",
    "THREE",
    "COAST",
    "SPREAD",
    "ENTIRE",
    "FOREST",
    "METAL",
    "FIELD",
    "ESCAPE",
    "LISTEN",
    "TABLE",
    "LATER",
    "ALMOST",
    "SISTER",
    "RELIEF",
    "STATE",
    "STREET",
    "STREAM",
    "DESIRE",
    "START",
    "ARREST",

};

const static unsigned char pickAnagrams[] =
{
    2,	// ROYAL, nonbonus anagrams: ['ROYAL', 'LAY']
    3,	// TERMS, nonbonus anagrams: ['TERM', 'SET', 'TERMS']
    2,	// TRULY, nonbonus anagrams: ['TRY', 'TRULY']
    2,	// PRICE, nonbonus anagrams: ['PRICE', 'ICE']
    3,	// WITHIN, nonbonus anagrams: ['WITHIN', 'WITH', 'THIN']
    2,	// FAVOR, nonbonus anagrams: ['FAR', 'FAVOR']
    2,	// AFFAIR, nonbonus anagrams: ['AFFAIR', 'FAIR']
    2,	// TOUCH, nonbonus anagrams: ['TOUCH', 'HOT']
    2,	// SERVE, nonbonus anagrams: ['SEE', 'SERVE']
    2,	// BESIDE, nonbonus anagrams: ['BESIDE', 'SIDE']
    2,	// UNLESS, nonbonus anagrams: ['UNLESS', 'LESS']
    2,	// THANK, nonbonus anagrams: ['THAN', 'THANK']
    2,	// MIDDLE, nonbonus anagrams: ['MIDDLE', 'MILE']
    2,	// VESSEL, nonbonus anagrams: ['VESSEL', 'LESS']
    2,	// TURNED, nonbonus anagrams: ['TURN', 'TURNED']
    2,	// WHILE, nonbonus anagrams: ['LIE', 'WHILE']
    2,	// MEMBER, nonbonus anagrams: ['MEMBER', 'MERE']
    2,	// SECOND, nonbonus anagrams: ['SECOND', 'SEND']
    2,	// FIGHT, nonbonus anagrams: ['FIT', 'FIGHT']
    2,	// NOTICE, nonbonus anagrams: ['NOTICE', 'ONCE']
    2,	// OCCUPY, nonbonus anagrams: ['OCCUPY', 'COPY']
    2,	// BECOME, nonbonus anagrams: ['BECOME', 'COME']
    2,	// WINDOW, nonbonus anagrams: ['WINDOW', 'WIND']
    2,	// GRAND, nonbonus anagrams: ['AND', 'GRAND']
    2,	// DURING, nonbonus anagrams: ['DURING', 'RING']
    2,	// VOICE, nonbonus anagrams: ['VOICE', 'ICE']
    2,	// SPEND, nonbonus anagrams: ['END', 'SPEND']
    2,	// OUGHT, nonbonus anagrams: ['OUGHT', 'OUT']
    3,	// MOTION, nonbonus anagrams: ['MOTION', 'OMIT', 'MOON']
    2,	// ISSUE, nonbonus anagrams: ['ISSUE', 'USE']
    2,	// FRONT, nonbonus anagrams: ['NOT', 'FRONT']
    2,	// TOTAL, nonbonus anagrams: ['TOTAL', 'LOT']
    4,	// SHOWN, nonbonus anagrams: ['SHOWN', 'OWN', 'NOW', 'SHOW']
    2,	// FIGURE, nonbonus anagrams: ['FIRE', 'FIGURE']
    2,	// FIRST, nonbonus anagrams: ['FIT', 'FIRST']
    3,	// READY, nonbonus anagrams: ['READ', 'READY', 'DAY']
    2,	// UNABLE, nonbonus anagrams: ['UNABLE', 'ABLE']
    2,	// RETURN, nonbonus anagrams: ['TURN', 'RETURN']
    2,	// STAND, nonbonus anagrams: ['AND', 'STAND']
    2,	// HONOR, nonbonus anagrams: ['NOR', 'HONOR']
    2,	// BELONG, nonbonus anagrams: ['BELONG', 'LONG']
    2,	// ORDER, nonbonus anagrams: ['ORDER', 'RED']
    2,	// PASSED, nonbonus anagrams: ['PASSED', 'PASS']
    2,	// FINISH, nonbonus anagrams: ['FISH', 'FINISH']
    2,	// AHEAD, nonbonus anagrams: ['HAD', 'AHEAD']
    2,	// VISIT, nonbonus anagrams: ['VISIT', 'SIT']
    2,	// SPENT, nonbonus anagrams: ['SPENT', 'TEN']
    2,	// EXTRA, nonbonus anagrams: ['EXTRA', 'ART']
    2,	// CAREER, nonbonus anagrams: ['CAREER', 'CARE']
    4,	// DROWN, nonbonus anagrams: ['DROWN', 'OWN', 'WORD', 'NOW']
    2,	// SENSE, nonbonus anagrams: ['SEE', 'SENSE']
    3,	// THESE, nonbonus anagrams: ['THESE', 'THE', 'SEE']
    2,	// SPRING, nonbonus anagrams: ['SPRING', 'RING']
    2,	// STONE, nonbonus anagrams: ['STONE', 'ONE']
    2,	// CORNER, nonbonus anagrams: ['CORN', 'CORNER']
    2,	// HEARD, nonbonus anagrams: ['HEARD', 'HEAR']
    2,	// RETIRE, nonbonus anagrams: ['RETIRE', 'TIRE']
    3,	// PUSHED, nonbonus anagrams: ['PUSH', 'PUSHED', 'SHED']
    2,	// THING, nonbonus anagrams: ['THIN', 'THING']
    2,	// WINTER, nonbonus anagrams: ['WIRE', 'WINTER']
    2,	// ISLAND, nonbonus anagrams: ['LAND', 'ISLAND']
    2,	// SHALL, nonbonus anagrams: ['ALL', 'SHALL']
    3,	// WAVES, nonbonus anagrams: ['SAW', 'WAVES', 'WAS']
    2,	// AWFUL, nonbonus anagrams: ['AWFUL', 'LAW']
    2,	// EMPIRE, nonbonus anagrams: ['EMPIRE', 'MERE']
    3,	// REACH, nonbonus anagrams: ['CARE', 'REACH', 'HER']
    2,	// HOURS, nonbonus anagrams: ['HOURS', 'HOUR']
    2,	// BECAME, nonbonus anagrams: ['BECAME', 'CAME']
    2,	// STRING, nonbonus anagrams: ['RING', 'STRING']
    2,	// NEARLY, nonbonus anagrams: ['NEAR', 'NEARLY']
    3,	// ALLOW, nonbonus anagrams: ['LAW', 'LOW', 'ALLOW']
    2,	// ELECT, nonbonus anagrams: ['LET', 'ELECT']
    2,	// BEING, nonbonus anagrams: ['BEING', 'BEG']
    2,	// TOWARD, nonbonus anagrams: ['TOWARD', 'DRAW']
    2,	// INFORM, nonbonus anagrams: ['FORM', 'INFORM']
    3,	// DELAY, nonbonus anagrams: ['LAY', 'DEAL', 'DELAY']
    4,	// WOMAN, nonbonus anagrams: ['WOMAN', 'OWN', 'NOW', 'MAN']
    2,	// EVERY, nonbonus anagrams: ['EVERY', 'EVER']
    2,	// HUMAN, nonbonus anagrams: ['HUMAN', 'MAN']
    2,	// FORTY, nonbonus anagrams: ['TRY', 'FORTY']
    4,	// PARTY, nonbonus anagrams: ['PAY', 'TRY', 'PART', 'PARTY']
    2,	// CAUSE, nonbonus anagrams: ['USE', 'CAUSE']
    2,	// HOUSE, nonbonus anagrams: ['HOUSE', 'USE']
    2,	// AFTER, nonbonus anagrams: ['FAR', 'AFTER']
    2,	// WATER, nonbonus anagrams: ['WATER', 'WAR']
    2,	// BEGUN, nonbonus anagrams: ['GUN', 'BEGUN']
    2,	// INCOME, nonbonus anagrams: ['COME', 'INCOME']
    2,	// ABOUT, nonbonus anagrams: ['ABOUT', 'OUT']
    2,	// STOLE, nonbonus anagrams: ['STOLE', 'LOST']
    2,	// GREAT, nonbonus anagrams: ['GREAT', 'EAT']
    2,	// DEATH, nonbonus anagrams: ['DEATH', 'HAT']
    2,	// GUEST, nonbonus anagrams: ['SET', 'GUEST']
    4,	// DESIGN, nonbonus anagrams: ['DESIGN', 'SIGN', 'SING', 'SIDE']
    2,	// SUFFER, nonbonus anagrams: ['SUFFER', 'SURE']
    2,	// CENTS, nonbonus anagrams: ['CENTS', 'CENT']
    2,	// FILLED, nonbonus anagrams: ['FILLED', 'FILL']
    3,	// BROWN, nonbonus anagrams: ['BROWN', 'OWN', 'NOW']
    2,	// UNTIL, nonbonus anagrams: ['UNTIL', 'UNIT']
    2,	// OFFER, nonbonus anagrams: ['FOR', 'OFFER']
    2,	// FOURTH, nonbonus anagrams: ['FOUR', 'FOURTH']
    2,	// ALONE, nonbonus anagrams: ['ALONE', 'ONE']
    2,	// SUMMER, nonbonus anagrams: ['SUMMER', 'SURE']
    2,	// SMALL, nonbonus anagrams: ['SMALL', 'ALL']
    2,	// WORTH, nonbonus anagrams: ['WORTH', 'HOW']
    2,	// SHAPE, nonbonus anagrams: ['SHAPE', 'SHE']
    4,	// KNOWN, nonbonus anagrams: ['OWN', 'KNOW', 'KNOWN', 'NOW']
    4,	// WOMEN, nonbonus anagrams: ['OWN', 'MEN', 'NOW', 'WOMEN']
    2,	// LESSON, nonbonus anagrams: ['LESSON', 'LESS']
    2,	// DIVIDE, nonbonus anagrams: ['DIED', 'DIVIDE']
    2,	// YOUNG, nonbonus anagrams: ['YOUNG', 'GUN']
    3,	// SINGLE, nonbonus anagrams: ['SING', 'SINGLE', 'SIGN']
    2,	// LEARN, nonbonus anagrams: ['RAN', 'LEARN']
    2,	// OFTEN, nonbonus anagrams: ['TEN', 'OFTEN']
    2,	// NEVER, nonbonus anagrams: ['EVEN', 'NEVER']
    2,	// MOTHER, nonbonus anagrams: ['MOTHER', 'MORE']
    2,	// BELOW, nonbonus anagrams: ['BELOW', 'LOW']
    2,	// OTHER, nonbonus anagrams: ['OTHER', 'HER']
    2,	// THINK, nonbonus anagrams: ['THINK', 'THIN']
    2,	// PLANT, nonbonus anagrams: ['PLANT', 'PLAN']
    2,	// PLANE, nonbonus anagrams: ['PLANE', 'PLAN']
    2,	// REPAIR, nonbonus anagrams: ['PAIR', 'REPAIR']
    2,	// OBTAIN, nonbonus anagrams: ['OBTAIN', 'BOAT']
    2,	// ASSURE, nonbonus anagrams: ['ASSURE', 'SURE']
    2,	// BUILT, nonbonus anagrams: ['BUILT', 'BUT']
    2,	// SELECT, nonbonus anagrams: ['SELECT', 'ELSE']
    2,	// REALLY, nonbonus anagrams: ['REAL', 'REALLY']
    3,	// CENTER, nonbonus anagrams: ['CENTER', 'CENT', 'RECENT']
    2,	// DOCTOR, nonbonus anagrams: ['DOOR', 'DOCTOR']
    2,	// SIGHT, nonbonus anagrams: ['SIGHT', 'SIT']
    2,	// RESULT, nonbonus anagrams: ['RESULT', 'SURE']
    3,	// UNDER, nonbonus anagrams: ['RUN', 'UNDER', 'RED']
    2,	// PAPER, nonbonus anagrams: ['PAPER', 'PER']
    2,	// RAISE, nonbonus anagrams: ['RAISE', 'ARE']
    4,	// HEART, nonbonus anagrams: ['HEART', 'ART', 'HEAR', 'THE']
    2,	// COLOR, nonbonus anagrams: ['COLOR', 'COOL']
    2,	// WORDS, nonbonus anagrams: ['WORDS', 'WORD']
    2,	// STILL, nonbonus anagrams: ['STILL', 'LIST']
    3,	// EVENT, nonbonus anagrams: ['EVEN', 'TEN', 'EVENT']
    2,	// DOUBT, nonbonus anagrams: ['DOUBT', 'BUT']
    2,	// CALLED, nonbonus anagrams: ['CALLED', 'CALL']
    3,	// MEANT, nonbonus anagrams: ['MEANT', 'NAME', 'MEAN']
    2,	// INSIDE, nonbonus anagrams: ['INSIDE', 'SIDE']
    2,	// PEOPLE, nonbonus anagrams: ['PEOPLE', 'POLE']
    2,	// REMAIN, nonbonus anagrams: ['REMAIN', 'MAIN']
    3,	// LEAST, nonbonus anagrams: ['LEAST', 'LET', 'SAT']
    2,	// CHART, nonbonus anagrams: ['ART', 'CHART']
    2,	// THOSE, nonbonus anagrams: ['THE', 'THOSE']
    2,	// STAMP, nonbonus anagrams: ['MAP', 'STAMP']
    2,	// TODAY, nonbonus anagrams: ['DAY', 'TODAY']
    3,	// THREE, nonbonus anagrams: ['THERE', 'THREE', 'THE']
    2,	// COAST, nonbonus anagrams: ['COAST', 'SAT']
    2,	// SPREAD, nonbonus anagrams: ['READ', 'SPREAD']
    2,	// ENTIRE, nonbonus anagrams: ['ENTIRE', 'TIRE']
    2,	// FOREST, nonbonus anagrams: ['REST', 'FOREST']
    2,	// METAL, nonbonus anagrams: ['MEAT', 'METAL']
    3,	// FIELD, nonbonus anagrams: ['LED', 'FIELD', 'FILE']
    2,	// ESCAPE, nonbonus anagrams: ['CASE', 'ESCAPE']
    3,	// LISTEN, nonbonus anagrams: ['LIST', 'LINE', 'LISTEN']
    2,	// TABLE, nonbonus anagrams: ['TABLE', 'EAT']
    2,	// LATER, nonbonus anagrams: ['LATER', 'LATE']
    3,	// ALMOST, nonbonus anagrams: ['LAST', 'ALMOST', 'MOST']
    2,	// SISTER, nonbonus anagrams: ['SISTER', 'REST']
    2,	// RELIEF, nonbonus anagrams: ['LIFE', 'RELIEF']
    2,	// STATE, nonbonus anagrams: ['STATE', 'EAT']
    3,	// STREET, nonbonus anagrams: ['REST', 'STREET', 'TEST']
    2,	// STREAM, nonbonus anagrams: ['STREAM', 'REST']
    2,	// DESIRE, nonbonus anagrams: ['DESIRE', 'SIDE']
    2,	// START, nonbonus anagrams: ['START', 'ART']
    2,	// ARREST, nonbonus anagrams: ['REST', 'ARREST']


};

const static unsigned char pickAnagramsBonus[] =
{
    0,	// ROYAL, bonus anagrams: []
    0,	// TERMS, bonus anagrams: []
    0,	// TRULY, bonus anagrams: []
    0,	// PRICE, bonus anagrams: []
    0,	// WITHIN, bonus anagrams: []
    0,	// FAVOR, bonus anagrams: []
    0,	// AFFAIR, bonus anagrams: []
    0,	// TOUCH, bonus anagrams: []
    0,	// SERVE, bonus anagrams: []
    0,	// BESIDE, bonus anagrams: []
    0,	// UNLESS, bonus anagrams: []
    0,	// THANK, bonus anagrams: []
    0,	// MIDDLE, bonus anagrams: []
    0,	// VESSEL, bonus anagrams: []
    0,	// TURNED, bonus anagrams: []
    0,	// WHILE, bonus anagrams: []
    0,	// MEMBER, bonus anagrams: []
    0,	// SECOND, bonus anagrams: []
    0,	// FIGHT, bonus anagrams: []
    0,	// NOTICE, bonus anagrams: []
    0,	// OCCUPY, bonus anagrams: []
    0,	// BECOME, bonus anagrams: []
    0,	// WINDOW, bonus anagrams: []
    1,	// GRAND, bonus anagrams: ['GRAN']
    1,	// DURING, bonus anagrams: ['DUNG']
    1,	// VOICE, bonus anagrams: ['VOE']
    1,	// SPEND, bonus anagrams: ['DEN']
    1,	// OUGHT, bonus anagrams: ['TOUGH']
    1,	// MOTION, bonus anagrams: ['MONO']
    1,	// ISSUE, bonus anagrams: ['SUE']
    1,	// FRONT, bonus anagrams: ['TON']
    1,	// TOTAL, bonus anagrams: ['LAT']
    1,	// SHOWN, bonus anagrams: ['WON']
    1,	// FIGURE, bonus anagrams: ['REIF']
    1,	// FIRST, bonus anagrams: ['FIRS']
    1,	// READY, bonus anagrams: ['DARE']
    1,	// UNABLE, bonus anagrams: ['BALE']
    1,	// RETURN, bonus anagrams: ['TURNER']
    1,	// STAND, bonus anagrams: ['ANTS']
    1,	// HONOR, bonus anagrams: ['RHO']
    1,	// BELONG, bonus anagrams: ['LOBE']
    1,	// ORDER, bonus anagrams: ['RODE']
    1,	// PASSED, bonus anagrams: ['APED']
    1,	// FINISH, bonus anagrams: ['SHIN']
    1,	// AHEAD, bonus anagrams: ['DAH']
    1,	// VISIT, bonus anagrams: ['TIS']
    1,	// SPENT, bonus anagrams: ['NET']
    1,	// EXTRA, bonus anagrams: ['AXE']
    1,	// CAREER, bonus anagrams: ['ACRE']
    1,	// DROWN, bonus anagrams: ['WON']
    1,	// SENSE, bonus anagrams: ['ENS']
    1,	// THESE, bonus anagrams: ['ETH']
    1,	// SPRING, bonus anagrams: ['RIPS']
    1,	// STONE, bonus anagrams: ['EON']
    1,	// CORNER, bonus anagrams: ['CORE']
    1,	// HEARD, bonus anagrams: ['RAD']
    1,	// RETIRE, bonus anagrams: ['TIER']
    1,	// PUSHED, bonus anagrams: ['EDHS']
    1,	// THING, bonus anagrams: ['GIN']
    1,	// WINTER, bonus anagrams: ['RENT']
    1,	// ISLAND, bonus anagrams: ['SIAL']
    1,	// SHALL, bonus anagrams: ['LASH']
    1,	// WAVES, bonus anagrams: ['WAVE']
    1,	// AWFUL, bonus anagrams: ['AWL']
    1,	// EMPIRE, bonus anagrams: ['PIER']
    1,	// REACH, bonus anagrams: ['ACRE']
    1,	// HOURS, bonus anagrams: ['OHS']
    1,	// BECAME, bonus anagrams: ['ACME']
    1,	// STRING, bonus anagrams: ['STIR']
    1,	// NEARLY, bonus anagrams: ['ARYL']
    1,	// ALLOW, bonus anagrams: ['OLLA']
    1,	// ELECT, bonus anagrams: ['TEL']
    1,	// BEING, bonus anagrams: ['GIN']
    1,	// TOWARD, bonus anagrams: ['WARD']
    2,	// INFORM, bonus anagrams: ['INFO', 'FOIN']
    2,	// DELAY, bonus anagrams: ['LADE', 'DEY']
    2,	// WOMAN, bonus anagrams: ['NAM', 'WON']
    2,	// EVERY, bonus anagrams: ['VEERY', 'VEER']
    2,	// HUMAN, bonus anagrams: ['HUN', 'NAM']
    2,	// FORTY, bonus anagrams: ['FOY', 'FORT']
    2,	// PARTY, bonus anagrams: ['YAP', 'TRAP']
    2,	// CAUSE, bonus anagrams: ['SUE', 'ACE']
    2,	// HOUSE, bonus anagrams: ['HOE', 'SUE']
    2,	// AFTER, bonus anagrams: ['RET', 'FATE']
    2,	// WATER, bonus anagrams: ['RAW', 'RET']
    2,	// BEGUN, bonus anagrams: ['NEB', 'BEN']
    2,	// INCOME, bonus anagrams: ['CONI', 'COIN']
    2,	// ABOUT, bonus anagrams: ['BAT', 'TAB']
    2,	// STOLE, bonus anagrams: ['LOTS', 'OLE']
    2,	// GREAT, bonus anagrams: ['TEA', 'TAE']
    2,	// DEATH, bonus anagrams: ['HATED', 'EDH']
    2,	// GUEST, bonus anagrams: ['TUG', 'GUT']
    2,	// DESIGN, bonus anagrams: ['SIGNED', 'SINGED']
    2,	// SUFFER, bonus anagrams: ['SUER', 'USER']
    2,	// CENTS, bonus anagrams: ['SCENT', 'SEC']
    2,	// FILLED, bonus anagrams: ['DEFI', 'DELL']
    2,	// BROWN, bonus anagrams: ['BROW', 'WON']
    2,	// UNTIL, bonus anagrams: ['TIL', 'LIT']
    2,	// OFFER, bonus anagrams: ['REF', 'FER']
    2,	// FOURTH, bonus anagrams: ['THRU', 'RUTH']
    2,	// ALONE, bonus anagrams: ['ALE', 'EON']
    2,	// SUMMER, bonus anagrams: ['SUER', 'USER']
    2,	// SMALL, bonus anagrams: ['LAMS', 'ALMS']
    2,	// WORTH, bonus anagrams: ['TROW', 'WORT']
    2,	// SHAPE, bonus anagrams: ['PASH', 'APE']
    2,	// KNOWN, bonus anagrams: ['WON', 'WONK']
    2,	// WOMEN, bonus anagrams: ['MEOW', 'WON']
    2,	// LESSON, bonus anagrams: ['LENO', 'NOEL']
    2,	// DIVIDE, bonus anagrams: ['VIED', 'VIDE']
    2,	// YOUNG, bonus anagrams: ['GOY', 'GNU']
    2,	// SINGLE, bonus anagrams: ['ISLE', 'LEIS']
    2,	// LEARN, bonus anagrams: ['LEAR', 'RALE']
    2,	// OFTEN, bonus anagrams: ['FON', 'NET']
    2,	// NEVER, bonus anagrams: ['NEVE', 'REV']
    2,	// MOTHER, bonus anagrams: ['MOTH', 'OMER']
    2,	// BELOW, bonus anagrams: ['LOBE', 'WEB']
    2,	// OTHER, bonus anagrams: ['TOR', 'ROT']
    2,	// THINK, bonus anagrams: ['KIN', 'INK']
    2,	// PLANT, bonus anagrams: ['ANT', 'TAN']
    2,	// PLANE, bonus anagrams: ['ANE', 'NAE']
    3,	// REPAIR, bonus anagrams: ['PARE', 'REAP', 'APER']
    3,	// OBTAIN, bonus anagrams: ['TAIN', 'BOTA', 'BONITA']
    3,	// ASSURE, bonus anagrams: ['USER', 'SUER', 'ERAS']
    3,	// BUILT, bonus anagrams: ['LIT', 'TIL', 'TUB']
    3,	// SELECT, bonus anagrams: ['SEEL', 'SECT', 'LEES']
    3,	// REALLY, bonus anagrams: ['RELY', 'LYRE', 'ALLY']
    3,	// CENTER, bonus anagrams: ['CERE', 'CENTRE', 'RENT']
    3,	// DOCTOR, bonus anagrams: ['ORDO', 'ROOD', 'ODOR']
    3,	// SIGHT, bonus anagrams: ['SIGH', 'TIS', 'GHIS']
    3,	// RESULT, bonus anagrams: ['SUTLER', 'SUER', 'USER']
    3,	// UNDER, bonus anagrams: ['NUDE', 'UNDE', 'NUDER']
    3,	// PAPER, bonus anagrams: ['PAR', 'RAP', 'REP']
    3,	// RAISE, bonus anagrams: ['ARISE', 'EAR', 'ERA']
    3,	// HEART, bonus anagrams: ['HET', 'TAR', 'RAT']
    3,	// COLOR, bonus anagrams: ['ROC', 'COR', 'LOCO']
    3,	// WORDS, bonus anagrams: ['SOW', 'WOS', 'SWORD']
    3,	// STILL, bonus anagrams: ['LILTS', 'ILL', 'LITS']
    3,	// EVENT, bonus anagrams: ['VET', 'NEVE', 'NET']
    3,	// DOUBT, bonus anagrams: ['TOD', 'TUB', 'DOT']
    3,	// CALLED, bonus anagrams: ['DELL', 'CADE', 'ACED']
    3,	// MEANT, bonus anagrams: ['ANT', 'MET', 'TAN']
    3,	// INSIDE, bonus anagrams: ['DENI', 'NISI', 'NIDE']
    4,	// PEOPLE, bonus anagrams: ['PEPO', 'POPE', 'PELE', 'PEEL']
    4,	// REMAIN, bonus anagrams: ['MARE', 'AMIN', 'REAM', 'REIN']
    4,	// LEAST, bonus anagrams: ['TEL', 'TAS', 'SALE', 'LEAS']
    4,	// CHART, bonus anagrams: ['TAR', 'CHAR', 'RAT', 'ARCH']
    4,	// THOSE, bonus anagrams: ['SOTH', 'ETHOS', 'OSE', 'ETH']
    4,	// STAMP, bonus anagrams: ['MATS', 'MAST', 'AMP', 'PAM']
    4,	// TODAY, bonus anagrams: ['TOY', 'TOAD', 'DATO', 'TOADY']
    4,	// THREE, bonus anagrams: ['ETHER', 'ERE', 'ETH', 'REE']
    4,	// COAST, bonus anagrams: ['COT', 'TAS', 'ASCOT', 'OCAS']
    4,	// SPREAD, bonus anagrams: ['SPADER', 'REPS', 'DAPS', 'DARE']
    4,	// ENTIRE, bonus anagrams: ['ERNE', 'RETINE', 'TINE', 'TIER']
    4,	// FOREST, bonus anagrams: ['RETS', 'FOSTER', 'FORE', 'ERST']
    4,	// METAL, bonus anagrams: ['MEL', 'TAME', 'META', 'LAT']
    4,	// FIELD, bonus anagrams: ['FID', 'ELD', 'DEL', 'FILED']
    5,	// ESCAPE, bonus anagrams: ['PEES', 'SEEP', 'ACES', 'CAPE', 'PEACES']
    5,	// LISTEN, bonus anagrams: ['NEST', 'LIEN', 'LITS', 'ENLIST', 'NETS']
    5,	// TABLE, bonus anagrams: ['ATE', 'BLAT', 'TAE', 'ETA', 'BLATE']
    5,	// LATER, bonus anagrams: ['TELA', 'RET', 'TEAL', 'ALTER', 'LAR']
    5,	// ALMOST, bonus anagrams: ['LATS', 'STOMAL', 'MOTS', 'MOLA', 'ALTS']
    6,	// SISTER, bonus anagrams: ['RESIST', 'REIS', 'SIRE', 'RETS', 'SITS', 'ERST']
    6,	// RELIEF, bonus anagrams: ['FERE', 'LIER', 'REEF', 'LIEF', 'LIEFER', 'LIRE']
    6,	// STATE, bonus anagrams: ['STAT', 'ATE', 'TASTE', 'TAE', 'ETA', 'TATS']
    7,	// STREET, bonus anagrams: ['RETE', 'TESTER', 'TETS', 'RETEST', 'RETS', 'STET', 'ERST']
    7,	// STREAM, bonus anagrams: ['REAM', 'MATS', 'MARE', 'RETS', 'MAST', 'MASTER', 'ERST']
    7,	// DESIRE, bonus anagrams: ['RESIDE', 'REIS', 'SIRE', 'DEER', 'REED', 'REDE', 'DERE']
    7,	// START, bonus anagrams: ['STAR', 'ARTS', 'RATS', 'TAR', 'TARTS', 'RAT', 'TSAR']
    11,	// ARREST, bonus anagrams: ['RASTER', 'ARTS', 'RARE', 'RATS', 'RAREST', 'RETS', 'TSAR', 'STAR', 'STARER', 'ERST', 'REAR']

};


Dictionary::Dictionary()
{
}

bool Dictionary::pickWord(char* buffer, unsigned& numAnagrams, unsigned& numBonusAnagrams)
{
    ASSERT(buffer);

    if (GameStateMachine::getCurrentMaxLettersPerCube() > 1)
    {
        STATIC_ASSERT(arraysize(pickAnagrams) >= arraysize(picks));
        _SYS_strlcpy(buffer, picks[sPickIndex], GameStateMachine::getCurrentMaxLettersPerWord() + 1);

        numAnagrams = pickAnagrams[sPickIndex];
        numBonusAnagrams = pickAnagramsBonus[sPickIndex];
        sPickIndex = (sPickIndex + 1) % arraysize(picks);
        return true;
    }

    if (PrototypeWordList::pickWord(buffer))
    {
        DEBUG_LOG(("picked word %s\n", buffer));
        return true;
    }
    return false;
}

bool Dictionary::isWord(const char* string, bool& isBonus)
{
    return PrototypeWordList::isWord(string, isBonus);
}

bool Dictionary::isOldWord(const char* word)
{
    for (unsigned i = 0; i < sNumOldWords; ++i)
    {
        if (_SYS_strncmp(sOldWords[i], word, GameStateMachine::getCurrentMaxLettersPerWord()) == 0)
        {
            return true;
        }
    }
    return false;
}

bool Dictionary::trim(const char* word, char* buffer)
{
    ASSERT(word);
    ASSERT(buffer);
    int firstLetter;
    int wordLen = _SYS_strnlen(word, MAX_LETTERS_PER_WORD + 1);
    for (firstLetter = 0; firstLetter < wordLen; ++firstLetter)
    {
        if (word[firstLetter] >= 'A' && word[firstLetter] <= 'Z')
        {
            break;
        }
    }
    ASSERT(firstLetter < wordLen);
    int lastLetter;
    for (lastLetter = wordLen - 1; lastLetter >= firstLetter; --lastLetter)
    {
        if (word[lastLetter] >= 'A' && word[lastLetter] <= 'Z')
        {
            break;
        }
    }
    ASSERT(lastLetter >= 0);
    ASSERT(lastLetter >= firstLetter);

    int i;
    for (i = firstLetter; i <= lastLetter; ++i)
    {
        buffer[i - firstLetter] = word[i];
    }
    buffer[i] = '\0';
    ASSERT(_SYS_strnlen(buffer, MAX_LETTERS_PER_WORD + 1) > 0);
    ASSERT((int)_SYS_strnlen(buffer, MAX_LETTERS_PER_WORD + 1) <= wordLen);
    return (firstLetter < wordLen && lastLetter >= 0 && lastLetter >= firstLetter);
}

void Dictionary::sOnEvent(unsigned eventID, const EventData& data)
{
    switch (eventID)
    {
    case EventID_NewAnagram:
        sNumOldWords = 0;
        break;

    case EventID_NewWordFound:
        if (sNumOldWords + 1 < MAX_OLD_WORDS)
        {
            _SYS_strlcpy(sOldWords[sNumOldWords++], data.mWordFound.mWord,
                         GameStateMachine::getCurrentMaxLettersPerWord() + 1);
        }
        break;

    case EventID_GameStateChanged:
        switch (data.mGameStateChanged.mNewStateIndex)
        {
        case GameStateIndex_EndOfRoundScored:
            // TODO remove demo code
            // start the first 5 rounds off with a set word list (reset the
            // rand to a value that is dependent on the round, but then increments)
            // randomize based on time from here on out
                WordGame::random.seed();
            break;
        }
        break;

    default:
        break;
    }
}
