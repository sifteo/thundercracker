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
    "DANCE",
    "BESIDE",
    "UNLESS",
    "MIDDLE",
    "VESSEL",
    "TERMS",
    "WHOLE",
    "TURNED",
    "MEMBER",
    "SECOND",
    "FIGHT",
    "WRONG",
    "NOTICE",
    "OCCUPY",
    "BECOME",
    "AFFAIR",
    "WINDOW",
    "HOUSE",
    "WITHIN",
    "DURING",
    "VOICE",
    "ORDER",
    "MOTION",
    "FRONT",
    "SHOWN",
    "FIGURE",
    "FLOOR",
    "FAVOR",
    "ISSUE",
    "READY",
    "UNABLE",
    "RETURN",
    "TOUCH",
    "STAND",
    "BELONG",
    "PASSED",
    "PRESS",
    "FINISH",
    "THANK",
    "AHEAD",
    "ALLOW",
    "VISIT",
    "CAREER",
    "DROWN",
    "THESE",
    "SPRING",
    "CORNER",
    "HEARD",
    "PUSHED",
    "REPLY",
    "THICK",
    "THING",
    "WINTER",
    "ISLAND",
    "AWFUL",
    "QUIET",
    "EMPIRE",
    "CRIED",
    "REACH",
    "HOURS",
    "FRUIT",
    "RETIRE",
    "STRING",
    "NEARLY",
    "TOWARD",
    "ELECT",
    "BEING",
    "BECAME",
    "INFORM",
    "DELAY",
    "WOMAN",
    "FORCE",
    "OFFER",
    "CAUSE",
    "HUMAN",
    "FORTY",
    "PARTY",
    "WATER",
    "INCOME",
    "DEATH",
    "GUEST",
    "DESIGN",
    "SUFFER",
    "CENTS",
    "ASKED",
    "EIGHT",
    "FILLED",
    "BROWN",
    "WORTH",
    "UNTIL",
    "EXTRA",
    "FOURTH",
    "ALONE",
    "SOLVE",
    "SUMMER",
    "SMALL",
    "CATCH",
    "KNOWN",
    "WOMEN",
    "LESSON",
    "DIVIDE",
    "YOUNG",
    "SINGLE",
    "LEARN",
    "OFTEN",
    "ALIKE",
    "MOTHER",
    "CLOUD",
    "THEIR",
    "OTHER",
    "THINK",
    "INSIDE",
    "CARRY",
    "WHOSE",
    "OBTAIN",
    "REPAIR",
    "MEANT",
    "EVERY",
    "TEACH",
    "BUILT",
    "CLEAN",
    "AFTER",
    "STICK",
    "BEGUN",
    "THOSE",
    "DOZEN",
    "HORSE",
    "SELECT",
    "REALLY",
    "BEGIN",
    "DOCTOR",
    "SIGHT",
    "SHARP",
    "RESULT",
    "UNDER",
    "RAISE",
    "RECENT",
    "HEART",
    "COLOR",
    "WORDS",
    "STILL",
    "ASSURE",
    "SINCE",
    "SHALL",
    "TRACK",
    "EVENT",
    "DOUBT",
    "NORTH",
    "NEVER",
    "CALLED",
    "LOCAL",
    "WROTE",
    "RAPID",
    "PEOPLE",
    "REMAIN",
    "MAYOR",
    "FOREST",
    "LEAST",
    "CHART",
    "STAMP",
    "TODAY",
    "SENSE",
    "STONE",
    "COAST",
    "SHAPE",
    "SPREAD",
    "CLOSE",
    "EARLY",
    "SPEED",
    "ENTIRE",
    "METAL",
    "THREE",
    "LEDGE",
    "FIELD",
    "LATER",
    "CLEAR",
    "ESCAPE",
    "TRAIN",
    "TABLE",
    "PAPER",
    "LISTEN",
    "TRADE",
    "EARTH",
    "ALMOST",
    "SEVEN",
    "SISTER",
    "SERVE",
    "RELIEF",
    "START",
    "STREET",
    "STREAM",
    "DESIRE",
    "STEEL",
    "ARREST",

};

const static unsigned char pickAnagrams[] =
{
    2,	// ROYAL, nonbonus anagrams: ['ROYAL', 'LAY']
    2,	// DANCE, nonbonus anagrams: ['AND', 'DANCE']
    2,	// BESIDE, nonbonus anagrams: ['BESIDE', 'SIDE']
    2,	// UNLESS, nonbonus anagrams: ['UNLESS', 'LESS']
    2,	// MIDDLE, nonbonus anagrams: ['MIDDLE', 'MILE']
    2,	// VESSEL, nonbonus anagrams: ['VESSEL', 'LESS']
    3,	// TERMS, nonbonus anagrams: ['TERM', 'SET', 'TERMS']
    4,	// WHOLE, nonbonus anagrams: ['HOW', 'HOLE', 'WHO', 'WHOLE']
    2,	// TURNED, nonbonus anagrams: ['TURN', 'TURNED']
    2,	// MEMBER, nonbonus anagrams: ['MEMBER', 'MERE']
    2,	// SECOND, nonbonus anagrams: ['SECOND', 'SEND']
    2,	// FIGHT, nonbonus anagrams: ['FIT', 'FIGHT']
    2,	// WRONG, nonbonus anagrams: ['WRONG', 'ROW']
    2,	// NOTICE, nonbonus anagrams: ['NOTICE', 'ONCE']
    2,	// OCCUPY, nonbonus anagrams: ['OCCUPY', 'COPY']
    2,	// BECOME, nonbonus anagrams: ['BECOME', 'COME']
    2,	// AFFAIR, nonbonus anagrams: ['AFFAIR', 'FAIR']
    2,	// WINDOW, nonbonus anagrams: ['WINDOW', 'WIND']
    2,	// HOUSE, nonbonus anagrams: ['HOUSE', 'USE']
    3,	// WITHIN, nonbonus anagrams: ['WITHIN', 'WITH', 'THIN']
    2,	// DURING, nonbonus anagrams: ['DURING', 'RING']
    2,	// VOICE, nonbonus anagrams: ['VOICE', 'ICE']
    2,	// ORDER, nonbonus anagrams: ['ORDER', 'RED']
    3,	// MOTION, nonbonus anagrams: ['MOTION', 'OMIT', 'MOON']
    2,	// FRONT, nonbonus anagrams: ['NOT', 'FRONT']
    4,	// SHOWN, nonbonus anagrams: ['SHOWN', 'OWN', 'NOW', 'SHOW']
    2,	// FIGURE, nonbonus anagrams: ['FIRE', 'FIGURE']
    2,	// FLOOR, nonbonus anagrams: ['FOR', 'FLOOR']
    2,	// FAVOR, nonbonus anagrams: ['FAVOR', 'FOR']
    2,	// ISSUE, nonbonus anagrams: ['ISSUE', 'USE']
    3,	// READY, nonbonus anagrams: ['READ', 'READY', 'DAY']
    2,	// UNABLE, nonbonus anagrams: ['UNABLE', 'ABLE']
    2,	// RETURN, nonbonus anagrams: ['TURN', 'RETURN']
    2,	// TOUCH, nonbonus anagrams: ['TOUCH', 'OUT']
    2,	// STAND, nonbonus anagrams: ['SAT', 'STAND']
    2,	// BELONG, nonbonus anagrams: ['BELONG', 'LONG']
    2,	// PASSED, nonbonus anagrams: ['PASSED', 'PASS']
    2,	// PRESS, nonbonus anagrams: ['PRESS', 'PER']
    2,	// FINISH, nonbonus anagrams: ['FISH', 'FINISH']
    2,	// THANK, nonbonus anagrams: ['HAT', 'THANK']
    2,	// AHEAD, nonbonus anagrams: ['HAD', 'AHEAD']
    3,	// ALLOW, nonbonus anagrams: ['LAW', 'LOW', 'ALLOW']
    2,	// VISIT, nonbonus anagrams: ['VISIT', 'SIT']
    2,	// CAREER, nonbonus anagrams: ['CAREER', 'CARE']
    4,	// DROWN, nonbonus anagrams: ['DROWN', 'OWN', 'WORD', 'NOW']
    3,	// THESE, nonbonus anagrams: ['THESE', 'THE', 'SEE']
    2,	// SPRING, nonbonus anagrams: ['SPRING', 'RING']
    2,	// CORNER, nonbonus anagrams: ['CORN', 'CORNER']
    2,	// HEARD, nonbonus anagrams: ['HEARD', 'HEAR']
    3,	// PUSHED, nonbonus anagrams: ['PUSH', 'PUSHED', 'SHED']
    2,	// REPLY, nonbonus anagrams: ['REPLY', 'PER']
    2,	// THICK, nonbonus anagrams: ['THICK', 'HIT']
    2,	// THING, nonbonus anagrams: ['THIN', 'THING']
    2,	// WINTER, nonbonus anagrams: ['WIRE', 'WINTER']
    2,	// ISLAND, nonbonus anagrams: ['LAND', 'ISLAND']
    2,	// AWFUL, nonbonus anagrams: ['AWFUL', 'LAW']
    2,	// QUIET, nonbonus anagrams: ['QUIET', 'QUITE']
    2,	// EMPIRE, nonbonus anagrams: ['EMPIRE', 'MERE']
    2,	// CRIED, nonbonus anagrams: ['RIDE', 'CRIED']
    3,	// REACH, nonbonus anagrams: ['CARE', 'REACH', 'HER']
    2,	// HOURS, nonbonus anagrams: ['HOURS', 'HOUR']
    2,	// FRUIT, nonbonus anagrams: ['FRUIT', 'FIT']
    2,	// RETIRE, nonbonus anagrams: ['RETIRE', 'TIRE']
    2,	// STRING, nonbonus anagrams: ['RING', 'STRING']
    2,	// NEARLY, nonbonus anagrams: ['NEAR', 'NEARLY']
    2,	// TOWARD, nonbonus anagrams: ['TOWARD', 'DRAW']
    2,	// ELECT, nonbonus anagrams: ['LET', 'ELECT']
    2,	// BEING, nonbonus anagrams: ['BEING', 'BEG']
    2,	// BECAME, nonbonus anagrams: ['BECAME', 'CAME']
    2,	// INFORM, nonbonus anagrams: ['FORM', 'INFORM']
    3,	// DELAY, nonbonus anagrams: ['LAY', 'DEAL', 'DELAY']
    4,	// WOMAN, nonbonus anagrams: ['WOMAN', 'OWN', 'NOW', 'MAN']
    2,	// FORCE, nonbonus anagrams: ['FOR', 'FORCE']
    2,	// OFFER, nonbonus anagrams: ['FOR', 'OFFER']
    2,	// CAUSE, nonbonus anagrams: ['USE', 'CAUSE']
    2,	// HUMAN, nonbonus anagrams: ['HUMAN', 'MAN']
    2,	// FORTY, nonbonus anagrams: ['FORTY', 'FOR']
    4,	// PARTY, nonbonus anagrams: ['PAY', 'TRY', 'PART', 'PARTY']
    2,	// WATER, nonbonus anagrams: ['WATER', 'WAR']
    2,	// INCOME, nonbonus anagrams: ['COME', 'INCOME']
    2,	// DEATH, nonbonus anagrams: ['DEATH', 'HAT']
    2,	// GUEST, nonbonus anagrams: ['SET', 'GUEST']
    4,	// DESIGN, nonbonus anagrams: ['DESIGN', 'SIGN', 'SING', 'SIDE']
    2,	// SUFFER, nonbonus anagrams: ['SUFFER', 'SURE']
    2,	// CENTS, nonbonus anagrams: ['CENTS', 'CENT']
    2,	// ASKED, nonbonus anagrams: ['ASK', 'ASKED']
    2,	// EIGHT, nonbonus anagrams: ['EIGHT', 'THE']
    2,	// FILLED, nonbonus anagrams: ['FILLED', 'FILL']
    3,	// BROWN, nonbonus anagrams: ['BROWN', 'OWN', 'NOW']
    3,	// WORTH, nonbonus anagrams: ['THROW', 'WORTH', 'ROW']
    2,	// UNTIL, nonbonus anagrams: ['UNTIL', 'UNIT']
    2,	// EXTRA, nonbonus anagrams: ['EXTRA', 'ARE']
    2,	// FOURTH, nonbonus anagrams: ['FOUR', 'FOURTH']
    2,	// ALONE, nonbonus anagrams: ['ALONE', 'ONE']
    2,	// SOLVE, nonbonus anagrams: ['SOLVE', 'LOVE']
    2,	// SUMMER, nonbonus anagrams: ['SUMMER', 'SURE']
    2,	// SMALL, nonbonus anagrams: ['SMALL', 'ALL']
    2,	// CATCH, nonbonus anagrams: ['CATCH', 'CAT']
    4,	// KNOWN, nonbonus anagrams: ['OWN', 'KNOW', 'KNOWN', 'NOW']
    4,	// WOMEN, nonbonus anagrams: ['OWN', 'MEN', 'NOW', 'WOMEN']
    2,	// LESSON, nonbonus anagrams: ['LESSON', 'LESS']
    2,	// DIVIDE, nonbonus anagrams: ['DIED', 'DIVIDE']
    2,	// YOUNG, nonbonus anagrams: ['YOUNG', 'GUN']
    3,	// SINGLE, nonbonus anagrams: ['SING', 'SINGLE', 'SIGN']
    2,	// LEARN, nonbonus anagrams: ['RAN', 'LEARN']
    2,	// OFTEN, nonbonus anagrams: ['TEN', 'OFTEN']
    2,	// ALIKE, nonbonus anagrams: ['LIKE', 'ALIKE']
    2,	// MOTHER, nonbonus anagrams: ['MOTHER', 'MORE']
    2,	// CLOUD, nonbonus anagrams: ['LOUD', 'CLOUD']
    2,	// THEIR, nonbonus anagrams: ['THEIR', 'THE']
    2,	// OTHER, nonbonus anagrams: ['OTHER', 'HER']
    2,	// THINK, nonbonus anagrams: ['THINK', 'THIN']
    2,	// INSIDE, nonbonus anagrams: ['INSIDE', 'SIDE']
    2,	// CARRY, nonbonus anagrams: ['CARRY', 'CAR']
    3,	// WHOSE, nonbonus anagrams: ['WHOSE', 'WHO', 'HOW']
    2,	// OBTAIN, nonbonus anagrams: ['OBTAIN', 'BOAT']
    2,	// REPAIR, nonbonus anagrams: ['PAIR', 'REPAIR']
    3,	// MEANT, nonbonus anagrams: ['MEANT', 'NAME', 'MEAN']
    2,	// EVERY, nonbonus anagrams: ['VERY', 'EVERY']
    3,	// TEACH, nonbonus anagrams: ['EACH', 'TEACH', 'EAT']
    2,	// BUILT, nonbonus anagrams: ['BUILT', 'BUT']
    2,	// CLEAN, nonbonus anagrams: ['CAN', 'CLEAN']
    2,	// AFTER, nonbonus anagrams: ['AFTER', 'ARE']
    3,	// STICK, nonbonus anagrams: ['SIT', 'STICK', 'ITS']
    2,	// BEGUN, nonbonus anagrams: ['BEG', 'BEGUN']
    3,	// THOSE, nonbonus anagrams: ['SET', 'HOT', 'THOSE']
    2,	// DOZEN, nonbonus anagrams: ['END', 'DOZEN']
    2,	// HORSE, nonbonus anagrams: ['ROSE', 'HORSE']
    2,	// SELECT, nonbonus anagrams: ['SELECT', 'ELSE']
    2,	// REALLY, nonbonus anagrams: ['REAL', 'REALLY']
    2,	// BEGIN, nonbonus anagrams: ['BEGIN', 'BEG']
    2,	// DOCTOR, nonbonus anagrams: ['DOOR', 'DOCTOR']
    2,	// SIGHT, nonbonus anagrams: ['SIGHT', 'SIT']
    2,	// SHARP, nonbonus anagrams: ['HAS', 'SHARP']
    2,	// RESULT, nonbonus anagrams: ['RESULT', 'SURE']
    3,	// UNDER, nonbonus anagrams: ['RUN', 'UNDER', 'RED']
    2,	// RAISE, nonbonus anagrams: ['RAISE', 'AIR']
    3,	// RECENT, nonbonus anagrams: ['CENTER', 'CENT', 'RECENT']
    4,	// HEART, nonbonus anagrams: ['HEART', 'ART', 'HEAR', 'THE']
    2,	// COLOR, nonbonus anagrams: ['COLOR', 'COOL']
    2,	// WORDS, nonbonus anagrams: ['WORDS', 'WORD']
    3,	// STILL, nonbonus anagrams: ['SIT', 'STILL', 'ITS']
    2,	// ASSURE, nonbonus anagrams: ['ASSURE', 'SURE']
    2,	// SINCE, nonbonus anagrams: ['SINCE', 'NICE']
    2,	// SHALL, nonbonus anagrams: ['HAS', 'SHALL']
    2,	// TRACK, nonbonus anagrams: ['TRACK', 'ART']
    3,	// EVENT, nonbonus anagrams: ['EVEN', 'TEN', 'EVENT']
    2,	// DOUBT, nonbonus anagrams: ['DOUBT', 'BUT']
    2,	// NORTH, nonbonus anagrams: ['NORTH', 'NOR']
    2,	// NEVER, nonbonus anagrams: ['NEVER', 'EVER']
    2,	// CALLED, nonbonus anagrams: ['CALLED', 'CALL']
    2,	// LOCAL, nonbonus anagrams: ['ALL', 'LOCAL']
    2,	// WROTE, nonbonus anagrams: ['WROTE', 'ROW']
    2,	// RAPID, nonbonus anagrams: ['RAPID', 'PAID']
    2,	// PEOPLE, nonbonus anagrams: ['PEOPLE', 'POLE']
    2,	// REMAIN, nonbonus anagrams: ['REMAIN', 'MAIN']
    2,	// MAYOR, nonbonus anagrams: ['MAY', 'MAYOR']
    2,	// FOREST, nonbonus anagrams: ['REST', 'FOREST']
    3,	// LEAST, nonbonus anagrams: ['LEAST', 'LET', 'SAT']
    2,	// CHART, nonbonus anagrams: ['ART', 'CHART']
    2,	// STAMP, nonbonus anagrams: ['MAP', 'STAMP']
    2,	// TODAY, nonbonus anagrams: ['DAY', 'TODAY']
    2,	// SENSE, nonbonus anagrams: ['SEEN', 'SENSE']
    2,	// STONE, nonbonus anagrams: ['STONE', 'TONE']
    2,	// COAST, nonbonus anagrams: ['COAST', 'SAT']
    2,	// SHAPE, nonbonus anagrams: ['SHAPE', 'HAS']
    2,	// SPREAD, nonbonus anagrams: ['READ', 'SPREAD']
    2,	// CLOSE, nonbonus anagrams: ['LOSE', 'CLOSE']
    2,	// EARLY, nonbonus anagrams: ['EARLY', 'ARE']
    2,	// SPEED, nonbonus anagrams: ['DEEP', 'SPEED']
    2,	// ENTIRE, nonbonus anagrams: ['ENTIRE', 'TIRE']
    2,	// METAL, nonbonus anagrams: ['MEAT', 'METAL']
    3,	// THREE, nonbonus anagrams: ['THERE', 'THREE', 'THE']
    3,	// LEDGE, nonbonus anagrams: ['LED', 'EDGE', 'LEDGE']
    3,	// FIELD, nonbonus anagrams: ['LED', 'FIELD', 'FILE']
    2,	// LATER, nonbonus anagrams: ['LATER', 'LATE']
    2,	// CLEAR, nonbonus anagrams: ['CAR', 'CLEAR']
    2,	// ESCAPE, nonbonus anagrams: ['CASE', 'ESCAPE']
    3,	// TRAIN, nonbonus anagrams: ['ART', 'RAIN', 'TRAIN']
    2,	// TABLE, nonbonus anagrams: ['TABLE', 'EAT']
    2,	// PAPER, nonbonus anagrams: ['PER', 'PAPER']
    3,	// LISTEN, nonbonus anagrams: ['LIST', 'LINE', 'LISTEN']
    3,	// TRADE, nonbonus anagrams: ['ART', 'TRADE', 'DEAR']
    3,	// EARTH, nonbonus anagrams: ['ARE', 'EARTH', 'THE']
    3,	// ALMOST, nonbonus anagrams: ['LAST', 'ALMOST', 'MOST']
    2,	// SEVEN, nonbonus anagrams: ['EVEN', 'SEVEN']
    2,	// SISTER, nonbonus anagrams: ['SISTER', 'REST']
    2,	// SERVE, nonbonus anagrams: ['SERVE', 'EVER']
    2,	// RELIEF, nonbonus anagrams: ['LIFE', 'RELIEF']
    2,	// START, nonbonus anagrams: ['START', 'ART']
    3,	// STREET, nonbonus anagrams: ['REST', 'STREET', 'TEST']
    2,	// STREAM, nonbonus anagrams: ['STREAM', 'REST']
    2,	// DESIRE, nonbonus anagrams: ['DESIRE', 'SIDE']
    2,	// STEEL, nonbonus anagrams: ['STEEL', 'SET']
    2,	// ARREST, nonbonus anagrams: ['REST', 'ARREST']

};

const static unsigned char pickAnagramsBonus[] =
{
    0,	// ROYAL, bonus anagrams: []
    0,	// DANCE, bonus anagrams: []
    0,	// BESIDE, bonus anagrams: []
    0,	// UNLESS, bonus anagrams: []
    0,	// MIDDLE, bonus anagrams: []
    0,	// VESSEL, bonus anagrams: []
    0,	// TERMS, bonus anagrams: []
    0,	// WHOLE, bonus anagrams: []
    0,	// TURNED, bonus anagrams: []
    0,	// MEMBER, bonus anagrams: []
    0,	// SECOND, bonus anagrams: []
    0,	// FIGHT, bonus anagrams: []
    0,	// WRONG, bonus anagrams: []
    0,	// NOTICE, bonus anagrams: []
    0,	// OCCUPY, bonus anagrams: []
    0,	// BECOME, bonus anagrams: []
    0,	// AFFAIR, bonus anagrams: []
    0,	// WINDOW, bonus anagrams: []
    2,	// HOUSE, bonus anagrams: ['HOE', 'SUE']
    0,	// WITHIN, bonus anagrams: []
    1,	// DURING, bonus anagrams: ['DUNG']
    1,	// VOICE, bonus anagrams: ['VOE']
    1,	// ORDER, bonus anagrams: ['RODE']
    1,	// MOTION, bonus anagrams: ['MONO']
    1,	// FRONT, bonus anagrams: ['TON']
    1,	// SHOWN, bonus anagrams: ['WON']
    1,	// FIGURE, bonus anagrams: ['REIF']
    1,	// FLOOR, bonus anagrams: ['FRO']
    1,	// FAVOR, bonus anagrams: ['FRO']
    1,	// ISSUE, bonus anagrams: ['SUE']
    1,	// READY, bonus anagrams: ['DARE']
    1,	// UNABLE, bonus anagrams: ['BALE']
    1,	// RETURN, bonus anagrams: ['TURNER']
    1,	// TOUCH, bonus anagrams: ['OUCH']
    1,	// STAND, bonus anagrams: ['TAS']
    1,	// BELONG, bonus anagrams: ['LOBE']
    1,	// PASSED, bonus anagrams: ['APED']
    1,	// PRESS, bonus anagrams: ['REP']
    1,	// FINISH, bonus anagrams: ['SHIN']
    1,	// THANK, bonus anagrams: ['HANK']
    1,	// AHEAD, bonus anagrams: ['DAH']
    1,	// ALLOW, bonus anagrams: ['OLLA']
    1,	// VISIT, bonus anagrams: ['TIS']
    1,	// CAREER, bonus anagrams: ['ACRE']
    1,	// DROWN, bonus anagrams: ['WON']
    1,	// THESE, bonus anagrams: ['ETH']
    1,	// SPRING, bonus anagrams: ['RIPS']
    1,	// CORNER, bonus anagrams: ['CORE']
    1,	// HEARD, bonus anagrams: ['RAD']
    1,	// PUSHED, bonus anagrams: ['EDHS']
    1,	// REPLY, bonus anagrams: ['REP']
    1,	// THICK, bonus anagrams: ['HICK']
    1,	// THING, bonus anagrams: ['GIN']
    1,	// WINTER, bonus anagrams: ['RENT']
    1,	// ISLAND, bonus anagrams: ['SIAL']
    1,	// AWFUL, bonus anagrams: ['AWL']
    1,	// QUIET, bonus anagrams: ['ETUI']
    1,	// EMPIRE, bonus anagrams: ['PIER']
    1,	// CRIED, bonus anagrams: ['IRED']
    1,	// REACH, bonus anagrams: ['ACRE']
    1,	// HOURS, bonus anagrams: ['OHS']
    1,	// FRUIT, bonus anagrams: ['FUR']
    1,	// RETIRE, bonus anagrams: ['TIER']
    1,	// STRING, bonus anagrams: ['STIR']
    1,	// NEARLY, bonus anagrams: ['ARYL']
    1,	// TOWARD, bonus anagrams: ['WARD']
    1,	// ELECT, bonus anagrams: ['TEL']
    1,	// BEING, bonus anagrams: ['GIN']
    1,	// BECAME, bonus anagrams: ['ACME']
    2,	// INFORM, bonus anagrams: ['INFO', 'FOIN']
    2,	// DELAY, bonus anagrams: ['LADE', 'DEY']
    2,	// WOMAN, bonus anagrams: ['NAM', 'WON']
    2,	// FORCE, bonus anagrams: ['CERO', 'FRO']
    2,	// OFFER, bonus anagrams: ['REF', 'FER']
    2,	// CAUSE, bonus anagrams: ['SUE', 'ACE']
    2,	// HUMAN, bonus anagrams: ['HUN', 'NAM']
    2,	// FORTY, bonus anagrams: ['TYRO', 'FRO']
    2,	// PARTY, bonus anagrams: ['YAP', 'TRAP']
    2,	// WATER, bonus anagrams: ['RAW', 'RET']
    2,	// INCOME, bonus anagrams: ['CONI', 'COIN']
    2,	// DEATH, bonus anagrams: ['HATED', 'EDH']
    2,	// GUEST, bonus anagrams: ['TUG', 'GUT']
    2,	// DESIGN, bonus anagrams: ['SIGNED', 'SINGED']
    2,	// SUFFER, bonus anagrams: ['SUER', 'USER']
    2,	// CENTS, bonus anagrams: ['SCENT', 'SEC']
    2,	// ASKED, bonus anagrams: ['SKA', 'DESK']
    2,	// EIGHT, bonus anagrams: ['GIE', 'ETH']
    2,	// FILLED, bonus anagrams: ['DEFI', 'DELL']
    2,	// BROWN, bonus anagrams: ['BROW', 'WON']
    2,	// WORTH, bonus anagrams: ['THRO', 'WROTH']
    2,	// UNTIL, bonus anagrams: ['TIL', 'LIT']
    2,	// EXTRA, bonus anagrams: ['EAR', 'ERA']
    2,	// FOURTH, bonus anagrams: ['THRU', 'RUTH']
    2,	// ALONE, bonus anagrams: ['ALE', 'EON']
    2,	// SOLVE, bonus anagrams: ['SOL', 'LOVES']
    2,	// SUMMER, bonus anagrams: ['SUER', 'USER']
    2,	// SMALL, bonus anagrams: ['LAMS', 'ALMS']
    2,	// CATCH, bonus anagrams: ['TACH', 'CHAT']
    2,	// KNOWN, bonus anagrams: ['WON', 'WONK']
    2,	// WOMEN, bonus anagrams: ['MEOW', 'WON']
    2,	// LESSON, bonus anagrams: ['LENO', 'NOEL']
    2,	// DIVIDE, bonus anagrams: ['VIED', 'VIDE']
    2,	// YOUNG, bonus anagrams: ['GOY', 'GNU']
    2,	// SINGLE, bonus anagrams: ['ISLE', 'LEIS']
    2,	// LEARN, bonus anagrams: ['LEAR', 'RALE']
    2,	// OFTEN, bonus anagrams: ['FON', 'NET']
    2,	// ALIKE, bonus anagrams: ['AIL', 'KEA']
    2,	// MOTHER, bonus anagrams: ['MOTH', 'OMER']
    2,	// CLOUD, bonus anagrams: ['CUD', 'COL']
    2,	// THEIR, bonus anagrams: ['HET', 'HEIR']
    2,	// OTHER, bonus anagrams: ['TOR', 'ROT']
    2,	// THINK, bonus anagrams: ['KIN', 'INK']
    2,	// INSIDE, bonus anagrams: ['NISI', 'NIDE']
    2,	// CARRY, bonus anagrams: ['CRY', 'ARC']
    3,	// WHOSE, bonus anagrams: ['SEW', 'HOSE', 'HOES']
    3,	// OBTAIN, bonus anagrams: ['TAIN', 'BOTA', 'BONITA']
    3,	// REPAIR, bonus anagrams: ['PARE', 'REAP', 'APER']
    3,	// MEANT, bonus anagrams: ['ANT', 'MET', 'TAN']
    3,	// EVERY, bonus anagrams: ['VEE', 'RYE', 'EVE']
    3,	// TEACH, bonus anagrams: ['CHEAT', 'TEA', 'TAE']
    3,	// BUILT, bonus anagrams: ['LIT', 'TIL', 'TUB']
    3,	// CLEAN, bonus anagrams: ['ELAN', 'CEL', 'LEAN']
    3,	// AFTER, bonus anagrams: ['AFT', 'ERA', 'REFT']
    3,	// STICK, bonus anagrams: ['TICKS', 'TIS', 'TICK']
    3,	// BEGUN, bonus anagrams: ['NUB', 'GENU', 'BUN']
    3,	// THOSE, bonus anagrams: ['THO', 'HOSE', 'HOES']
    3,	// DOZEN, bonus anagrams: ['ZONED', 'ZONE', 'DEN']
    3,	// HORSE, bonus anagrams: ['ORES', 'ROES', 'HES']
    3,	// SELECT, bonus anagrams: ['SEEL', 'SECT', 'LEES']
    3,	// REALLY, bonus anagrams: ['RELY', 'LYRE', 'ALLY']
    3,	// BEGIN, bonus anagrams: ['BIN', 'NIB', 'BINGE']
    3,	// DOCTOR, bonus anagrams: ['ORDO', 'ROOD', 'ODOR']
    3,	// SIGHT, bonus anagrams: ['SIGH', 'TIS', 'GHIS']
    3,	// SHARP, bonus anagrams: ['SHA', 'HARPS', 'HARP']
    3,	// RESULT, bonus anagrams: ['SUTLER', 'SUER', 'USER']
    3,	// UNDER, bonus anagrams: ['NUDE', 'UNDE', 'NUDER']
    3,	// RAISE, bonus anagrams: ['RES', 'RIA', 'SER']
    3,	// RECENT, bonus anagrams: ['CERE', 'CENTRE', 'RENT']
    3,	// HEART, bonus anagrams: ['HET', 'TAR', 'RAT']
    3,	// COLOR, bonus anagrams: ['ROC', 'COR', 'LOCO']
    3,	// WORDS, bonus anagrams: ['SOW', 'WOS', 'SWORD']
    3,	// STILL, bonus anagrams: ['TILLS', 'TIS', 'TILL']
    3,	// ASSURE, bonus anagrams: ['USER', 'SUER', 'ERAS']
    3,	// SINCE, bonus anagrams: ['SEC', 'SIN', 'INS']
    3,	// SHALL, bonus anagrams: ['HALLS', 'SHA', 'HALL']
    3,	// TRACK, bonus anagrams: ['RAT', 'RACK', 'TAR']
    3,	// EVENT, bonus anagrams: ['VET', 'NEVE', 'NET']
    3,	// DOUBT, bonus anagrams: ['TOD', 'TUB', 'DOT']
    3,	// NORTH, bonus anagrams: ['NTH', 'THORN', 'THRO']
    3,	// NEVER, bonus anagrams: ['NERVE', 'ERN', 'VEER']
    3,	// CALLED, bonus anagrams: ['DELL', 'CADE', 'ACED']
    3,	// LOCAL, bonus anagrams: ['COAL', 'COL', 'COLA']
    3,	// WROTE, bonus anagrams: ['WET', 'TEW', 'ROTE']
    4,	// RAPID, bonus anagrams: ['PAR', 'PADI', 'RAP', 'RID']
    4,	// PEOPLE, bonus anagrams: ['PEPO', 'POPE', 'PELE', 'PEEL']
    4,	// REMAIN, bonus anagrams: ['MARE', 'AMIN', 'REAM', 'REIN']
    4,	// MAYOR, bonus anagrams: ['MORAY', 'ROM', 'MOR', 'YAM']
    4,	// FOREST, bonus anagrams: ['RETS', 'FOSTER', 'FORE', 'ERST']
    4,	// LEAST, bonus anagrams: ['TEL', 'TAS', 'SALE', 'LEAS']
    4,	// CHART, bonus anagrams: ['TAR', 'CHAR', 'RAT', 'ARCH']
    4,	// STAMP, bonus anagrams: ['MATS', 'MAST', 'AMP', 'PAM']
    4,	// TODAY, bonus anagrams: ['TOY', 'TOAD', 'DATO', 'TOADY']
    4,	// SENSE, bonus anagrams: ['ESS', 'SENE', 'ENS', 'SEN']
    4,	// STONE, bonus anagrams: ['ENS', 'SOT', 'TONES', 'SEN']
    4,	// COAST, bonus anagrams: ['COT', 'TAS', 'ASCOT', 'OCAS']
    4,	// SHAPE, bonus anagrams: ['EPHAS', 'PES', 'SHA', 'EPHA']
    4,	// SPREAD, bonus anagrams: ['SPADER', 'REPS', 'DAPS', 'DARE']
    4,	// CLOSE, bonus anagrams: ['OLES', 'COLES', 'SEC', 'COL']
    4,	// EARLY, bonus anagrams: ['LYE', 'ARYL', 'EAR', 'ERA']
    4,	// SPEED, bonus anagrams: ['DEEPS', 'PEDES', 'PES', 'PEED']
    4,	// ENTIRE, bonus anagrams: ['ERNE', 'RETINE', 'TINE', 'TIER']
    4,	// METAL, bonus anagrams: ['MEL', 'TAME', 'META', 'LAT']
    4,	// THREE, bonus anagrams: ['ETHER', 'ERE', 'ETH', 'REE']
    4,	// LEDGE, bonus anagrams: ['LEG', 'DEL', 'GEED', 'GEL']
    4,	// FIELD, bonus anagrams: ['FID', 'ELD', 'DEL', 'FILED']
    5,	// LATER, bonus anagrams: ['TELA', 'RET', 'TEAL', 'ALTER', 'LAR']
    5,	// CLEAR, bonus anagrams: ['CEL', 'LEAR', 'ARC', 'CARLE', 'RALE']
    5,	// ESCAPE, bonus anagrams: ['PEES', 'SEEP', 'ACES', 'CAPE', 'PEACES']
    5,	// TRAIN, bonus anagrams: ['TAR', 'RANI', 'RAT', 'TIN', 'NIT']
    5,	// TABLE, bonus anagrams: ['ATE', 'BLAT', 'TAE', 'ETA', 'BLATE']
    5,	// PAPER, bonus anagrams: ['PAP', 'APER', 'REP', 'PARE', 'REAP']
    5,	// LISTEN, bonus anagrams: ['NEST', 'LIEN', 'LITS', 'ENLIST', 'NETS']
    5,	// TRADE, bonus anagrams: ['TAR', 'TED', 'RAT', 'TARED', 'DERAT']
    5,	// EARTH, bonus anagrams: ['ERA', 'RATHE', 'ETH', 'EAR', 'RATH']
    5,	// ALMOST, bonus anagrams: ['LATS', 'STOMAL', 'MOTS', 'MOLA', 'ALTS']
    5,	// SEVEN, bonus anagrams: ['NEVES', 'NEVE', 'ENS', 'SEN', 'EVENS']
    6,	// SISTER, bonus anagrams: ['RESIST', 'REIS', 'SIRE', 'RETS', 'SITS', 'ERST']
    6,	// SERVE, bonus anagrams: ['SER', 'VEER', 'ERS', 'RES', 'VEERS', 'SEVER']
    6,	// RELIEF, bonus anagrams: ['FERE', 'LIER', 'REEF', 'LIEF', 'LIEFER', 'LIRE']
    7,	// START, bonus anagrams: ['STAR', 'ARTS', 'RATS', 'TAR', 'TARTS', 'RAT', 'TSAR']
    7,	// STREET, bonus anagrams: ['RETE', 'TESTER', 'TETS', 'RETEST', 'RETS', 'STET', 'ERST']
    7,	// STREAM, bonus anagrams: ['REAM', 'MATS', 'MARE', 'RETS', 'MAST', 'MASTER', 'ERST']
    7,	// DESIRE, bonus anagrams: ['RESIDE', 'REIS', 'SIRE', 'DEER', 'REED', 'REDE', 'DERE']
    10,	// STEEL, bonus anagrams: ['TEEL', 'TEELS', 'TELES', 'LEETS', 'LEET', 'SLEET', 'TELE', 'ELS', 'SEL', 'STELE']
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
