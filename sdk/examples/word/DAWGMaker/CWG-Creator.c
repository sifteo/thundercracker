// This program will first compile a traditional DAWG encoding from the "Word-List.txt" file.
// Next, data files are written to assist in the "CWG" creation process.
// A "Caroline Word Graph" will then be created using the intermediate data files, and stored in "CWG_Data_For_Word-List.dat".
// There is a very good reason for why this program is 1800 lines long.  The CWG is a perfect and complete hash function for English-Language in TWL06.

// 1) "Word-List.txt" is a text file with the number of words written on the very first line, and 1 word per line after that.  The words are case-insensitive.
// 2) The "CWG" encoding is very sensitive to the size and content of "Word-List.txt", so only minor alterations are guaranteed to work without code analysis.

// Include the big-three header files.
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// General high-level program constants.
#define MAX 6
#define NUMBER_OF_ENGLISH_LETTERS 26
#define INPUT_LIMIT 30
#define LOWER_IT 32
#define TEN 10
#define BIT_COUNT_FOR_CHILD_INDEX 17
#define EOW_FLAG 0X40000000
#define INT_BITS 32
#define CHILD_MASK 0X0001FFFF
#define LIST_FORMAT_INDEX_MASK 0X3FFE0000
#define LIST_FORMAT_BIT_SHIFT 17
#define TRADITIONAL_CHILD_SHIFT 5
#define TRADITIONAL_EOW_FLAG 0X00800000
#define TRADITIONAL_EOL_FLAG 0X00400000

// C requires a boolean variable type so use C's typedef concept to create one.
typedef enum { FALSE = 0, TRUE = 1 } Bool;
typedef Bool* BoolPtr;

// The lexicon text file.
#define RAW_LEXICON "dictionary.txt"

// This program will create "5" binary-data files for use, and "1" text-data file for inspection.
#define TRADITIONAL_DAWG_DATA "Traditional_Dawg_For_Word-List.dat"
#define TRADITIONAL_DAWG_TEXT_DATA "Traditional_Dawg_Text_For_Word-List.txt"

#define DIRECT_GRAPH_DATA_PART_ONE "Part_One_Direct_Graph_For_Word-List.dat"
#define DIRECT_GRAPH_DATA_PART_TWO "Part_Two_Direct_Graph_For_Word-List.dat"
#define FINAL_NODES_DATA "Final_Nodes_For_Word-List.dat"

// The complete "CWG" graph is stored here.
#define CWG_DATA "CWG_Data_For_Word-List.dat"

// Lookup tables used for node encoding and number-string decoding.
const unsigned int PowersOfTwo[INT_BITS - 1] = { 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384,
 32768, 65536, 131072, 262144, 524288, 1048576, 2097152, 4194304, 8388608, 16777216, 33554432, 67108864, 134217728,
 268435456, 536870912, 1073741824 };
const unsigned int PowersOfTen[TEN] = { 1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000 };

// Lookup tables used to extract child-offset values.
const unsigned char PopCountTable[256] = { 0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4,
 3, 4, 4, 5, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 1, 2, 2, 3, 2, 3,
 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4,
 4, 5, 4, 5, 5, 6, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 2, 3, 3, 4,
 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6,
 6, 7, 4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8 };
const int ChildListMasks[NUMBER_OF_ENGLISH_LETTERS] = { 0X1, 0X3, 0X7, 0XF, 0X1F, 0X3F, 0X7F, 0XFF, 0X1FF, 0X3FF, 0X7FF, 0XFFF,
 0X1FFF, 0X3FFF, 0X7FFF, 0XFFFF, 0X1FFFF, 0X3FFFF, 0X7FFFF, 0XFFFFF, 0X1FFFFF, 0X3FFFFF, 0X7FFFFF, 0XFFFFFF, 0X1FFFFFF, 0X3FFFFFF };

// This simple function clips off the extra chars for each "fgets()" line.  Works for Linux and Windows text format.
void CutOffExtraChars(char *ThisLine){
	if ( ThisLine[strlen(ThisLine) - 2] == '\r' ) ThisLine[strlen(ThisLine) - 2] = '\0';
	else if ( ThisLine[strlen(ThisLine) - 1] == '\n' ) ThisLine[strlen(ThisLine) - 1] = '\0';
}

// This is an important function involved in any movement through the CWG.
// The "CompleteChildList" is masked according to "LetterPosition", and then a byte-wise pop-count is carried out. ('A' => 0).
// The function retruns the corresponding offset for the "LetterPosition"th letter.
int ListFormatPopCount(int CompleteChildList, int LetterPosition){
	// This jump-table eliminates needless instructions and cumbersome conditional tests.
	const static void *PositionJumpTable[NUMBER_OF_ENGLISH_LETTERS] = { &&On, &&On, &&On, &&On, &&On, &&On, &&On, &&On, &&Tw, &&Tw, &&Tw, 
	&&Tw, &&Tw, &&Tw, &&Tw, &&Tw, &&Th, &&Th, &&Th, &&Th, &&Th, &&Th, &&Th, &&Th, &&Fo, &&Fo };
	int Result = 0;
	// By casting the agrument variable "CompleteChildList" as an "unsigned char *" we can access each byte within it using simple arithmetic.
	// Computer-programs in C use "little-endian" byte-order.  The least significant bit comes first.
	unsigned char *ByteZero = (unsigned char *)&CompleteChildList;
	
	// Mask "CompleteChildList" according to "LetterPosition".
	CompleteChildList &= ChildListMasks[LetterPosition];
	// Query the "PopCountTable" a minimum number of times.
	goto *PositionJumpTable[LetterPosition];
	Fo:
		Result += PopCountTable[*(ByteZero + 3)];
	Th:
		Result += PopCountTable[*(ByteZero + 2)];
	Tw:
		Result += PopCountTable[*(ByteZero + 1)];
	On:
		Result += PopCountTable[*ByteZero];
	return Result;
}

// Returns the positive "int" rerpresented by "TheNumberNotYet" string.  An invalid "TheNumberNotYet" returns "0".
int StringToPositiveInt(char* TheNumberNotYet){
	int Result = 0;
	int X;
	int Length = strlen(TheNumberNotYet);
	if ( Length > TEN ) return 0;
	for ( X = 0; X < Length; X++ ) {
		if ( TheNumberNotYet[X] < '0' || TheNumberNotYet[X] > '9' ) return 0;
		Result += ((TheNumberNotYet[X] - '0')*PowersOfTen[Length - X - 1 ]);
	}
	return Result;
}


// The "BinaryNode" string must be at least 32 + 5 + 1 bytes in length.  Space for the bits, the seperation pipes, and the end of string char.
// This function is used to fill the text file used to inspect the graph created in the first segment of the program.
void ConvertIntNodeToBinaryString(int TheNode, char *BinaryNode){
	int X;
	int Bit;
	BinaryNode[0] = '[';
	// Bit 31 is not being used.  It will always be '0'.
	BinaryNode[1] = '_';
	BinaryNode[2] = '|';
	// Bit 30 holds the End-Of-Word, EOW_FLAG.
	BinaryNode[3] = (TheNode & PowersOfTwo[30])?'1':'0';
	BinaryNode[4] = '|';
	// 13 Bits, (29-->17) represent the child-format index.
	Bit = 29;
	for ( X = 5; X <= 17; X++, Bit-- ) BinaryNode[X] = (TheNode & PowersOfTwo[Bit])?'1':'0';
	BinaryNode[18] = '|';
	// The "Child" index requires 17 bits, and it will complete the 32 bit int.
	Bit = 16;
	for ( X = 19; X <= 35; X++, Bit-- ) BinaryNode[X] = (TheNode & PowersOfTwo[Bit])?'1':'0';
	BinaryNode[36] = ']';
	BinaryNode[37] = '\0';
}


// The "BinaryChildList" string must be at least 32 + 3 + 1 bytes in length.  Space for the bits, the seperation pipes, and the end of string char.
void ConvertChildListIntToBinaryString(int TheChildList, char *BinaryChildList){
	int X;
	int Bit;
	BinaryChildList[0] = '[';
	// Bit 31 to bit 26 remain unused in a "ChildList".
	for ( X = 1; X <= 6; X++ ) BinaryChildList[X] = '_';
	BinaryChildList[7] = '|';
	Bit = 25;
	for ( X = 8; X <= 33; X++, Bit-- ) BinaryChildList[X] = (TheChildList & PowersOfTwo[Bit])?'1':'0';
	BinaryChildList[34] = ']';
	BinaryChildList[35] = '\0';
}

//This Function converts any lower case letters inside "RawWord" to capitals, so that the whole string is made of capital letters.
void MakeMeAllCapital(char *RawWord){
	unsigned int Count = 0;
	unsigned int Length = strlen(RawWord);
	for ( Count = 0; Count < Length; Count++ ) {
		if ( RawWord[Count] >= 'a' && RawWord[Count] <= 'z' ) RawWord[Count] -= LOWER_IT;
	}
}

/*Trie to Dawg TypeDefs*/
struct tnode {
	struct tnode* Next;
	struct tnode* Child;
	struct tnode* ParentalUnit;
	struct tnode* ReplaceMeWith;
	// When populating the DAWG array, you must know the index assigned to each "Child".
	// To get this information, "ArrayIndex" is stored in every node, so that we can mine the information from the reduced pointer-style-trie.
	int ArrayIndex;
	char DirectChild;
	char Letter;
	char MaxChildDepth;
	char Level;
	char NumberOfChildren;
	char Dangling;
	char Protected;
	char EndOfWordFlag;
};

typedef struct tnode Tnode;
typedef Tnode* TnodePtr;

// Functions to access internal "Tnode" members.
int TnodeArrayIndex(TnodePtr ThisTnode){
	return ThisTnode->ArrayIndex;
}

char TnodeDirectChild(TnodePtr ThisTnode){
	return ThisTnode->DirectChild;
}

TnodePtr TnodeNext(TnodePtr ThisTnode){
	return ThisTnode->Next;
}

TnodePtr TnodeChild (TnodePtr ThisTnode){
	return ThisTnode->Child;
}

TnodePtr TnodeParentalUnit(TnodePtr ThisTnode){
	return ThisTnode->ParentalUnit;
}

TnodePtr TnodeReplaceMeWith(TnodePtr ThisTnode){
	return ThisTnode->ReplaceMeWith;
}

char TnodeLetter(TnodePtr ThisTnode){
	return ThisTnode->Letter;
}

char TnodeMaxChildDepth(TnodePtr ThisTnode){
	return ThisTnode->MaxChildDepth;
}

char TnodeNumberOfChildren(TnodePtr ThisTnode){
	return ThisTnode->NumberOfChildren;
}

char TnodeEndOfWordFlag(TnodePtr ThisTnode){
	return ThisTnode->EndOfWordFlag;
}

char TnodeLevel(TnodePtr ThisTnode){
	return ThisTnode->Level;
}

char TnodeDangling(TnodePtr ThisTnode){
	return ThisTnode->Dangling;
}

char TnodeProtected(TnodePtr ThisTnode){
	return ThisTnode->Protected;
}

// Allocate a "Tnode" and fill it with initial values.
TnodePtr TnodeInit(char Chap, TnodePtr OverOne, char WordEnding, char Leveler, int StarterDepth, TnodePtr Parent, char IsaChild){
	TnodePtr Result = (Tnode *)malloc(sizeof(Tnode));
	Result->Letter = Chap;
	Result->ArrayIndex = 0;
	Result->NumberOfChildren = 0;
	Result->MaxChildDepth = StarterDepth;
	Result->Next = OverOne;
	Result->Child = NULL;
	Result->ParentalUnit = Parent;
	Result->Dangling = FALSE;
	Result->Protected = FALSE;
	Result->ReplaceMeWith = NULL;
	Result->EndOfWordFlag = WordEnding;
	Result->Level = Leveler;
	Result->DirectChild = IsaChild;
	return Result;
}

// Modify internal "Tnode" member values.
void TnodeSetArrayIndex(TnodePtr ThisTnode, int TheWhat){
	ThisTnode->ArrayIndex = TheWhat;
}

void TnodeSetChild(TnodePtr ThisTnode, TnodePtr Chump){
	ThisTnode->Child = Chump;
}
	
void TnodeSetNext(TnodePtr ThisTnode, TnodePtr Nexis){
	ThisTnode->Next = Nexis;
}

void TnodeSetParentalUnit(TnodePtr ThisTnode, TnodePtr Parent){
	ThisTnode->ParentalUnit = Parent;
}

void TnodeSetReplaceMeWith(TnodePtr ThisTnode, TnodePtr Living){
	ThisTnode->ReplaceMeWith = Living;
}

void TnodeSetMaxChildDepth(TnodePtr ThisTnode, int NewDepth){
	ThisTnode->MaxChildDepth = NewDepth;
}

void TnodeSetDirectChild(TnodePtr ThisTnode, char Status){
	ThisTnode->DirectChild = Status;
}

void TnodeFlyEndOfWordFlag(TnodePtr ThisTnode){
	ThisTnode->EndOfWordFlag = TRUE;
}

// This function Dangles a node, but also recursively dangles every node under it as well.
// Dangling a "Tnode" means that it will be exculded from the "DAWG".
// By recursion, nodes that are not direct children will get dangled.
// The function returns the total number of nodes dangled as a result.
int TnodeDangle(TnodePtr ThisTnode){
	if ( ThisTnode->Dangling == TRUE ) return 0;
	int Result = 0;
	if ( (ThisTnode->Next) != NULL ) Result += TnodeDangle(ThisTnode->Next);
	if ( (ThisTnode->Child) != NULL ) Result += TnodeDangle(ThisTnode->Child);
	ThisTnode->Dangling = TRUE;
	Result += 1;
	return Result;
}


// This function "Protects" a node being directly referenced in the elimination process.
// "Protected" nodes can be "Dangled", but special precautions need to be taken to ensure graph-integrity.
void TnodeProtect(TnodePtr ThisTnode){
	ThisTnode->Protected = TRUE;
}

// This function removes "Protection" status from "ThisNode".
void TnodeUnProtect(TnodePtr ThisTnode){
	ThisTnode->Protected = FALSE;
}

// This function returns a Boolean value indicating if a node coming after "ThisTnode" is "Protected".
// The Boolean argument "CheckThisNode" determines if "ThisTnode" is included in the search.
// A "Tnode" being eliminated with "Protected" nodes beneath it requires special precautions.
Bool TnodeProtectionCheck(TnodePtr ThisTnode, Bool CheckThisTnode){
	if ( ThisTnode == NULL ) return FALSE;
	if ( CheckThisTnode == TRUE ) if ( ThisTnode->Protected == TRUE ) return TRUE;
	if ( TnodeProtectionCheck(ThisTnode->Next, TRUE) == TRUE ) return TRUE;
	if ( TnodeProtectionCheck(ThisTnode->Child, TRUE) == TRUE ) return TRUE;
	return FALSE;
}

// This function returns the pointer to the Tnode in a parallel list of nodes with the letter "ThisLetter", and returns NULL if the Tnode does not exist.
// If the function returns NULL, then an insertion is required.
TnodePtr TnodeFindParaNode(TnodePtr ThisTnode, char ThisLetter){
	if ( ThisTnode == NULL ) return NULL;
	TnodePtr Result = ThisTnode;
	if ( Result->Letter == ThisLetter ) return Result;
	while ( Result->Letter < ThisLetter ) {
		Result = Result->Next;
		if ( Result == NULL ) return NULL;
	}
	if ( Result->Letter == ThisLetter ) return Result;
	else return NULL;
}

// This function inserts a new Tnode before a larger letter node or at the end of a para list If the list does not esist then it is put at the beginnung.  
// The new node has ThisLetter in it.  AboveTnode is the node 1 level above where the new node will be placed.
// This function should never be passed a "NULL" pointer.  "ThisLetter" should never exist in the "Child" para-list.
void TnodeInsertParaNode(TnodePtr AboveTnode, char ThisLetter, char WordEnder, int StartDepth){
	AboveTnode->NumberOfChildren += 1;
	TnodePtr Holder = NULL;
	TnodePtr Currently = NULL;
	// Case 1: ParaList does not exist yet so start it.
	if ( AboveTnode->Child == NULL ) AboveTnode->Child = TnodeInit(ThisLetter, NULL, WordEnder, AboveTnode->Level + 1, StartDepth, AboveTnode, TRUE);
	// Case 2: ThisLetter should be the first in the ParaList.
	else if ( ((AboveTnode->Child)->Letter) > ThisLetter ) {
		Holder = AboveTnode->Child;
		// The holder node is no longer a direct child so set it as such.
		TnodeSetDirectChild(Holder, FALSE);
		AboveTnode->Child = TnodeInit(ThisLetter, Holder, WordEnder, AboveTnode->Level + 1, StartDepth, AboveTnode, TRUE);
		// The parent node needs to be changed on what used to be the child. it is the Tnode in "Holder".
		Holder->ParentalUnit = AboveTnode->Child;
	}
	// Case 3: The ParaList exists and ThisLetter is not first in the list.
	else if ( (AboveTnode->Child)->Letter < ThisLetter ) {
		Currently = AboveTnode->Child;
		while ( Currently->Next !=NULL ) {
			if ( TnodeLetter(Currently->Next) > ThisLetter ) break;
			Currently = Currently->Next;
		}
		Holder = Currently->Next;
		Currently->Next = TnodeInit(ThisLetter, Holder, WordEnder, AboveTnode->Level + 1, StartDepth, Currently, FALSE);
		if ( Holder != NULL ) Holder->ParentalUnit = Currently->Next;
	}
}

// The "MaxChildDepth" of the two nodes cannot be assumed equal due to the recursive nature of this function, so we must check for equivalence.
char TnodeAreWeTheSame(TnodePtr FirstNode, TnodePtr SecondNode){
	if ( FirstNode == SecondNode ) return TRUE;
	if ( FirstNode == NULL || SecondNode == NULL ) return FALSE;
	if ( FirstNode->Letter != SecondNode->Letter ) return FALSE;
	if ( FirstNode->MaxChildDepth != SecondNode->MaxChildDepth ) return FALSE;
	if ( FirstNode->NumberOfChildren != SecondNode->NumberOfChildren ) return FALSE;
	if ( FirstNode->EndOfWordFlag != SecondNode->EndOfWordFlag ) return FALSE;
	if ( TnodeAreWeTheSame(FirstNode->Child, SecondNode->Child) == FALSE ) return FALSE;
	if ( TnodeAreWeTheSame(FirstNode->Next, SecondNode->Next) == FALSE ) return FALSE;
	else return TRUE;
}

struct dawg {
	int NumberOfTotalWords;
	int NumberOfTotalNodes;
	TnodePtr First;
};

typedef struct dawg Dawg;
typedef Dawg* DawgPtr;

// Set up the parent nodes in the Dawg.
DawgPtr DawgInit(void){
	DawgPtr Result = (Dawg *)malloc(sizeof(Dawg));
	Result->NumberOfTotalWords = 0;
	Result->NumberOfTotalNodes = 0;
	Result->First = TnodeInit('0', NULL, FALSE, 0, 0, NULL, FALSE);
	return Result;
}

// This function is responsible for adding "Word" to the "Dawg" under its root node.  It returns the number of new nodes inserted.
int TnodeDawgAddWord(TnodePtr ParentNode, const char *Word){
	int Result = 0;
	int X, Y = 0;
	int WordLength = strlen(Word);
	TnodePtr HangPoint = NULL;
	TnodePtr Current = ParentNode;
	for ( X = 0; X < WordLength; X++){
		HangPoint = TnodeFindParaNode(TnodeChild(Current), Word[X]);
		if ( HangPoint == NULL ) {
			TnodeInsertParaNode(Current, Word[X], (X == WordLength - 1 ? TRUE : FALSE), WordLength - X - 1);
			Result++;
			Current = TnodeFindParaNode(TnodeChild(Current), Word[X]);
			for ( Y = X + 1; Y < WordLength; Y++ ) {
				TnodeInsertParaNode(Current, Word[Y], (Y == WordLength - 1 ? TRUE : FALSE), WordLength - Y - 1);
				Result += 1;
				Current = TnodeChild(Current);
			}
			break;
		}
		else {
			if ( TnodeMaxChildDepth(HangPoint) < WordLength - X - 1 ) TnodeSetMaxChildDepth(HangPoint, WordLength - X - 1);
		}
		Current = HangPoint;
		// The path for the word that we are trying to insert already exists, so just make sure that the end flag is flying on the last node.
		// This should never happen if we are to add words in increasing word length.
		if ( X == WordLength - 1 ) TnodeFlyEndOfWordFlag(Current);
	}
	return Result;
}

// Add "NewWord" to "ThisDawg", which at this point is a "Trie" with a lot of information in each node.
// "NewWord" must not exist in "ThisDawg" already.
void DawgAddWord(DawgPtr ThisDawg, char * NewWord){
	ThisDawg->NumberOfTotalWords += 1;
	int NodesAdded = TnodeDawgAddWord(ThisDawg->First, NewWord);
	ThisDawg->NumberOfTotalNodes += NodesAdded;
}

// This is a standard depth first preorder tree traversal.
// The objective is to count "Living" "Tnodes" of various "MaxChildDepths", and store these values into "Tabulator".
void TnodeGraphTabulateRecurse(TnodePtr ThisTnode, int Level, int* Tabulator){
	if ( Level == 0 ) TnodeGraphTabulateRecurse(TnodeChild(ThisTnode), 1, Tabulator);
	else {
		// We will only ever be concerned with "Living" nodes.  "Dangling" Nodes will be eliminated, so don't count them.
		if ( ThisTnode->Dangling == FALSE ) {
			Tabulator[ThisTnode->MaxChildDepth] += 1;
			// Go Down if possible.
			if ( ThisTnode->Child != NULL ) TnodeGraphTabulateRecurse(TnodeChild(ThisTnode), Level + 1, Tabulator);
			// Go Right if possible.
			if ( ThisTnode->Next != NULL ) TnodeGraphTabulateRecurse(TnodeNext(ThisTnode), Level, Tabulator);
		}
	}
}

// Count the "Living" "Tnode"s of each "MaxChildDepth" in "ThisDawg", and store the values in "Count".
void DawgGraphTabulate(DawgPtr ThisDawg, int* Count){
	int Numbers[MAX];
	int X = 0;
	for ( X = 0; X < MAX; X++ ) Numbers[X] = 0;
	if ( ThisDawg->NumberOfTotalWords > 0 ) {
		TnodeGraphTabulateRecurse(ThisDawg->First, 0, Numbers);
		for ( X = 0; X < MAX; X++ ) {
			Count[X] = Numbers[X];
		}
	}
}

// This function can never be called after a trie has been "Mowed" because this will result in pointers being freed twice.
// A core dump is bad.
void FreeTnodeRecurse(TnodePtr ThisTnode){
	if ( ThisTnode->Child != NULL ) FreeTnodeRecurse(ThisTnode->Child);
	if ( ThisTnode->Next != NULL ) FreeTnodeRecurse(ThisTnode->Next);
	free(ThisTnode);
}

// This function can never be called after a trie has been "Mowed" because this will result in pointers being freed twice.
// A core dump is bad.
void FreeDawg(DawgPtr ThisDawg){
	if ( ThisDawg->NumberOfTotalWords > 0 ) {
		FreeTnodeRecurse(ThisDawg->First);
	}
	free(ThisDawg);
}

// Recursively replaces all redundant nodes under "ThisTnode".
// "DirectChild" "Tnode" in a "Dangling" state have "ReplaceMeWith" set within them.
// Crosslinking of "Tnode"s being eliminated must be taken-care-of before this function gets called.
// When "Tnode" branches are crosslinked, the living branch has members being replaced with "Tnode"s in the branch being killed.
void TnodeLawnMowerRecurse(TnodePtr ThisTnode){
	if ( ThisTnode->Level == 0 ) TnodeLawnMowerRecurse(ThisTnode->Child);
	else {
		if ( ThisTnode->Next == NULL && ThisTnode->Child == NULL ) return;
		// The first "Tnode" being eliminated will always be a "DirectChild".
		if ( ThisTnode->Child != NULL ) {
			// The node is tagged to be "Mowed", so replace it with "ReplaceMeWith", keep following "ReplaceMeWith" until an un"Dangling" "Tnode" is found.
			if ( (ThisTnode->Child)->Dangling == TRUE ) {
				ThisTnode->Child = TnodeReplaceMeWith(ThisTnode->Child);
				while ( (ThisTnode->Child)->Dangling == TRUE ) ThisTnode->Child = TnodeReplaceMeWith(ThisTnode->Child);
			}
			else {
				TnodeLawnMowerRecurse(ThisTnode->Child);
			}
		}
		if ( ThisTnode->Next != NULL ){
			TnodeLawnMowerRecurse(ThisTnode->Next);
		}
	}
}

// Replaces all pointers to "Dangling" "Tnodes" in the "ThisDawg" Trie with living ones.
void DawgLawnMower(DawgPtr ThisDawg){
	TnodeLawnMowerRecurse(ThisDawg->First);
}

// This function accepts two identical node branch structures, "EliminateThis" and "ReplaceWith".  It is recursive.
// The Boolean "ValidReplacement" determines if the function will check for "Crosslink"s or alter "Protected" and "ReplaceMeWith" states.
// When "ValidReplacement" is ON, it must set "ReplaceMeWith" values for "Protected" nodes under "EliminateThis".
// It will also assign "Protected" status to the corresponding nodes under "ReplaceWith".
// This function returns "FALSE" if a "Crosslink" is found.  It returns "TRUE" if replacement is a GO.
Bool TnodeProtectAndReplaceBranchStructure(TnodePtr EliminateThis, TnodePtr ReplaceWith, Bool ValidReplacement){
	if ( EliminateThis == NULL || ReplaceWith == NULL) return TRUE;
	if ( TnodeProtected(EliminateThis) == TRUE ) {
		if ( TnodeDangling(ReplaceWith) == TRUE ) {
			// Though we verify the "Crosslink" condition for confirmation, the two conditions above guarantee the condition below.
			if ( EliminateThis == TnodeReplaceMeWith(ReplaceWith) ) {
				// In the case of a confirmed "Crosslink", "EliminateThis" will be a "DirectChild".
				// The logic is that "EliminateThis" and "ReplaceWith" have the same structure.
				// Since "ReplaceWith" is "Dangling" with a valid "ReplaceMeWith", it is a "DirectChild".
				// Thus "EliminateThis" must also be a "DirectChild".  Verify this.
				printf("\n   -***- Crosslink found, so exchange branches first, then Dangle the unProtected Tnodes.\n");
				// Resort to drastic measures:  Simply flip the actual nodes in the Trie.
				(ReplaceWith->ParentalUnit)->Child = EliminateThis;
				(EliminateThis->ParentalUnit)->Child = ReplaceWith;
				return FALSE;
			}
		}
		// When "ValidReplacement" is set, if "EliminateThis" is a protected "Tnode", "ReplaceWith" isn't "Dangling".
		// We can "Dangle" "Protected" "Tnodes", as long as we set a proper "ReplaceMeWith" value for them.
		// Since we are now pointing "Tnode"s at "ReplaceWith", protect it.
		if ( ValidReplacement ) {
			TnodeSetReplaceMeWith(EliminateThis, ReplaceWith);
			TnodeProtect(ReplaceWith);
		}
	}
	if ( TnodeProtectAndReplaceBranchStructure(EliminateThis->Next, ReplaceWith->Next, ValidReplacement) == FALSE ) return FALSE;
	if ( TnodeProtectAndReplaceBranchStructure(EliminateThis->Child, ReplaceWith->Child, ValidReplacement) == TRUE ) return TRUE;
	else return FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// A queue is required for breadth first traversal, and the rest is self-evident.

struct breadthqueuenode {
	TnodePtr Element;
	struct breadthqueuenode *Next;
};

typedef struct breadthqueuenode BreadthQueueNode;
typedef BreadthQueueNode* BreadthQueueNodePtr;

void BreadthQueueNodeSetNext(BreadthQueueNodePtr ThisBreadthQueueNode, BreadthQueueNodePtr Nexit){
		ThisBreadthQueueNode->Next = Nexit;
}

BreadthQueueNodePtr BreadthQueueNodeNext(BreadthQueueNodePtr ThisBreadthQueueNode){
		return ThisBreadthQueueNode->Next;
}

TnodePtr BreadthQueueNodeElement(BreadthQueueNodePtr ThisBreadthQueueNode){
		return ThisBreadthQueueNode->Element;
}

BreadthQueueNodePtr BreadthQueueNodeInit(TnodePtr NewElement){
	BreadthQueueNodePtr Result = (BreadthQueueNode *)malloc(sizeof(BreadthQueueNode));
	Result->Element = NewElement;
	Result->Next = NULL;
	return Result;
}

struct breadthqueue {
	BreadthQueueNodePtr Front;
	BreadthQueueNodePtr Back;
	int Size;
};

typedef struct breadthqueue BreadthQueue;
typedef BreadthQueue* BreadthQueuePtr;

BreadthQueuePtr BreadthQueueInit(void){
	BreadthQueuePtr Result = (BreadthQueue *)malloc(sizeof(BreadthQueue));
	Result->Front = NULL;
	Result->Back = NULL;
	Result->Size = 0;
}

void BreadthQueuePush(BreadthQueuePtr ThisBreadthQueue, TnodePtr NewElemental){
	BreadthQueueNodePtr Noob = BreadthQueueNodeInit(NewElemental);
	if ( (ThisBreadthQueue->Back) != NULL ) BreadthQueueNodeSetNext(ThisBreadthQueue->Back, Noob);
	else ThisBreadthQueue->Front = Noob;
	ThisBreadthQueue->Back = Noob;
	(ThisBreadthQueue->Size) += 1;
}

TnodePtr BreadthQueuePop(BreadthQueuePtr ThisBreadthQueue){
	if ( ThisBreadthQueue->Size == 0 ) return NULL;
	if ( ThisBreadthQueue->Size == 1 ) {
		ThisBreadthQueue->Back = NULL;
		ThisBreadthQueue->Size = 0;
		TnodePtr Result = (ThisBreadthQueue->Front)->Element;
		free(ThisBreadthQueue->Front);
		ThisBreadthQueue->Front = NULL;
		return Result;
	}
	TnodePtr Result = (ThisBreadthQueue->Front)->Element;
	BreadthQueueNodePtr Holder = ThisBreadthQueue->Front;
	ThisBreadthQueue->Front = (ThisBreadthQueue->Front)->Next;
	free(Holder);
	ThisBreadthQueue->Size -= 1;
	return Result;
}


// For the "Tnode" "Dangling" process, arrange the "Tnodes" in the "Holder" array, with breadth-first traversal order.
void BreadthQueuePopulateReductionArray(BreadthQueuePtr ThisBreadthQueue, TnodePtr Root, TnodePtr **Holder){
	int Taker[MAX];
	int X = 0;
	memset(Taker, 0, MAX*sizeof(int));
	int CurrentMaxChildDepth = 0;
	TnodePtr Current = Root;
	// Push the first row onto the queue.
	while ( Current != NULL ) {
		BreadthQueuePush(ThisBreadthQueue, Current);
		Current = Current->Next;
	}
	// Initiate the pop followed by push all children loop.
	while ( (ThisBreadthQueue->Size) != 0 ) {
		Current = BreadthQueuePop(ThisBreadthQueue);
		CurrentMaxChildDepth = Current->MaxChildDepth;
		Holder[CurrentMaxChildDepth][Taker[CurrentMaxChildDepth]] = Current;
		Taker[CurrentMaxChildDepth] += 1;
		Current = TnodeChild(Current);
		while ( Current != NULL ) {
			BreadthQueuePush(ThisBreadthQueue, Current);
			Current = TnodeNext(Current);
		}
	}
}


// It is of absolutely critical importance that only "DirectChild" nodes are pushed onto the queue as child nodes.  This will not always be the case.
// In a DAWG a child pointer may point to an internal node in a longer list.  Check for this.
int BreadthQueueUseToIndex(BreadthQueuePtr ThisBreadthQueue, TnodePtr Root){
	int IndexNow = 0;
	TnodePtr Current = Root;
	// Push the first row onto the queue.
	while ( Current != NULL ) {
		BreadthQueuePush(ThisBreadthQueue, Current);
		Current = Current->Next;
	}
	// Pop each element off of the queue and only push its children if has not been "Dangled" yet.  Assign index if one has not been given to it yet.
	while ( (ThisBreadthQueue->Size) != 0 ) {
		Current = BreadthQueuePop(ThisBreadthQueue);
		// A traversal of the Trie will never land on "Dangling" "Tnodes", but it will try to visit certain "Tnodes" many times.
		if ( TnodeArrayIndex(Current) == 0 ) {
			IndexNow += 1;
			TnodeSetArrayIndex(Current, IndexNow);
			Current = TnodeChild(Current);
			if ( Current != NULL ) {
				// The graph will lead to intermediate positions, but we cannot start numbering "Tnodes" from the middle of a list.
				if ( TnodeDirectChild(Current) == TRUE && TnodeArrayIndex(Current) == 0 ) {
					while ( Current != NULL ) {
						BreadthQueuePush(ThisBreadthQueue, Current);
						Current = Current->Next;
					}
				}
			}
		}
	}
	return IndexNow;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Next and Child become indices.
struct arraydnode{
	int Next;
	int Child;
	char Letter;
	char EndOfWordFlag;
	char Level;
};

typedef struct arraydnode ArrayDnode;
typedef ArrayDnode* ArrayDnodePtr;

void ArrayDnodeInit(ArrayDnodePtr ThisArrayDnode, char Chap, int Nextt, int Childd, char EndingFlag, char Breadth){
	ThisArrayDnode->Letter = Chap;
	ThisArrayDnode->EndOfWordFlag = EndingFlag;
	ThisArrayDnode->Next = Nextt;
	ThisArrayDnode->Child = Childd;
	ThisArrayDnode->Level = Breadth;
}

void ArrayDnodeTnodeTranspose(ArrayDnodePtr ThisArrayDnode, TnodePtr ThisTnode){
	ThisArrayDnode->Letter = ThisTnode->Letter;
	ThisArrayDnode->EndOfWordFlag = ThisTnode->EndOfWordFlag;
	ThisArrayDnode->Level = ThisTnode->Level;
	if ( ThisTnode->Next == NULL ) ThisArrayDnode->Next = 0;
	else ThisArrayDnode->Next = (ThisTnode->Next)->ArrayIndex;
	if ( ThisTnode->Child == NULL ) ThisArrayDnode->Child = 0;
	else ThisArrayDnode->Child = (ThisTnode->Child)->ArrayIndex;
}

int ArrayDnodeNext(ArrayDnodePtr ThisArrayDnode){
	return ThisArrayDnode->Next;
}

int ArrayDnodeChild (ArrayDnodePtr ThisArrayDnode){
	return ThisArrayDnode->Child;
}

char ArrayDnodeLetter(ArrayDnodePtr ThisArrayDnode){
	return ThisArrayDnode->Letter;
}

char ArrayDnodeEndOfWordFlag(ArrayDnodePtr ThisArrayDnode){
	return ThisArrayDnode->EndOfWordFlag;
}

int ArrayDnodeNumberOfChildrenPlusString(ArrayDnodePtr DoggieDog, int Index, char* FillThisString){
	int X;
	int CurrentArrayPosition;
	
	if ( (DoggieDog[Index]).Child == 0 ) {
		FillThisString[0] = '\0';
		return 0;
	}
	CurrentArrayPosition = (DoggieDog[Index]).Child;
	for ( X = 0; X < NUMBER_OF_ENGLISH_LETTERS; X++ ) {
		FillThisString[X] = (DoggieDog[CurrentArrayPosition]).Letter;
		if ( (DoggieDog[CurrentArrayPosition]).Next == 0 ) {
			FillThisString[X + 1] = '\0';
			return (X + 1);
		}
		CurrentArrayPosition += 1;
	}
}

struct arraydawg {
	int NumberOfStrings;
	ArrayDnodePtr DawgArray;
	int First;
	char MinStringLength;
	char MaxStringLength;
};

typedef struct arraydawg ArrayDawg;
typedef ArrayDawg* ArrayDawgPtr;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int CrossLinkCount = 0;

// This function is the core of the dawg creation procedure.  Pay close attention to the order of the steps involved.

ArrayDawgPtr ArrayDawgInit(char **Dictionary, int *SegmentLenghts, int MaxStringLength){
	int X = 0;
	int Y = 0;
	int *ChildCount;
	char *ChildStrings;
	
	printf("Step 0 - Allocate the framework for the intermediate Array-Data-Structure.\n");
	// Dynamically allocate the upper Data-Structure.
	ArrayDawgPtr Result = (ArrayDawgPtr)malloc(sizeof(ArrayDawg));
	// set MinStringLength, MaxStringLength, and NumberOfStrings.
	
	while ( SegmentLenghts[X] == 0 ) X++;
	Result->MinStringLength = X;
	Result->MaxStringLength = MaxStringLength;
	Result->NumberOfStrings = 0;
	for ( X = Result->MinStringLength; X <= Result->MaxStringLength ; X++ ) Result->NumberOfStrings += SegmentLenghts[X];

	printf("\nStep 1 - Create a Temporary-Working-Trie and begin filling it with the |%d| words.\n", Result->NumberOfStrings);
	/// Create a Temp Trie structure and then feed in the given dictionary.
	DawgPtr TemporaryTrie = DawgInit();
	for ( Y = Result->MinStringLength; Y <= Result->MaxStringLength; Y++ ) {
		for ( X = 0; X < SegmentLenghts[Y]; X++ ) {
			DawgAddWord(TemporaryTrie, &(Dictionary[Y][(Y + 1)*X]));
		}
	}

	printf("\nStep 2 - Finished adding words to the Temporary-Working-Trie.\n");
	// Allocate two "Tnode" counter arrays.
	int *NodeNumberCounter = (int*)calloc((Result->MaxStringLength), sizeof(int));
	int *NodeNumberCounterInit = (int*)calloc((Result->MaxStringLength), sizeof(int));
	
	// Count up the number of "Tnode"s in the Raw-Trie according to MaxChildDepth.
	printf("\nStep 3 - Count the total number of Tnodes in the Raw-Trie according to MaxChildDepth.\n");
	DawgGraphTabulate(TemporaryTrie, NodeNumberCounter);
	
	printf("\nStep 4 - Initial Tnode counting is complete, so display results:\n\n");
	int TotalNodeSum = 0;
	for ( X = 0; X < Result->MaxStringLength; X++ ){
	NodeNumberCounterInit[X] = NodeNumberCounter[X];
		TotalNodeSum += NodeNumberCounter[X];
	}
	for ( X = 0; X < Result->MaxStringLength; X++ ){
		printf("  Initial Tnode Count For MaxChildDepth =|%2d| is |%6d|\n", X, NodeNumberCounterInit[X]);
	}
	printf("\n  Total Tnode Count For The Raw-Trie = |%d| \n", TotalNodeSum);
	// We will have exactly enough space for all of the Tnode pointers.

	printf("\nStep 5 - Allocate a 2 dimensional array of Tnode pointers to search for redundant Tnodes.\n");
	TnodePtr ** HolderOfAllTnodePointers = (TnodePtr **)malloc((Result->MaxStringLength)*sizeof(TnodePtr *));
	for ( X = 0; X < MAX; X++ ) HolderOfAllTnodePointers[X] = (TnodePtr *)malloc(NodeNumberCounterInit[X]*sizeof(TnodePtr));
	// A breadth-first traversal is used when populating the final array.
	// It is then much more likely for living "Tnode"s to appear first, if we fill "HolderOfAllTnodePointers" breadth first.

	printf("\nStep 6 - Populate the 2 dimensional Tnode pointer array.\n");
	// Use a breadth first traversal to populate the "HolderOfAllTnodePointers" array.
	BreadthQueuePtr Populator = BreadthQueueInit();
	BreadthQueuePopulateReductionArray(Populator, (TemporaryTrie->First)->Child, HolderOfAllTnodePointers);

	printf("\nStep 7 - Population complete.\n");
	// Flag all of the reduntant "Tnode"s, and store a "ReplaceMeWith" "Tnode" reference inside the "Dangling" "Tnode"s.
	// Flagging requires the "TnodeAreWeTheSame()" function, and beginning with the highest "MaxChildDepth" "Tnode"s will reduce the processing time.
	int NumberDangled = 0;
	int DangledNow;
	int NumberAtHeight;
	int TotalDangled = 0;
	int W = 0;
	
	// keep track of the number of nodes of each MaxChildDepth dangled recursively so we can check how many remaining nodes we need for the optimal array.
	int DangleCount[Result->MaxStringLength];
	for ( X = 0; X < Result->MaxStringLength; X++) DangleCount[X] = 0;

	printf("\nStep 8 - Tag redundant Tnodes as Dangling - Use recursion, because only DirectChild Tnodes are considered for elimination:\n");
	printf("\n  This procedure is at the very heart of the CWG creation alogirthm, and it would be much slower, without heavy optimization.\n");
	printf("\n  ---------------------------------------------------------------------------------------------------------------------------\n");
	// *** Test the other way.  Start at the largest "MaxChildDepth" and work down from there for recursive reduction to take place.
	for ( Y = Result->MaxStringLength - 1; Y >= 0; Y-- ) {
		NumberDangled = 0;
		NumberAtHeight = 0;
		// "X" is the index of the node we are trying to kill.
		for ( X = NodeNumberCounterInit[Y] - 1; X >= 0; X-- ) {
			// If the node is "Dangling" already, or it is not a "DirectChild", then "continue".
			if ( TnodeDangling(HolderOfAllTnodePointers[Y][X]) == TRUE ) continue;
			if ( TnodeDirectChild(HolderOfAllTnodePointers[Y][X]) == FALSE ) continue;
			// Make sure that we don't emiminate "Tnodes" being pointed to by other "Tnodes" in the graph.
			// This is a tricky procedure because node beneath "X" can be "Protected".
			// "W" will be the index of the first undangled "Tnode" with the same structure, if one exists.
			for ( W = 0; W < NodeNumberCounterInit[Y]; W++ ) {
				if ( W == X ) continue;
				if ( TnodeDangling(HolderOfAllTnodePointers[Y][W]) == FALSE ) {
					if ( TnodeAreWeTheSame(HolderOfAllTnodePointers[Y][X], HolderOfAllTnodePointers[Y][W]) == TRUE ) {
						// In the special case where the node being "Dangled" has "Protected" nodes beneath it, more needs to be done.
						// When we "Dangle" a "Protected" "Tnode", we must set it's "ReplaceMeWith", and a recursive function is needed for this special case.
						// This construct deals with regular and "Crosslink"ed branch structures.
						// It happens when "Protected" "Tnodes come beneath the one we want to "Dangle".
						if ( TnodeProtectionCheck(HolderOfAllTnodePointers[Y][X], FALSE) == TRUE ) {
							while ( !TnodeProtectAndReplaceBranchStructure(HolderOfAllTnodePointers[Y][X], HolderOfAllTnodePointers[Y][W], FALSE) ) ;
							TnodeProtectAndReplaceBranchStructure(HolderOfAllTnodePointers[Y][X], HolderOfAllTnodePointers[Y][W], TRUE);
						}
						// Set the "Protected" and "ReplaceMeWith" status of the corresponding top-level "Tnode"s.
						TnodeProtect(HolderOfAllTnodePointers[Y][W]);
						TnodeSetReplaceMeWith(HolderOfAllTnodePointers[Y][X], HolderOfAllTnodePointers[Y][W]);
						// "Dangle" all nodes under "HolderOfAllTnodePointers[Y][X]", and update the "Dangle" counters.
						NumberAtHeight += 1;
						DangledNow = TnodeDangle(HolderOfAllTnodePointers[Y][X]);
						NumberDangled += DangledNow;
						break;
					}
				}
			}
		}
		printf("  Dangled |%5d| Tnodes, and |%5d| Tnodes In all, through recursion, for MaxChildDepth of |%2d|\n", NumberAtHeight, NumberDangled, Y);
		DangleCount[Y] = NumberDangled;
		TotalDangled += NumberDangled;
	}
	printf("  ---------------------------------------------------------------------------------------------------------------------------\n");
	
	int NumberOfLivingNodes;
	printf("\n  |%6d| = Original # of Tnodes.\n", TotalNodeSum);	
	printf("  |%6d| = Dangled # of Tnodes.\n", TotalDangled);
	printf("  |%6d| = Remaining # of Tnodes.\n", NumberOfLivingNodes = TotalNodeSum - TotalDangled);

	printf("\nStep 9 - Count the number of living Tnodes by traversing the Raw-Trie to check the Dangling numbers.\n\n");
	DawgGraphTabulate(TemporaryTrie, NodeNumberCounter);
	for ( X = 0; X < Result->MaxStringLength; X++ ) {
		printf("  New count for MaxChildDepth |%2d| Tnodes is |%5d|.  Tnode count was |%6d| in Raw-Trie pre-Dangling.  Killed |%6d| Tnodes.\n"
		, X, NodeNumberCounter[X], NodeNumberCounterInit[X], NodeNumberCounterInit[X] - NodeNumberCounter[X]);
	}
	int TotalDangledCheck = 0;
	for ( X = 0; X < MAX; X++ ) {
		TotalDangledCheck += (NodeNumberCounterInit[X] - NodeNumberCounter[X]);
	}
	if ( TotalDangled == TotalDangledCheck ) printf("\n  Tnode Dangling count is consistent.\n");
	else printf("\n  MISMATCH for Tnode Dangling count.\n");
	
	printf("\nStep 9.5 - Run a final check to verify that all redundant nodes have been Dangled.\n\n");
	
	for ( Y = Result->MaxStringLength - 1; Y >= 0; Y-- ) {
		NumberAtHeight = 0;
		// "X" is the index of the node we are trying to kill.
		for ( X = NodeNumberCounterInit[Y] - 1; X >= 0; X-- ) {
			// If the node is "Dangling" already, or it is not a "DirectChild", then "continue".
			if ( TnodeDangling(HolderOfAllTnodePointers[Y][X]) == TRUE ) continue;
			if ( TnodeDirectChild(HolderOfAllTnodePointers[Y][X]) == FALSE ) continue;
			// "W" will be the index of the first undangled "Tnode" with the same structure, if one slipped through the cracks.
			for ( W = 0; W < NodeNumberCounterInit[Y]; W++ ) {
				if ( W == X ) continue;
				if ( TnodeDangling(HolderOfAllTnodePointers[Y][W]) == FALSE ) {
					if ( TnodeAreWeTheSame(HolderOfAllTnodePointers[Y][X], HolderOfAllTnodePointers[Y][W]) == TRUE ) {
						NumberAtHeight += 1;
						break;
					}
				}
			}
		}
		printf("  MaxChildDepth |%2d| - Identical living nodes found = |%2d|.\n", Y, NumberAtHeight);
	}

	printf("\nstep 10 - Kill the Dangling Tnodes using the internal \"ReplaceMeWith\" values.\n");
	// Node replacement has to take place before indices are set up so nothing points to redundant nodes.
	// - This step is absolutely critical.  Mow The Lawn so to speak!  Then Index.
	DawgLawnMower(TemporaryTrie);
	printf("\n  Killing complete.\n");

	printf("\nStep 11 - Dawg-Lawn-Mowing is now complete, so assign array indexes to all living Tnodes using a Breadth-First-Queue.\n");
	BreadthQueuePtr OrderMatters = BreadthQueueInit();
	// The Breadth-First-Queue must assign an index value to each living "Tnode" only once.
	// "HolderOfAllTnodePointers[MAX - 1][0]" becomes the first node in the new DAWG array.
	int IndexCount = BreadthQueueUseToIndex(OrderMatters, HolderOfAllTnodePointers[MAX - 1][0]);
	printf("\n  Index assignment is now complete.\n");
	printf("\n  |%d| = NumberOfLivingNodes from after the Dangling process.\n", NumberOfLivingNodes);
	printf("  |%d| = IndexCount from the breadth-first assignment function.\n", IndexCount);

	// Allocate the space needed to store the "DawgArray".
	Result->DawgArray = (ArrayDnodePtr)calloc((NumberOfLivingNodes + 1), sizeof(ArrayDnode));
	int IndexFollow = 0;
	int IndexFollower = 0;
	int TransposeCount = 0;
	// Roll through the pointer arrays and use the "ArrayDnodeTnodeTranspose" function to populate it.
	// Set the dummy entry at the beginning of the array.
	ArrayDnodeInit(&(Result->DawgArray[0]), 0, 0, 0, 0, 0);
	Result->First = 1;

	printf("\nStep 12 - Populate the new Working-Array-Dawg structure, which is used to verify validity and create the final integer-graph-encodings.\n");
	// Scroll through "HolderOfAllTnodePointers" and look for un"Dangling" "Tnodes", if so then transpose them into "Result->DawgArray".
	for ( X = Result->MaxStringLength - 1; X >= 0; X-- ) {
		for (W = 0; W < NodeNumberCounterInit[X]; W++ ) {
			if ( TnodeDangling(HolderOfAllTnodePointers[X][W]) == FALSE ) {
				IndexFollow = TnodeArrayIndex(HolderOfAllTnodePointers[X][W]);
				ArrayDnodeTnodeTranspose(&(Result->DawgArray[IndexFollow]), HolderOfAllTnodePointers[X][W]);
				TransposeCount += 1;
				if ( IndexFollow > IndexFollower ) IndexFollower = IndexFollow;
			}
		}
	}
	printf("\n  |%d| = IndexFollower, which is the largest index assigned in the Working-Array-Dawg.\n", IndexFollower);
	printf("  |%d| = TransposeCount, holds the number of Tnodes transposed into the Working-Array-Dawg.\n", TransposeCount);
	printf("  |%d| = NumberOfLivingNodes.  Make sure that these three values are equal, because they must be.\n", NumberOfLivingNodes);
	if ( (IndexFollower == TransposeCount) && (IndexFollower == NumberOfLivingNodes) ) printf("\n  Equality assertion passed.\n");
	else printf("\n  Equality assertion failed.\n");
	
	// Conduct dynamic-memory-cleanup and free the whole Raw-Trie, which is no longer needed.
	for ( X = 0; X < Result->MaxStringLength; X++ ) for ( W = 0; W < NodeNumberCounterInit[X]; W++ ) free(HolderOfAllTnodePointers[X][W]);
	free(TemporaryTrie);
	free(NodeNumberCounter);
	free(NodeNumberCounterInit);
	for ( X = 0; X < Result->MaxStringLength; X++ ) free(HolderOfAllTnodePointers[X]);
	free(HolderOfAllTnodePointers);
	
	printf("\nStep 13 - Creation of the traditional-DAWG is complete, so store it in a binary file for use.\n");
	
	FILE *Data;
	Data = fopen( TRADITIONAL_DAWG_DATA,"wb" );
	// The "NULL" node in position "0" must be counted now.
	int CurrentNodeInteger = NumberOfLivingNodes + 1;
	// It is critical, especially in a binary file, that the first integer written to the file be the number of nodes stored in the file.
	fwrite( &CurrentNodeInteger, sizeof(int), 1, Data );
	// Write the "NULL" node to the file first.
	CurrentNodeInteger = 0;
	fwrite( &CurrentNodeInteger, sizeof(int), 1, Data );
	for ( X = 1; X <= NumberOfLivingNodes ; X++ ){
		CurrentNodeInteger = (Result->DawgArray)[X].Child;
		CurrentNodeInteger <<= TRADITIONAL_CHILD_SHIFT;
		CurrentNodeInteger += ((Result->DawgArray)[X].Letter) - 'A';
		if ( (Result->DawgArray)[X].EndOfWordFlag == TRUE ) CurrentNodeInteger += TRADITIONAL_EOW_FLAG;
		if ( (Result->DawgArray)[X].Next == 0 ) CurrentNodeInteger += TRADITIONAL_EOL_FLAG;
		fwrite( &CurrentNodeInteger, sizeof(int), 1, Data );
	}
	fclose(Data);
	printf( "\n  The Traditional-DAWG-Encoding data file is now written.\n" );
	
	printf("\nStep 14 - Create a preliminary encoding of the more advanced CWG, and store these intermediate arrays into data files.\n");
	
	FILE *Text;
	Text = fopen(TRADITIONAL_DAWG_TEXT_DATA,"w");
	
	FILE *Main;
	FILE *Secondary;
	Main = fopen(DIRECT_GRAPH_DATA_PART_ONE,"wb" );
	Secondary = fopen(DIRECT_GRAPH_DATA_PART_TWO,"wb" );
	
	// The following variables will be used when setting up the child-List-Format integer values.
	int CurrentNumberOfChildren = 0;
	
	char CurrentChildLetterString[NUMBER_OF_ENGLISH_LETTERS + 1];
	CurrentChildLetterString[0] = '\0';
	char TheNodeInBinary[32+5+1];
	char TheChildListInBinary[32+3+1];
	TheNodeInBinary[0] = '\0';
	
	int CompleteChildList;
	int CurrentOffsetNumberIndex;
	int CompleteThirtyTwoBitNode;
	fwrite(&NumberOfLivingNodes, 4, 1, Main);
	
	FILE *ListE;
	int EndOfListCount = 0;
	int EOLTracker = 0;
	int *EndOfListIndicies;
	ListE = fopen(FINAL_NODES_DATA, "wb");
	
	// Set up an array to hold all of the unique child strings for the reduced lexicon DAWG.  The empty placeholder will be all zeds.
	int NumberOfUniqueChildStrings = 0;
	int InsertionPoint = 0;
	Bool IsSheUnique = TRUE;
	char *TempHolder;
	char **HolderOfUniqueChildStrings = (char**)malloc(NumberOfLivingNodes * sizeof(char*));
	for ( X = 0; X < NumberOfLivingNodes; X++ ) {
		HolderOfUniqueChildStrings[X] = (char*)malloc((NUMBER_OF_ENGLISH_LETTERS + 1) * sizeof(char));
		strcpy(HolderOfUniqueChildStrings[X], "ZZZZZZZZZZZZZZZZZZZZZZZZZZ");
	}
	
	// Right here we will tabulate the child information so that it can be turned into an "int" array and stored in a data file.
	// Also, we need to count the number of unique list structures, and calculate the number of bits required to store index values for them.
	// The idea is that there are a small number of actual values that these 26 bits will hold due to patterns in the English Language.
	for ( X = 1; X <= NumberOfLivingNodes ; X++ ) {
		CurrentNumberOfChildren = ArrayDnodeNumberOfChildrenPlusString(Result->DawgArray, X, CurrentChildLetterString);
		// Insert the "CurrentChildLetterString" into the "HolderOfUniqueChildStrings" if, and only if, it is unique.
		for ( Y = 0; Y <= NumberOfUniqueChildStrings; Y++ ) {
			if ( strcmp(CurrentChildLetterString, HolderOfUniqueChildStrings[Y]) == 0 ) {
				IsSheUnique = FALSE;
				InsertionPoint = 0;
				break;
			}
			if ( strcmp(CurrentChildLetterString, HolderOfUniqueChildStrings[Y]) < 0 ) {
				IsSheUnique = TRUE;
				InsertionPoint = Y;
				break;
			}
		}
		if ( IsSheUnique == TRUE ) {
			TempHolder = HolderOfUniqueChildStrings[NumberOfUniqueChildStrings];
			strcpy(TempHolder, CurrentChildLetterString);
			memmove(HolderOfUniqueChildStrings + InsertionPoint + 1, HolderOfUniqueChildStrings + InsertionPoint
			, (NumberOfUniqueChildStrings - InsertionPoint)*sizeof(char*));
			HolderOfUniqueChildStrings[InsertionPoint] = TempHolder;
			NumberOfUniqueChildStrings += 1;
		}
	}
	
	printf("\nStep 15 - NumberOfUniqueChildStrings = |%d|.\n", NumberOfUniqueChildStrings);
	
	int *ChildListValues = (int*)malloc(NumberOfUniqueChildStrings*sizeof(int));
	
	// Encode the unique child strings as "int"s, so that each corresponding bit is popped.
	for ( X = 0; X < NumberOfUniqueChildStrings ; X++ ) {
		strcpy(CurrentChildLetterString, HolderOfUniqueChildStrings[X]);
		CurrentNumberOfChildren = strlen(CurrentChildLetterString);
		CompleteChildList = 0;
		for ( Y = 0; Y < CurrentNumberOfChildren; Y++ ) {
			CompleteChildList += PowersOfTwo[CurrentChildLetterString[Y] - 'A'];
		}
		ChildListValues[X] = CompleteChildList;
	}
	
	fprintf(Text, "Behold, the |%d| graph nodes are decoded below.\r\n\r\n", NumberOfLivingNodes);
	
	// We are now ready to output to the text file, and the "Main" intermediate binary data file.
	for ( X = 1; X <= NumberOfLivingNodes ; X++ ){
		CurrentNumberOfChildren = ArrayDnodeNumberOfChildrenPlusString(Result->DawgArray, X, CurrentChildLetterString);
		
		// Get the correct offset index to store into the current node
		for ( Y = 0; Y < NumberOfUniqueChildStrings; Y++ ) {
			if ( strcmp(CurrentChildLetterString, HolderOfUniqueChildStrings[Y]) == 0 ) {
				CurrentOffsetNumberIndex = Y;
				break;
			}
		}
		
		CompleteThirtyTwoBitNode = CurrentOffsetNumberIndex;
		CompleteThirtyTwoBitNode <<= BIT_COUNT_FOR_CHILD_INDEX;
		CompleteThirtyTwoBitNode += (Result->DawgArray)[X].Child;
		// The first "BIT_COUNT_FOR_GRAPH_INDEX" are for the first child index.  The EOW_FLAG is stored in the 2^30 bit.
		// The remaining bits will hold a reference to the child list configuration.  No space is used for a letter.
		// The 2's complement sign bit is not needed, so a signed integer is acceptable.
		if ( (Result->DawgArray)[X].EndOfWordFlag == 1 ) CompleteThirtyTwoBitNode += EOW_FLAG;
		fwrite(&CompleteThirtyTwoBitNode, sizeof(int), 1, Main);
		ConvertIntNodeToBinaryString(CompleteThirtyTwoBitNode, TheNodeInBinary);
		ConvertChildListIntToBinaryString(ChildListValues[CurrentOffsetNumberIndex], TheChildListInBinary);
		fprintf(Text, "N%6d-%s,Raw|%10d|,Lev|%2d|", X, TheNodeInBinary, CompleteThirtyTwoBitNode, (Result->DawgArray)[X].Level);
		fprintf(Text, ",{'%c',%d,%6d", (Result->DawgArray)[X].Letter, (Result->DawgArray)[X].EndOfWordFlag, (Result->DawgArray)[X].Next);
		fprintf(Text, ",%6d},Childs|%2d|-|%26s|", (Result->DawgArray)[X].Child , CurrentNumberOfChildren, CurrentChildLetterString);
		fprintf(Text, ",ChildList%s-|%8d|.\r\n", TheChildListInBinary, ChildListValues[CurrentOffsetNumberIndex] );
		if ( CompleteThirtyTwoBitNode == 0 ) printf("\n  Error in node encoding process.\n");
	}
	fclose(Main);
	
	fwrite(&NumberOfUniqueChildStrings, sizeof(int), 1, Secondary);
	fwrite(ChildListValues, sizeof(int), NumberOfUniqueChildStrings, Secondary);
	fclose(Secondary);
	
	fprintf(Text, "\r\nNumber Of Living Nodes |%d| Plus The NULL Node.  Also, there are %d child list ints.\r\n\r\n"
	, NumberOfLivingNodes, NumberOfUniqueChildStrings);
	
	for ( X = 0; X < NumberOfUniqueChildStrings; X++ ) {
		fprintf(Text, "#%4d - |%26s| - |%8d|\r\n", X, HolderOfUniqueChildStrings[X], ChildListValues[X] );
	}
	
	// free all of the memory used to compile the Child-List-Format array.
	for ( X = 0; X < NumberOfLivingNodes; X++ ) {
		free(HolderOfUniqueChildStrings[X]);
	}
	free(HolderOfUniqueChildStrings);
	free(ChildListValues);
	
	printf("\nStep 16 - Create an array with all End-Of-List index values.\n");
	
	for ( X = 1; X <= NumberOfLivingNodes; X++ ) {
		if ( (Result->DawgArray)[X].Next == 0 ){
			EndOfListCount += 1;
		}
	}
	EndOfListIndicies = (int*)malloc(EndOfListCount*sizeof(int));
	fwrite(&EndOfListCount, sizeof(int), 1, ListE);
	
	for ( X = 1; X <= NumberOfLivingNodes; X++ ) {
		if ( (Result->DawgArray)[X].Next == 0 ) {
			EndOfListIndicies[EOLTracker] = X;
			EOLTracker += 1;
		}
	}	
	
	printf("\n  EndOfListCount = |%d|\n", EndOfListCount);
	
	fwrite(EndOfListIndicies, sizeof(int), EndOfListCount, ListE);
	
	fclose(ListE);
	
	fprintf(Text, "\nEndOfListCount |%d|\r\n\r\n", EndOfListCount);
	
	for ( X = 0; X < EndOfListCount; X++ ) {
		fprintf(Text, "#%5d - |%d|\r\n", X, EndOfListIndicies[X]);
	}
	
	fclose(Text);
	
	printf("\nStep 17 - Creation of Traditional-DAWG-Encoding file, Intermediate-Array files, and text-inspection-file complete.\n");

	return Result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int TraverseTheDawgArrayRecurse(int *TheDawg, int *ListFormats, int *OnIt, int CurrentIndex, char *TheWordSoFar
	, int FillThisPosition, char CurrentLetter, int *WordCounter, Bool PrintMe){
	int X;
	int ChildListFormat;
	int CurrentChild;
	int WhatsBelowMe = 0;
	int CorrectOffset;
	TheWordSoFar[FillThisPosition] = CurrentLetter;
	if ( TheDawg[CurrentIndex] & EOW_FLAG ) {
		*WordCounter += 1;
		TheWordSoFar[FillThisPosition+ 1] = '\0';
		if ( PrintMe ) printf("#|%d| - |%s|\n", *WordCounter, TheWordSoFar);
		WhatsBelowMe += 1;
	}
	if ( CurrentChild = (TheDawg[CurrentIndex] & CHILD_MASK) ) {
		ChildListFormat = ListFormats[(TheDawg[CurrentIndex] & LIST_FORMAT_INDEX_MASK) >> LIST_FORMAT_BIT_SHIFT];
		for ( X = 0; X < NUMBER_OF_ENGLISH_LETTERS ; X++ ) {
			// Verify if the X'th letter exists in the Child-List.
			if ( ChildListFormat & PowersOfTwo[X] ) {
				// Because the X'th letter exists, run "ListFormatPopCount", to extract the "CorrectOffset".
				CorrectOffset = ListFormatPopCount(ChildListFormat, X);
				CorrectOffset -= 1;
				WhatsBelowMe += TraverseTheDawgArrayRecurse(TheDawg, ListFormats, OnIt, CurrentChild + CorrectOffset, TheWordSoFar
				, FillThisPosition + 1, X + 'A', WordCounter, PrintMe);
			}
		}
	}
	// Because CWG is a compressed graph, many values of the "OnIt" array will be updated multiple times with the same values.
	OnIt[CurrentIndex] = WhatsBelowMe;
	return WhatsBelowMe;
}

void TraverseTheDawgArray(int *TheDawg, int *TheListFormats, int *BelowingMe, Bool PrintToScreen){
	int X;
	int TheCounter = 0;
	char RunningWord[MAX + 1];
	for ( X = 0; X < NUMBER_OF_ENGLISH_LETTERS ; X++ ) {
		TraverseTheDawgArrayRecurse(TheDawg, TheListFormats, BelowingMe, X + 1, RunningWord, 0, 'A' + X, &TheCounter, PrintToScreen);
	}
}

int main(){
	printf("\n  The 29-Step CWG-Creation-Process has commenced: (Hang in there, it will be over soon.)\n");
	int X;
	int Y;
	// All of the words of similar length will be stored sequentially in the same array so that there will be (MAX + 1)  arrays in total.
	// The Smallest length of a string is assumed to be 2.
	char *AllWordsInEnglish[MAX + 1];
	for ( X = 0; X < (MAX + 1); X++ ) AllWordsInEnglish[X] = NULL;
	
	FILE *Input;
	Input = fopen(RAW_LEXICON,"r");
	char ThisLine[100] = "\0";
	int FirstLineIsSize;
	int LineLength;
	
	fgets(ThisLine, 100, Input);
	CutOffExtraChars(ThisLine);
	FirstLineIsSize = StringToPositiveInt(ThisLine);
	
	printf("\n  FirstLineIsSize = Number-Of-Words = |%d|\n", FirstLineIsSize);
	int DictionarySizeIndex[MAX + 1];
	for ( X = 0; X <= MAX; X++ ) DictionarySizeIndex[X] = 0;
	char **LexiconInRam = (char**)malloc(FirstLineIsSize*sizeof(char *)); 
	
	// The first line is the Number-Of-Words, so read them all into RAM, temporarily.
	for ( X = 0; X < FirstLineIsSize; X++ ) {
		fgets(ThisLine, 100, Input);
		CutOffExtraChars(ThisLine);
		LineLength = strlen(ThisLine);
		if ( LineLength <= MAX ) DictionarySizeIndex[LineLength] += 1;
		LexiconInRam[X] = (char *)malloc((LineLength + 1)*sizeof(char));
		strcpy(LexiconInRam[X], ThisLine);
	}
	printf("\n  Word-List.txt is now in RAM.\n");
	// Allocate enough space to hold all of the words in strings so that we can add them to the trie by length.
	for ( X = 2; X < (MAX + 1); X++ ) AllWordsInEnglish[X] = (char *)malloc((X + 1)*DictionarySizeIndex[X]*sizeof(char));
	
	int CurrentTracker[MAX + 1];
	for ( X = 0; X < (MAX + 1); X++ ) CurrentTracker[X] = 0;
	int CurrentLength;
	// Copy all of the strings into the halfway house 1.
	for ( X = 0; X < FirstLineIsSize; X++ ) {
		CurrentLength = strlen(LexiconInRam[X]);
		// Simply copy a string from its temporary ram location to the array of length equivelant strings for processing in making the DAWG.
		if ( CurrentLength <= MAX ) strcpy( &((AllWordsInEnglish[CurrentLength])[(CurrentTracker[CurrentLength]*(CurrentLength + 1))]), LexiconInRam[X] );
		CurrentTracker[CurrentLength] += 1;
	}
	printf("\n  The words are now stored in an array according to length.\n\n");
	// Make sure that the counting has resulted in all of the strings being placed correctly.
	for ( X = 0; X < (MAX + 1); X++ ) {
		if ( DictionarySizeIndex[X] == CurrentTracker[X] ) printf("  |%2d| Letter word count = |%5d| is verified.\n", X, CurrentTracker[X]);
		else printf("  Something went wrong with |%2d| letter words.\n", X);
	}
	
	// Free the the initial dynamically allocated memory.
	for ( X = 0; X < FirstLineIsSize; X++ ) free(LexiconInRam[X]);
	free(LexiconInRam);
	
	printf("\n  Begin Creator init function.\n\n");
	
	ArrayDawgPtr Adoggy = ArrayDawgInit(AllWordsInEnglish, DictionarySizeIndex, MAX);
	
	//-----------------------------------------------------------------------------------
	// Begin tabulation of "NumberOfWordsToEndOfBranchList" array.
	FILE *PartOne = fopen(DIRECT_GRAPH_DATA_PART_ONE, "rb");
	FILE *PartTwo = fopen(DIRECT_GRAPH_DATA_PART_TWO, "rb");
	FILE *ListE = fopen(FINAL_NODES_DATA, "rb");
	int NumberOfPartOneNodes;
	int NumberOfPartTwoNodes;
	int NumberOfFinalNodes;
	int CurrentFinalNodeIndex;
	int CurrentCount;
	
	fread(&NumberOfPartOneNodes, sizeof(int), 1, PartOne);
	fread(&NumberOfPartTwoNodes, sizeof(int), 1, PartTwo);
	fread(&NumberOfFinalNodes, sizeof(int), 1, ListE);
	int *PartOneArray = (int *)malloc((NumberOfPartOneNodes + 1)*sizeof(int));
	int *PartTwoArray = (int *)malloc(NumberOfPartTwoNodes*sizeof(int));
	int *FinalNodeLocations = (int *)malloc(NumberOfFinalNodes*sizeof(int));
	
	fread(PartOneArray + 1, sizeof(int), NumberOfPartOneNodes, PartOne);
	fread(PartTwoArray, sizeof(int), NumberOfPartTwoNodes, PartTwo);
	fread(FinalNodeLocations, sizeof(int), NumberOfFinalNodes, ListE);
	
	int *NumberOfWordsBelowMe = (int *)malloc((NumberOfPartOneNodes + 1)*sizeof(int));
	int *NumberOfWordsToEndOfBranchList =(int *)malloc((NumberOfPartOneNodes + 1)*sizeof(int));
	int *RearrangedNumberOfWordsToEndOfBranchList =(int *)malloc((NumberOfPartOneNodes + 1)*sizeof(int));
	
	NumberOfWordsBelowMe[0] = 0;
	NumberOfWordsToEndOfBranchList[0] = 0;
	RearrangedNumberOfWordsToEndOfBranchList[0] = 0;
	PartOneArray[0] = 0;
	
	fclose(PartOne);
	fclose(PartTwo);
	fclose(ListE);
	
	printf("\nStep 18 - Display the Mask-Format for CWG Main-Nodes:\n\n");
	
	char Something[38];
	ConvertIntNodeToBinaryString(CHILD_MASK, Something);
	printf("  %s - CHILD_MASK\n", Something);
	
	ConvertIntNodeToBinaryString(LIST_FORMAT_INDEX_MASK, Something);
	printf("  %s - LIST_FORMAT_INDEX_MASK\n", Something);
	
	printf("\nStep 19 - Traverse the DawgArray to fill the NumberOfWordsBelowMe array.\n");
	
	// This function is run to fill the "NumberOfWordsBelowMe" array.
	TraverseTheDawgArray(PartOneArray, PartTwoArray, NumberOfWordsBelowMe, FALSE);
	
	printf("\nStep 20 - Use FinalNodeLocations and NumberOfWordsBelowMe to fill the NumberOfWordsToEndOfBranchList array.\n");
	
	// This little piece of code compiles the "NumberOfWordsToEndOfBranchList" array.
	// The requirements are the "NumberOfWordsBelowMe" array and the "FinalNodeLocations" array.
	CurrentFinalNodeIndex = 0;
	for ( X = 1; X <= NumberOfPartOneNodes; X++ ) {
		CurrentCount = 0;
		for ( Y = X; Y <= FinalNodeLocations[CurrentFinalNodeIndex]; Y++ ) {
			CurrentCount += NumberOfWordsBelowMe[Y];
		}
		NumberOfWordsToEndOfBranchList[X] = CurrentCount;
		if ( X ==  FinalNodeLocations[CurrentFinalNodeIndex] ) CurrentFinalNodeIndex +=1;
	}
	
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Now with preliminary analysis complete, it is time to rearrange the PartOne nodes and then set up PartThree.
	
	int ListSizeCounter[NUMBER_OF_ENGLISH_LETTERS + 1];
	int TotalNumberOfLists = 0;
	Bool AreWeInBigList = FALSE;
	int TheCurrentChild;
	int StartOfCurrentList = 1;
	int SizeOfCurrentList = FinalNodeLocations[0];
	int EndOfCurrentList = FinalNodeLocations[0];
	int InsertionPoint = 1;
	int CurrentlyCopyingThisList = 0;
	int *PartOneRearrangedArray = (int *)malloc((NumberOfPartOneNodes + 1)*sizeof(int));
	int *CurrentAdjustments = (int *)malloc((NumberOfPartOneNodes + 1)*sizeof(int));
	
	PartOneRearrangedArray[0] = 0;
	for ( X = 0; X <= NumberOfPartOneNodes; X++ ) CurrentAdjustments[X] = 0;
	
	for ( X = 0; X <= NUMBER_OF_ENGLISH_LETTERS; X++ ) ListSizeCounter[X] = 0;
	
	printf("\nStep 21 - Relocate all node-lists with WTEOBL values greater than 255, to the front of the Main CWG array.\n");
	
	printf("\n  All corresponding node data and End-Of-List data must also be shifted around.\n");
	
	// This code is responsible for rearranging the node lists inside of the CWG int array so the word-heavy lists filter to the front.
	CurrentFinalNodeIndex = 0;
	for ( X = 1; X <= NumberOfPartOneNodes; X++ ) {
		if ( NumberOfWordsToEndOfBranchList[X] > 255 ) AreWeInBigList = TRUE;
		if ( X ==  EndOfCurrentList ) {
			ListSizeCounter[SizeOfCurrentList] += 1;
			// We are now at the end of a big list that must to be moved up to the InsertionPoint.
			// This also implies moving everything between its current location and its new one.
			if ( AreWeInBigList == TRUE ) {
				// First step is to copy the CurrentList into the new array at its correct position.
				for ( Y = 0; Y < SizeOfCurrentList; Y++ ) {
					PartOneRearrangedArray[InsertionPoint + Y] = PartOneArray[StartOfCurrentList + Y];
					RearrangedNumberOfWordsToEndOfBranchList[InsertionPoint + Y] = NumberOfWordsToEndOfBranchList[StartOfCurrentList + Y];
				}
				// The following steps are required when we are actually moving the position of a list.  The first set of lists will bypass these steps.
				if ( InsertionPoint !=  StartOfCurrentList ) {
					// Step 2 is to move all of the nodes between the original and final location, "SizeOfCurrentList" number of places back, starting from the end.
					for ( Y = EndOfCurrentList; Y >= (InsertionPoint + SizeOfCurrentList); Y-- ) {
						PartOneArray[Y] = PartOneArray[Y - SizeOfCurrentList];
						NumberOfWordsToEndOfBranchList[Y] = NumberOfWordsToEndOfBranchList[Y - SizeOfCurrentList];
					}
					// Step 3 is to copy the list we are moving up from the rearranged array back into the original.
					for ( Y = InsertionPoint; Y < (InsertionPoint + SizeOfCurrentList); Y++ ) {
						PartOneArray[Y] = PartOneRearrangedArray[Y];
						NumberOfWordsToEndOfBranchList[Y] = RearrangedNumberOfWordsToEndOfBranchList[Y];
					}
					// Step 4 is to fill the "CurrentAdjustments" array with the amount that each child references must be moved.
					// The two arrays are identical now up to the new insertion point.
					// At this stage, the "CurrentAdjustments" array is all zeros.
					for ( Y = 1; Y <= NumberOfPartOneNodes; Y++ ) {
						TheCurrentChild = (PartOneArray[Y] & CHILD_MASK);
						if ( (TheCurrentChild >= InsertionPoint) && (TheCurrentChild < StartOfCurrentList) ) CurrentAdjustments[Y] = SizeOfCurrentList;
						if ( (TheCurrentChild >= StartOfCurrentList) && (TheCurrentChild <= EndOfCurrentList) ) CurrentAdjustments[Y] = InsertionPoint - StartOfCurrentList;
					}
					// Step 5 is to fix all of the child reference values in both of the arrays.
					// Start with the rearranged array.
					for ( Y = 1; Y < (InsertionPoint + SizeOfCurrentList); Y++ ) {
						if ( CurrentAdjustments[Y] != 0 ) PartOneRearrangedArray[Y] += CurrentAdjustments[Y];
					}
					// Finish with the original array.  Make sure to zero all the values after the adjustments have been made to get ready for the next round.
					for ( Y = 1; Y <= NumberOfPartOneNodes; Y++ ) {
						if ( CurrentAdjustments[Y] != 0 ) {
							PartOneArray[Y] += CurrentAdjustments[Y];
							CurrentAdjustments[Y] = 0;
						}
					}
				}
				// Step 7 is to set the new InsertionPoint and change the "FinalNodeLocations", so that they reflect the shift.
				InsertionPoint += SizeOfCurrentList;
				// Shift all of the end of lists between the "CurrentlyCopyingThisList" and "CurrentFinalNodeIndex".
				for ( Y = CurrentFinalNodeIndex; Y > CurrentlyCopyingThisList; Y-- ) {
					FinalNodeLocations[Y] = FinalNodeLocations[Y - 1] + SizeOfCurrentList;
				}
				FinalNodeLocations[CurrentlyCopyingThisList] = InsertionPoint - 1;
				CurrentlyCopyingThisList += 1;
				
			}
			// Even when we are not in a big list, we still need to update the current list parameters.
			CurrentFinalNodeIndex += 1;
			SizeOfCurrentList = FinalNodeLocations[CurrentFinalNodeIndex] - EndOfCurrentList;
			EndOfCurrentList = FinalNodeLocations[CurrentFinalNodeIndex];
			StartOfCurrentList = X + 1;
			AreWeInBigList = FALSE;
		}
	}
	
	printf("\n  Word-Heavy list-shifting is now complete.\n");
	
	// Step 8 is to copy all of the small lists from the original array to the rearranged array.  All of the references should be properly adjusted at this point.
	for ( X = InsertionPoint; X <= NumberOfPartOneNodes; X++  ) {
		PartOneRearrangedArray[X] = PartOneArray[X];
		RearrangedNumberOfWordsToEndOfBranchList[X] = NumberOfWordsToEndOfBranchList[X];
	}
	
	// Rearrangement of the DAWG lists to reduce size of the PartThree data file is complete, so check if the new and old lists are identical, because they should be.
	for ( X = 1; X <= NumberOfPartOneNodes; X++  ) {
		if ( PartOneArray[X] != PartOneRearrangedArray[X] ) printf("  What A Mistake!\n");
		if ( RearrangedNumberOfWordsToEndOfBranchList[X] != NumberOfWordsToEndOfBranchList[X] ) printf("  Mistaken.\n");
	}
	
	// The two arrays are now identical, so as a final precaution, traverse the rearranged array.
	TraverseTheDawgArray(PartOneRearrangedArray, PartTwoArray, NumberOfWordsBelowMe, FALSE);
	
	// Check for duplicate lists.  It is now highly likely that there are some duplicates.
	// Lists of size X, can be replaced with partial lists of size X+n.  Make sure to check for this case.
	
	printf("\nStep 22 - Create an array to organize End-Of-List values by size.\n\n");
	
	// Add up the total number of lists.
	for ( X = 1; X <= NUMBER_OF_ENGLISH_LETTERS; X++ ) {
		TotalNumberOfLists += ListSizeCounter[X];
		printf("  Size|%2d| Lists = |%5d|\n", X, ListSizeCounter[X]);
	}
	printf("\n  TotalNumberOfLists = |%d|\n", TotalNumberOfLists);
	
	int **NodeListsBySize = (int **)malloc((NUMBER_OF_ENGLISH_LETTERS + 1)*sizeof(int *));
	int WhereWeAt[NUMBER_OF_ENGLISH_LETTERS + 1];
	for ( X = 0; X <= NUMBER_OF_ENGLISH_LETTERS; X++ ) WhereWeAt[X] = 0;
	
	for ( X = 1; X <= NUMBER_OF_ENGLISH_LETTERS; X++ ) {
		NodeListsBySize[X] = (int *)malloc(ListSizeCounter[X]*sizeof(int));
	}
	
	// We are now required to fill the "NodeListsBySize" array.  Simply copy over the correct "FinalNode" information.  
	// Note that the "FinalNode" information reflects the readjustment that just took place.
	
	CurrentFinalNodeIndex = 0;
	EndOfCurrentList = FinalNodeLocations[0];
	SizeOfCurrentList = FinalNodeLocations[0];
	for ( X = 0; X < NumberOfFinalNodes; X++ ) {
		(NodeListsBySize[SizeOfCurrentList])[WhereWeAt[SizeOfCurrentList]] = EndOfCurrentList;
		WhereWeAt[SizeOfCurrentList] += 1;
		CurrentFinalNodeIndex += 1;
		SizeOfCurrentList = FinalNodeLocations[CurrentFinalNodeIndex] - EndOfCurrentList;
		EndOfCurrentList = FinalNodeLocations[CurrentFinalNodeIndex];
	}
	
	printf("\n  End-Of-List values are now organized.\n");
	
	int Z;
	int V;
	int W;
	int U;
	int TheNewChild;
	int TotalNumberOfKilledLists = 0;
	int TotalNumberOfKilledNodes = 0;
	int NewNumberOfKilledLists = -1;
	int InspectThisEndOfList;
	int MaybeReplaceWithThisEndOfList;
	int NumberOfListsKilledThisRound = -1;
	int CurrentNumberOfPartOneNodes = NumberOfPartOneNodes;
	Bool EliminateCurrentList = TRUE;
	
	printf("\nStep 23 - Kill more lists by using the ends of longer lists or lists of equal size.\n\n");
	
	// Try to eliminate lists with partial lists.
	// "X" is the list-length of lists we are trying to kill.
	//for ( X = 1; X < NUMBER_OF_ENGLISH_LETTERS; X++ ) {
	for ( X = NUMBER_OF_ENGLISH_LETTERS; X >= 1; X-- ) {
		printf("  Try To Eliminate Lists of Size |%2d| - ", X);
		NewNumberOfKilledLists = 0;
		// Look for partial lists at the end of "Y" sized lists, to replace the "X" sized lists with.
		for ( Y = NUMBER_OF_ENGLISH_LETTERS; Y >= X; Y-- ) {
			// Try to kill list # "Z".
			for ( Z = 0; Z < ListSizeCounter[X]; Z++ ) {
				InspectThisEndOfList = (NodeListsBySize[X])[Z];
				// Try to replace with list # "Z".
				for ( W = 0; W < ListSizeCounter[Y]; W++ ) {
					// Never try to replace a list with itself.
					if ( (X == Y) && (Z == W) ) continue;
					MaybeReplaceWithThisEndOfList = (NodeListsBySize[Y])[W];
					for( V = 0; V < X; V++ ) {
						if ( PartOneArray[InspectThisEndOfList - V] != PartOneArray[MaybeReplaceWithThisEndOfList - V] ) {
							EliminateCurrentList = FALSE;
							break;
						}
					}
					// When eliminating a list, make sure to adjust the WTEOBL data.
					if ( EliminateCurrentList == TRUE ) {
						// Step 1 - Replace all references to the duplicate list with the earlier equivalent.
						for( V = 1; V <= CurrentNumberOfPartOneNodes; V++ ) {
							TheCurrentChild = (PartOneArray[V] & CHILD_MASK);
							if ( (TheCurrentChild > (InspectThisEndOfList - X)) && (TheCurrentChild <= InspectThisEndOfList) ) {
								TheNewChild = MaybeReplaceWithThisEndOfList - (InspectThisEndOfList - TheCurrentChild);
								PartOneArray[V] -= TheCurrentChild;
								PartOneArray[V] += TheNewChild;
							}
						}
						// Step 2 - Eliminate the dupe list by moving the higher lists forward.
						for ( V = (InspectThisEndOfList - X + 1); V <= (CurrentNumberOfPartOneNodes - X); V++ ) {
							PartOneArray[V] = PartOneArray[V + X];
							NumberOfWordsToEndOfBranchList[V] = NumberOfWordsToEndOfBranchList[V + X];
						}
						// Step 3 - Change CurrentNumberOfPartOneNodes.
						CurrentNumberOfPartOneNodes -= X;
						// Step 4 - Lower all references to the nodes coming after the dupe list.
						for ( V = 1; V <= CurrentNumberOfPartOneNodes; V++ ) {
							TheCurrentChild = (PartOneArray[V] & CHILD_MASK);
							if ( TheCurrentChild > InspectThisEndOfList ){
								PartOneArray[V] -= X;
							}
						}
						// Step 5 - Readjust all of the lists after "Z" forward 1 and down X to the value, and lower ListSizeCounter[X] by 1.
						for( V = Z; V < (ListSizeCounter[X] - 1); V++ ) {
							(NodeListsBySize[X])[V] = (NodeListsBySize[X])[V + 1] - X;
						}
						ListSizeCounter[X] -= 1;
						// Step 6 - Lower any list, of any size, greater than (NodeListsBySize[X])[Z], down by X.
						for ( V = 1; V <= (X - 1); V++ ) {
							for ( U = 0; U < ListSizeCounter[V]; U++ ) {
								if ( (NodeListsBySize[V])[U] > InspectThisEndOfList ) (NodeListsBySize[V])[U] -= X;
							}
						}
						for ( V = (X + 1); V <= NUMBER_OF_ENGLISH_LETTERS; V++ ) {
							for ( U = 0; U < ListSizeCounter[V]; U++ ) {
								if ( (NodeListsBySize[V])[U] > InspectThisEndOfList ) (NodeListsBySize[V])[U] -= X;
							}
						}
						// Step 7 - Lower "Z" by 1 and increase "TotalNumberOfKilledLists".
						Z -= 1;
						TotalNumberOfKilledLists += 1;
						NewNumberOfKilledLists += 1;
						TotalNumberOfKilledNodes += X;
						break;
					}
					EliminateCurrentList = TRUE;
				}
			}
		}
		if ( NewNumberOfKilledLists > 0 ) printf("Killed |%d| lists.\n", NewNumberOfKilledLists);
		else printf("Empty handed.\n");
		NewNumberOfKilledLists = 0;
	}
	
	printf("\n  Removal of the new-redundant-lists is now complete:\n");
	printf("\n  |%5d| = Original # of lists.\n", TotalNumberOfLists);
	printf("  |%5d| = Killed # of lists.\n", TotalNumberOfKilledLists);
	printf("  |%5d| = Remaining # of lists.\n", TotalNumberOfLists = TotalNumberOfLists - TotalNumberOfKilledLists);

	printf("\n  |%6d| = Original # of nodes.\n", NumberOfPartOneNodes);	
	printf("  |%6d| = Killed # of nodes.\n", TotalNumberOfKilledNodes);
	printf("  |%6d| = Remaining # of nodes.\n", CurrentNumberOfPartOneNodes);
	
	// Try to eliminate lists with partial lists again to check that we've got em all.
	printf("\nStep 24 - Run the redundant-list-analysis one more time to test that no-more exist.\n\n");
	NumberOfPartOneNodes = CurrentNumberOfPartOneNodes;
	TotalNumberOfKilledLists = 0;
	TotalNumberOfKilledNodes = 0;
	EliminateCurrentList = TRUE;
	// "X" is the list-length of lists we are trying to kill.
	for ( X = NUMBER_OF_ENGLISH_LETTERS; X >= 1; X-- ) {
		printf("  Try To Eliminate Lists of Size |%2d| - ", X);
		NewNumberOfKilledLists = 0;
		// Look for partial lists at the end of "Y" sized lists, to replace the "X" sized lists with.
		for ( Y = NUMBER_OF_ENGLISH_LETTERS; Y >= X; Y-- ) {
			// Try to kill list # "Z".
			for ( Z = 0; Z < ListSizeCounter[X]; Z++ ) {
				InspectThisEndOfList = (NodeListsBySize[X])[Z];
				// Try to replace with list # "Z".
				for ( W = 0; W < ListSizeCounter[Y]; W++ ) {
					// Never try to replace a list with itself.
					if ( (X == Y) && (Z == W) ) continue;
					MaybeReplaceWithThisEndOfList = (NodeListsBySize[Y])[W];
					for( V = 0; V < X; V++ ) {
						if ( PartOneArray[InspectThisEndOfList - V] != PartOneArray[MaybeReplaceWithThisEndOfList - V] ) {
							EliminateCurrentList = FALSE;
							break;
						}
					}
					// When eliminating a list, make sure to adjust the WTEOBL data.
					if ( EliminateCurrentList == TRUE ) {
						// Step 1 - Replace all references to the duplicate list with the earlier equivalent.
						for( V = 1; V <= CurrentNumberOfPartOneNodes; V++ ) {
							TheCurrentChild = (PartOneArray[V] & CHILD_MASK);
							if ( (TheCurrentChild > (InspectThisEndOfList - X)) && (TheCurrentChild <= InspectThisEndOfList) ) {
								TheNewChild = MaybeReplaceWithThisEndOfList - (InspectThisEndOfList - TheCurrentChild);
								PartOneArray[V] -= TheCurrentChild;
								PartOneArray[V] += TheNewChild;
							}
						}
						// Step 2 - Eliminate the dupe list by moving the higher lists forward.
						for ( V = (InspectThisEndOfList - X + 1); V <= (CurrentNumberOfPartOneNodes - X); V++ ) {
							PartOneArray[V] = PartOneArray[V + X];
							NumberOfWordsToEndOfBranchList[V] = NumberOfWordsToEndOfBranchList[V + X];
						}
						// Step 3 - Change CurrentNumberOfPartOneNodes.
						CurrentNumberOfPartOneNodes -= X;
						// Step 4 - Lower all references to the nodes coming after the dupe list.
						for ( V = 1; V <= CurrentNumberOfPartOneNodes; V++ ) {
							TheCurrentChild = (PartOneArray[V] & CHILD_MASK);
							if ( TheCurrentChild > InspectThisEndOfList ){
								PartOneArray[V] -= X;
							}
						}
						// Step 5 - Readjust all of the lists after "Z" forward 1 and down X to the value, and lower ListSizeCounter[X] by 1.
						for( V = Z; V < (ListSizeCounter[X] - 1); V++ ) {
							(NodeListsBySize[X])[V] = (NodeListsBySize[X])[V + 1] - X;
						}
						ListSizeCounter[X] -= 1;
						// Step 6 - Lower any list, of any size, greater than (NodeListsBySize[X])[Z], down by X.
						for ( V = 1; V <= (X - 1); V++ ) {
							for ( U = 0; U < ListSizeCounter[V]; U++ ) {
								if ( (NodeListsBySize[V])[U] > InspectThisEndOfList ) (NodeListsBySize[V])[U] -= X;
							}
						}
						for ( V = (X + 1); V <= NUMBER_OF_ENGLISH_LETTERS; V++ ) {
							for ( U = 0; U < ListSizeCounter[V]; U++ ) {
								if ( (NodeListsBySize[V])[U] > InspectThisEndOfList ) (NodeListsBySize[V])[U] -= X;
							}
						}
						// Step 7 - Lower "Z" by 1 and increase "TotalNumberOfKilledLists".
						Z -= 1;
						TotalNumberOfKilledLists += 1;
						NewNumberOfKilledLists += 1;
						TotalNumberOfKilledNodes += X;
						break;
					}
					EliminateCurrentList = TRUE;
				}
			}
		}
		if ( NewNumberOfKilledLists > 0 ) printf("Killed |%d| lists.\n", NewNumberOfKilledLists);
		else printf("Empty handed.\n");
		NewNumberOfKilledLists = 0;
	}
	
	printf("\n  The no-more redundant-list-test is now complete:\n");
	printf("\n  |%5d| = Original # of lists.\n", TotalNumberOfLists);
	printf("  |%5d| = Killed # of lists.\n", TotalNumberOfKilledLists);
	printf("  |%5d| = Remaining # of lists.\n", TotalNumberOfLists - TotalNumberOfKilledLists);

	printf("\n  |%6d| = Original # of nodes.\n", NumberOfPartOneNodes);	
	printf("  |%6d| = Killed # of nodes.\n", TotalNumberOfKilledNodes);
	printf("  |%6d| = Remaining # of nodes.\n", CurrentNumberOfPartOneNodes);
	
	// verify that the reduction procedures have resulted in a valid word graph. 
	
	// "FinalNodeLocations" needs to be recompiled from what is left in the "NodeListsBySize" arrays.
	
	printf("\nStep 25 - Recompile the FinalNodeLocations array and display the distribution.\n\n");
	
	TotalNumberOfLists = 0;
	for ( X = 1; X <= NUMBER_OF_ENGLISH_LETTERS; X++ ) {
		TotalNumberOfLists += ListSizeCounter[X];
		printf("  List Size|%2d| - Number Of Lists|%5d|\n", X, ListSizeCounter[X]);
	}
	printf("\n  TotalNumberOfLists|%d|\n", TotalNumberOfLists);
	
	// Set all initial values in "FinalNodeLocations" array to BOGUS numbers.
	for ( X = 0; X < TotalNumberOfLists; X++ ) {
		FinalNodeLocations[X] = 1000000;
	}
	
	// Filter all of the living "FinalNode" values into the "FinalNodeLocations" array.
	
	int TempValue;
	
	for ( X = NUMBER_OF_ENGLISH_LETTERS; X >= 1; X-- ) {
		for ( Y = 0; Y < ListSizeCounter[X]; Y++ ) {
			FinalNodeLocations[TotalNumberOfLists - 1] = (NodeListsBySize[X])[Y];
			// The new list has been placed at the end of the "FinalNodeLocations" array, now filter it up to where it should be.
			for ( Z = (TotalNumberOfLists - 1); Z > 0; Z-- ) {
				if ( FinalNodeLocations[Z - 1] > FinalNodeLocations[Z] ) {
					TempValue = FinalNodeLocations[Z - 1];
					FinalNodeLocations[Z - 1] = FinalNodeLocations[Z];
					FinalNodeLocations[Z] = TempValue;
				}
				else break;
			}
		}
	}
	
	
	// Test for logical errors in the list elimination procedure.
	for ( X = 0; X < (TotalNumberOfLists - 1); X++ ) {
		if ( FinalNodeLocations[X] == FinalNodeLocations[X + 1] ) printf("\nNo Two Lists Can End On The Same Node. |%d|%d|\n", X, FinalNodeLocations[X]);
	}
	
	printf("\n  The FinalNodeLocations array is now compiled and tested for obvious errors.\n");
	
	printf("\nStep 26 - Recompile WTEOBL array by graph traversal, and test equivalence with the one modified during list-killing.\n");
	
	// Compile "RearrangedNumberOfWordsToEndOfBranchList", and verify that it is the same as "NumberOfWordsToEndOfBranchList".
	TraverseTheDawgArray(PartOneArray, PartTwoArray, NumberOfWordsBelowMe, FALSE);
	
	// This little piece of code compiles the "RearrangedNumberOfWordsToEndOfBranchList" array.
	// The requirements are the "NumberOfWordsBelowMe" array and the "FinalNodeLocations" array.
	CurrentFinalNodeIndex = 0;
	for ( X = 1; X <= CurrentNumberOfPartOneNodes; X++ ) {
		CurrentCount = 0;
		for ( Y = X; Y <= FinalNodeLocations[CurrentFinalNodeIndex]; Y++ ) {
			CurrentCount += NumberOfWordsBelowMe[Y];
		}
		RearrangedNumberOfWordsToEndOfBranchList[X] = CurrentCount;
		if ( X ==  FinalNodeLocations[CurrentFinalNodeIndex] ) CurrentFinalNodeIndex +=1;
	}
	
	printf("\n  New WTEOBL array is compiled, so test for equality.\n");
	
	for ( X = 1; X <= CurrentNumberOfPartOneNodes; X++ ) {
		if ( RearrangedNumberOfWordsToEndOfBranchList[X] != NumberOfWordsToEndOfBranchList[X] ) printf("\nMismatch found.\n");
	}
	
	printf("\n  Equality test complete.\n");
	
	printf("\nStep 27 - Determine the final node index that requires a short integer for its WTEOBL value.\n");
	
	// Find out the final index number that requires an integer greater in size than a byte to hold it. Part 3 of the data structure will be held in three arrays.
	int FurthestBigNode = 0;
	int FirstSmallNode;
	for ( X = 1; X <= CurrentNumberOfPartOneNodes; X++ ) {
		if ( RearrangedNumberOfWordsToEndOfBranchList[X] > 0XFF ) FurthestBigNode = X;
	}
	
	for ( X = 0; X < TotalNumberOfLists; X++ ) {
		if ( FinalNodeLocations[X] >= FurthestBigNode ) {
			printf("\n  End of final short-integer WTEOBL list = |%d|.\n", FinalNodeLocations[X]);
			FurthestBigNode = FinalNodeLocations[X];
			break;
		}
	}
	
	FirstSmallNode = FurthestBigNode + 1;
	
	printf("\n  Index of first node requiring only an unsigned-char for its WTEOBL = |%d|.\n", FirstSmallNode);
	
	// The first 26 nodes are the only ones in need of 4-Byte int variables to hold their WTEOBL values.
	// Being the entry points for the graph, it makes sense to hold these values in a "const int" array, defined in the program code.
	
	// The "short int" array holding the medium size WTEOBL values will hold '0's for elements [0, 26], inclusive.
	// The "unsigned char" array must be unsigned because many of the values require 8-bit representation.
	
	// The entire CWG will be stored inside of the one "CWG_Data_For_Word-List.dat" data file.
	// The first integer will be the total number of words in the graph.
	// The next five integers will be the array sizes.
	// After these header values, each array will then be written to the file in order, using the correct integer type.
	
	printf("\nStep 28 - Separate the final 3 WTEOBL arrays, and write all 5 arrays to the FinalProduct CWG data file.\n");
	
	int ArrayOneSize = CurrentNumberOfPartOneNodes + 1;
	int ArrayTwoSize = NumberOfPartTwoNodes;
	int ArrayThreeSize = NUMBER_OF_ENGLISH_LETTERS + 1;
	int ArrayFourSize = FurthestBigNode + 1;
	int ArrayFiveSize = CurrentNumberOfPartOneNodes - FurthestBigNode;
	
	// Allocate the final three arrays.
	int *PartThreeArray = (int *)malloc(ArrayThreeSize*sizeof(int));
	short int *PartFourArray = (short int *)malloc(ArrayFourSize*sizeof(short int));
	unsigned char *PartFiveArray = (unsigned char *)malloc(ArrayFiveSize*sizeof(unsigned char));
	
	// Fill the final three CWG arrays.
	for ( X = 0; X < ArrayThreeSize; X++ ) {
		PartThreeArray[X] = RearrangedNumberOfWordsToEndOfBranchList[X];
		PartFourArray[X] = 0;
	}
	for ( X = ArrayThreeSize; X < ArrayFourSize; X++ ) PartFourArray[X] = RearrangedNumberOfWordsToEndOfBranchList[X];
	for ( X = 0; X < ArrayFiveSize; X++ ) PartFiveArray[X] = RearrangedNumberOfWordsToEndOfBranchList[ArrayFourSize + X];
	
	FILE* FinalProduct = fopen(CWG_DATA, "wb");
	
	fwrite(&FirstLineIsSize, sizeof(int), 1, FinalProduct);
	fwrite(&ArrayOneSize, sizeof(int), 1, FinalProduct);
	fwrite(&ArrayTwoSize, sizeof(int), 1, FinalProduct);
	fwrite(&ArrayThreeSize, sizeof(int), 1, FinalProduct);
	fwrite(&ArrayFourSize, sizeof(int), 1, FinalProduct);
	fwrite(&ArrayFiveSize, sizeof(int), 1, FinalProduct);
	
	fwrite(PartOneArray, sizeof(int), ArrayOneSize, FinalProduct);
	fwrite(PartTwoArray, sizeof(int), ArrayTwoSize, FinalProduct);
	fwrite(PartThreeArray, sizeof(int), ArrayThreeSize, FinalProduct);
	fwrite(PartFourArray, sizeof(short int), ArrayFourSize, FinalProduct);
	fwrite(PartFiveArray, sizeof(unsigned char), ArrayFiveSize, FinalProduct);
	
	fclose(FinalProduct);
	
	printf("\n  The new CWG is ready to use.  Now, understand it, then go out and use it.\n\n");
	
	return 0;
}
