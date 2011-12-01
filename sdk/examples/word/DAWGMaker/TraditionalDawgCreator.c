// This program will compile a Traditional DAWG encoding from the "Word-List.txt" file.

// 1) "Word-List.txt" is a text file with the number of words written on the very first line, and 1 word per line after that.
// The words are case-insensitive.

// Include the big-three header files.
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// General high-level program constants.
#define MAX 6//15
#define NUMBER_OF_ENGLISH_LETTERS 26
#define INPUT_LIMIT 30
#define LOWER_IT 32
#define TEN 10
#define INT_BITS 32
#define CHILD_BIT_SHIFT 5
#define CHILD_INDEX_BIT_MASK 0X003FFFE0
#define LETTER_BIT_MASK 0X0000001F
#define END_OF_WORD_BIT_MASK 0X00800000
#define END_OF_LIST_BIT_MASK 0X00400000

// C requires a boolean variable type so use C's typedef concept to create one.
typedef enum { FALSE = 0, TRUE = 1 } Bool;
typedef Bool* BoolPtr;

// The lexicon text file.
//#define RAW_LEXICON "Word-List.txt"
#define RAW_LEXICON "dictionary.txt"

// This program will create "5" binary-data files for use, and "1" text-data file for inspection.
//#define TRADITIONAL_DAWG_DATA "Traditional_Dawg_For_Word-List.dat"
//#define TRADITIONAL_DAWG_TEXT_DATA "Traditional_Dawg_Explicit_Text_For_Word-List.txt"
#define TRADITIONAL_DAWG_DATA "dictionary.dat"
#define TRADITIONAL_DAWG_TEXT_DATA "dictionary_stats.txt"

// Lookup tables used for node encoding and number-string decoding.
const unsigned int PowersOfTwo[INT_BITS - 1] = { 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384,
 32768, 65536, 131072, 262144, 524288, 1048576, 2097152, 4194304, 8388608, 16777216, 33554432, 67108864, 134217728,
 268435456, 536870912, 1073741824 };
const unsigned int PowersOfTen[TEN] = { 1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000 };

// This simple function clips off the extra chars for each "fgets()" line.  Works for Linux and Windows text format.
void CutOffExtraChars(char *ThisLine){
	if ( ThisLine[strlen(ThisLine) - 2] == '\r' ) ThisLine[strlen(ThisLine) - 2] = '\0';
	else if ( ThisLine[strlen(ThisLine) - 1] == '\n' ) ThisLine[strlen(ThisLine) - 1] = '\0';
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

// The "BinaryNode" string must be at least 32 + 6 + 1 bytes in length.  Space for the bits,
// the seperation pipes, and the end of string char.
// This function is used to fill the text file used to inspect the graph created in the first segment of the program.
void ConvertIntNodeToBinaryString(int TheNode, char *BinaryNode){
	int X;
	int Bit;
	BinaryNode[0] = '[';
	// Bit 31-to-24 are not being used.  They will always be '0'.
	for ( X = 1; X <= 8; X++ )BinaryNode[X] = '_';
	BinaryNode[9] = '|';
	// Bit 23 holds the End-Of-Word flag.
	BinaryNode[10] = (TheNode & PowersOfTwo[23])?'1':'0';
	BinaryNode[11] = '|';
	// Bit 22 holds the End-Of-List flag.
	BinaryNode[12] = (TheNode & PowersOfTwo[22])?'1':'0';
	BinaryNode[13] = '|';
	// 17 Bits, (21-->5) hold the First-Child index.
	Bit = 21;
	for ( X = 14; X <= 30; X++, Bit-- ) BinaryNode[X] = (TheNode & PowersOfTwo[Bit])?'1':'0';
	BinaryNode[31] = '|';
	// The Letter is held in the final 5 bits, (4->0).
	Bit = 4;
	for ( X = 32; X <= 36; X++, Bit-- ) BinaryNode[X] = (TheNode & PowersOfTwo[Bit])?'1':'0';
	BinaryNode[37] = ']';
	BinaryNode[38] = '\0';
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
	// "ArrayIndex" Is stored in every node, so that we can mine the information from the Trie.
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

// This function removes "Protected" status from "ThisNode".
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

// This function returns the pointer to the Tnode in a parallel list of nodes with the letter "ThisLetter",
// and returns NULL if the Tnode does not exist.
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

// This function inserts a new Tnode before a larger letter node or at the end of a para list.
// If the list does not esist then it is put at the beginnung.  
// The new node has ThisLetter in it.  AboveTnode is the node 1 level above where the new node will be placed.
// This function should never be passed a "NULL" pointer.  "ThisLetter" should never exist in the "Child" para-list.
void TnodeInsertParaNode(TnodePtr AboveTnode, char ThisLetter, char WordEnder, int StartDepth){
	AboveTnode->NumberOfChildren += 1;
	TnodePtr Holder = NULL;
	TnodePtr Currently = NULL;
	// Case 1: ParaList does not exist yet so start it.
	if ( AboveTnode->Child == NULL ) AboveTnode->Child = TnodeInit(ThisLetter, NULL, WordEnder, AboveTnode->Level + 1,
	StartDepth, AboveTnode, TRUE);
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

// The "MaxChildDepth" of the two nodes cannot be assumed equal due to the recursive nature of this function,
// so we must check for equivalence.
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

// This function is responsible for adding "Word" to the "Dawg" under its root node.
// It returns the number of new nodes inserted.
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
		// The path for the word that we are trying to insert already exists,
		// so just make sure that the end flag is flying on the last node.
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

// Recursively replaces all redundant nodes under "ThisTnode".
// "DirectChild" "Tnode" in a "Dangling" state have "ReplaceMeWith" set within them.
// Crosslinking of "Tnode"s being eliminated must be taken-care-of before this function gets called.
// When "Tnode" branches are crosslinked, the living branch has members being replaced with
// "Tnode"s in the branch being killed.
void TnodeLawnMowerRecurse(TnodePtr ThisTnode){
	if ( ThisTnode->Level == 0 ) TnodeLawnMowerRecurse(ThisTnode->Child);
	else {
		if ( ThisTnode->Next == NULL && ThisTnode->Child == NULL ) return;
		// The first "Tnode" being eliminated will always be a "DirectChild".
		if ( ThisTnode->Child != NULL ) {
			// The node is tagged to be "Mowed", so replace it with "ReplaceMeWith",
			// keep following "ReplaceMeWith" until an un"Dangling" "Tnode" is found.
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
// The Boolean "ValidReplacement" determines if the function will check for "Crosslink"s,
// or alter "Protected" and "ReplaceMeWith" states.
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


// It is of absolutely critical importance that only "DirectChild" nodes are pushed onto the queue as child nodes.
// This will not always be the case.
// In a DAWG a child pointer may point to an internal node in a longer list.  Check for this.
int BreadthQueueUseToIndex(BreadthQueuePtr ThisBreadthQueue, TnodePtr Root){
	int IndexNow = 0;
	TnodePtr Current = Root;
	// Push the first row onto the queue.
	while ( Current != NULL ) {
		BreadthQueuePush(ThisBreadthQueue, Current);
		Current = Current->Next;
	}
	// Pop each element off of the queue and only push its children if has not been "Dangled" yet.
	// Assign index if one has not been given to it yet.
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

struct arraydawg {
	int NumberOfStrings;
	ArrayDnodePtr DawgArray;
	int First;
	char MinStringLength;
	char MaxStringLength;
};

typedef struct arraydawg ArrayDawg;
typedef ArrayDawg* ArrayDawgPtr;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This function is the core of the DAWG creation procedure.  Pay close attention to the order of the steps involved.

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

	printf("\nStep 5 - Allocate a 2-D array of Tnode pointers to search for redundant Tnodes.\n");
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
	// Flagging requires the "TnodeAreWeTheSame()" function, and beginning with the highest
	// "MaxChildDepth" "Tnode"s will reduce the processing time.
	int NumberDangled = 0;
	int DangledNow;
	int NumberAtHeight;
	int TotalDangled = 0;
	int W = 0;
	
	// keep track of the number of nodes of each MaxChildDepth dangled recursively so we can check how many
	// remaining nodes we need for the optimal array.
	int DangleCount[Result->MaxStringLength];
	for ( X = 0; X < Result->MaxStringLength; X++) DangleCount[X] = 0;

	printf("\nStep 8 - Tag redundant Tnodes as Dangling - Use recursion, because only DirectChild Tnodes are considered for elimination:\n");
	printf("\n  This procedure is at the very heart of the DAWG creation alogirthm, and it would be much slower, without heavy optimization.\n");
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
						// When we "Dangle" a "Protected" "Tnode", we must set it's "ReplaceMeWith",
						// and a recursive function is needed for this special case.
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

	printf("\nStep 11 - Dawg-Lawn-Mowing is now complete, so assign array indicies to all living Tnodes using a Breadth-First-Queue.\n");
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

	printf("\nStep 12 - Populate the new Working-Array-Dawg structure, used to verify validity and create the final integer-graph-encodings.\n");
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
		CurrentNodeInteger <<= CHILD_BIT_SHIFT;
		CurrentNodeInteger += ((Result->DawgArray)[X].Letter) - 'A';
		if ( (Result->DawgArray)[X].EndOfWordFlag == TRUE ) CurrentNodeInteger += END_OF_WORD_BIT_MASK;
		if ( (Result->DawgArray)[X].Next == 0 ) CurrentNodeInteger += END_OF_LIST_BIT_MASK;
		fwrite( &CurrentNodeInteger, sizeof(int), 1, Data );
	}
	fclose(Data);
	printf( "\n  The Traditional-DAWG-Encoding data file is now written.\n" );
	
	printf("\nStep 14 - Output a text file with all the node information explicitly layed out.\n");
	
	FILE *Text;
	Text = fopen(TRADITIONAL_DAWG_TEXT_DATA,"w");

	char TheNodeInBinary[32+6+1];
	
	int CompleteThirtyTwoBitNode;
	
	fprintf(Text, "Behold, the |%d| Traditional DAWG nodes are decoded below:\r\n\r\n", NumberOfLivingNodes);
	
	// We are now ready to output to the text file, and the "Main" intermediate binary data file.
	for ( X = 1; X <= NumberOfLivingNodes ; X++ ){
		CompleteThirtyTwoBitNode = (Result->DawgArray)[X].Child;
		CompleteThirtyTwoBitNode <<= CHILD_BIT_SHIFT;
		CompleteThirtyTwoBitNode += (Result->DawgArray)[X].Letter - 'A';
		// The 2's complement sign bit is not needed, so a signed integer is acceptable.
		if ( (Result->DawgArray)[X].EndOfWordFlag == 1 ) CompleteThirtyTwoBitNode += END_OF_WORD_BIT_MASK;
		if ( (Result->DawgArray)[X].Next == 0 ) CompleteThirtyTwoBitNode += END_OF_LIST_BIT_MASK;
		ConvertIntNodeToBinaryString(CompleteThirtyTwoBitNode, TheNodeInBinary);
		fprintf(Text, "N%6d-%s, Raw|%8d|, Lev|%2d|", X, TheNodeInBinary, CompleteThirtyTwoBitNode, (Result->DawgArray)[X].Level);
		fprintf(Text, ", {'%c',%d,%6d", (Result->DawgArray)[X].Letter, (Result->DawgArray)[X].EndOfWordFlag, (Result->DawgArray)[X].Next);
		fprintf(Text, ",%6d}", (Result->DawgArray)[X].Child);
		fprintf(Text, ".\r\n");
		if ( CompleteThirtyTwoBitNode == 0 ) printf("\n  Error in node encoding process.\n");
	}
	
	fprintf(Text, "\r\nNumber Of Living Nodes |%d| Plus The NULL Node.\r\n\r\n", NumberOfLivingNodes);

	fclose(Text);
	
	printf("\nStep 15 - Creation of Traditional-DAWG-Encoding file complete.\n");

	return Result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main(){
	printf("\n  The 17-Step Traditional-DAWG-Creation-Process has commenced: (Hang in there, it will be over soon.)\n");
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
		if ( CurrentLength <= MAX ) strcpy( &((AllWordsInEnglish[CurrentLength])[(CurrentTracker[CurrentLength]*(CurrentLength + 1))]),
		LexiconInRam[X] );
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
	
	printf("\nStep 16 - Display the Mask-Format for the DAWG int-nodes:\n\n");
	
	char Something[32+6+1];
	ConvertIntNodeToBinaryString(END_OF_WORD_BIT_MASK, Something);
	printf("  %s - END_OF_WORD_BIT_MASK\n", Something);
	
	ConvertIntNodeToBinaryString(END_OF_LIST_BIT_MASK, Something);
	printf("  %s - END_OF_LIST_BIT_MASK\n", Something);
	
	ConvertIntNodeToBinaryString(CHILD_INDEX_BIT_MASK, Something);
	printf("  %s - CHILD_INDEX_BIT_MASK\n", Something);
	
	ConvertIntNodeToBinaryString(LETTER_BIT_MASK, Something);
	printf("  %s - LETTER_BIT_MASK\n", Something);
	
	return 0;
}
