// This program demonstrates the use of a traditional DAWG as applied to in-order anagramming, where the data structure really shines.

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAX 15
#define MAX_INPUT 120
#define BIG_IT_UP -32
#define DAWG_DATA "Traditional_Dawg_For_Word-List.dat"
#define WORD_LIST_OUTPUT "Dawg_Word_List.txt"

// These values define the format of the "Dawg" node encoding.
#define CHILD_BIT_SHIFT 5
#define LETTER_BIT_MASK 0X0000001F
#define CHILD_INDEX_BIT_MASK 0X003FFFE0
#define END_OF_WORD_BIT_MASK 0X00800000
#define END_OF_LIST_BIT_MASK 0X00400000

// Define the boolean type as an enumeration.
typedef enum {FALSE = 0, TRUE = 1} Bool;
typedef Bool* BoolPtr;

// When reading strings from a file, the new-line character is appended, and this macro will remove it before processing.
#define CUT_OFF_NEW_LINE(string) (string[strlen(string) - 1] = '\0')

// For speed, define these two simple functions as macros.  They modify the "LettersToWorkWith" string in the recursive anagrammer.
#define REMOVE_CHAR_FROM_STRING(thestring, theposition, shiftsize) ( memmove(thestring + theposition, thestring + theposition + 1, shiftsize) )
#define INSERT_CHAR_IN_STRING(ts, tp, thechar, shiftsize) ( (memmove(ts + tp + 1, ts + tp, shiftsize)), (ts[tp] = thechar) )

// This function converts any lower case letters in the string "RawWord", into capitals, so that the whole string is made of capital letters.
void MakeMeAllCapital(char *RawWord){
	unsigned int X;
	for ( X = 0; X < strlen(RawWord); X++ ){
		if ( RawWord[X] >= 'a' && RawWord[X] <= 'z' ) RawWord[X] = RawWord[X] + BIG_IT_UP;
	}
}

// This function removes all non-letter chars from "ThisString".
void RemoveIllegalChars(char *ThisString){
	unsigned int X;
	for ( X = 0; X < strlen(ThisString); X++ ) {
		if ( !(ThisString[X] >= 'A' && ThisString[X] <= 'Z') && !(ThisString[X] >= 'a' && ThisString[X] <= 'z') ) {
			memmove(ThisString + X, ThisString + X + 1, strlen(ThisString) - X);
			X -= 1;
		}
	}
}

// This is a simple Bubble Sort.
void Alphabetize(char* Word){
	int X;
	int Y;
	char WorkingChar;
	int WordSize = strlen(Word);
	for( X = 1; X < WordSize; X++ ) {
		for( Y = 0; Y <= (WordSize - X - 1); Y++ ) {
			if (Word[Y] > Word[Y + 1]){
				WorkingChar = Word[Y + 1];
				Word[Y + 1] = Word[Y];
				Word[Y] = WorkingChar;
			}
		}
	}
}

// Define the "Dawg" functionality as macros for speed.
#define DAWG_LETTER(thearray, theindex) ((thearray[theindex]&LETTER_BIT_MASK) + 'A')
#define DAWG_END_OF_WORD(thearray, theindex) ((thearray[theindex]&END_OF_WORD_BIT_MASK)? TRUE: FALSE)
#define DAWG_NEXT(thearray, theindex) ((thearray[theindex]&END_OF_LIST_BIT_MASK)? 0: (theindex + 1))
#define DAWG_CHILD(thearray, theindex) ((thearray[theindex]&CHILD_INDEX_BIT_MASK)>>CHILD_BIT_SHIFT)

// A recursive depth first traversal of "TheDawg" lexicon to produce a readable wordlist in "TheStream".
void DawgTraverseLexiconRecurse(unsigned int *TheDawg, int CurrentIndex, int FillThisPosition,
 char *WorkingString, int *TheCount, FILE *TheStream){
	int PassOffIndex;
	WorkingString[FillThisPosition] = DAWG_LETTER(TheDawg, CurrentIndex);
	if ( DAWG_END_OF_WORD(TheDawg, CurrentIndex) ) {
		*TheCount += 1;
		WorkingString[FillThisPosition + 1] = '\0';
		// Include the Windows Carriage Return char.
		fprintf(TheStream, "|%6d|-|%-15s|\r\n", *TheCount, WorkingString);
	}
	if ( PassOffIndex = DAWG_CHILD(TheDawg, CurrentIndex) ) DawgTraverseLexiconRecurse(TheDawg,
	PassOffIndex, FillThisPosition + 1, WorkingString, TheCount, TheStream);
	if ( PassOffIndex = DAWG_NEXT(TheDawg, CurrentIndex) ) DawgTraverseLexiconRecurse(TheDawg,
	PassOffIndex, FillThisPosition, WorkingString, TheCount, TheStream);
}

// Move through the entire "ThisDawg" lexicon, and print the words into "ThisStream".
void DawgTraverseLexicon(unsigned int *ThisDawg, FILE *ThisStream){
	char *BufferWord = (char*)malloc((MAX + 1)*sizeof(char));
	int *WordCounter = (int*)malloc(sizeof(int));
	*WordCounter = 0;
	// Include the Windows Carriage Return char.
	fprintf(ThisStream, "This is the lexicon contained in the file |%s|.\r\n\r\n", DAWG_DATA);
	DawgTraverseLexiconRecurse(ThisDawg, 1, 0, BufferWord, WordCounter, ThisStream);
	free(BufferWord);
	free(WordCounter);
}

// This function is the core component of this program.
// It requires that "UnusedChars" be in alphabetical order because the tradition Dawg is a list based structure.
void DawgAnagrammerRecurse(unsigned int *DawgOfWar, int CurrentIndex, char *ToyWithMe,
 int FillThisPosition, char *UnusedChars, int SizeOfBank, int *ForTheCounter){
	int X;
	char PreviousChar = '\0';
	char CurrentChar;
	int TempIndex = DAWG_CHILD(DawgOfWar, CurrentIndex);
	
	ToyWithMe[FillThisPosition] = DAWG_LETTER(DawgOfWar, CurrentIndex);
	if ( DAWG_END_OF_WORD(DawgOfWar, CurrentIndex) ) {
		*ForTheCounter += 1;
		ToyWithMe[FillThisPosition + 1] = '\0';
		printf("|%4d| - |%-15s|\n", *ForTheCounter, ToyWithMe);
	}
	if ( (SizeOfBank > 0) && (TempIndex != 0) ) {
		for ( X = 0; X < SizeOfBank; X++ ) {
			CurrentChar = UnusedChars[X];
			if ( CurrentChar == PreviousChar ) continue;
			do {
				if ( CurrentChar == DAWG_LETTER(DawgOfWar, TempIndex) ) {
					REMOVE_CHAR_FROM_STRING(UnusedChars, X, SizeOfBank - X);
					DawgAnagrammerRecurse(DawgOfWar, TempIndex, ToyWithMe, FillThisPosition + 1,
					UnusedChars, SizeOfBank - 1, ForTheCounter);
					INSERT_CHAR_IN_STRING(UnusedChars, X, CurrentChar, SizeOfBank - X);
					TempIndex = DAWG_NEXT(DawgOfWar, TempIndex);
					break;
				}
				else if ( CurrentChar < DAWG_LETTER(DawgOfWar, TempIndex) ) break;
			} while ( TempIndex = DAWG_NEXT(DawgOfWar, TempIndex) );
			if ( TempIndex == 0 ) break;
			PreviousChar = CurrentChar;
		}
	}
}

// This function uses "MasterDawg" to determine the the words that can be made from the letters in "CharBank".
// The return value is the total number of words found.
int DawgAnagrammer(unsigned int *MasterDawg, char * CharBank){
	int X;
	int Result;
	int BankSize = strlen(CharBank);
	int *ForTheCount = (int*)malloc(sizeof(int));
	char *TheWordSoFar = (char*)malloc((MAX + 1)*sizeof(char));
	char *LettersToWorkWith = (char*)malloc((MAX_INPUT)*sizeof(char));
	char PreviousChar = '\0';
	char CurrentChar;
	int NumberOfLetters;
	strcpy(LettersToWorkWith, CharBank);
	NumberOfLetters = strlen(LettersToWorkWith);
	
	*ForTheCount = 0;
	for ( X = 0; X < BankSize; X++ ) {
		CurrentChar = CharBank[X];
		// Move to the next letter if we have already processed the "CurrentChar".
		if ( CurrentChar == PreviousChar ) continue;
		// We can assume that every letter in the lexicon exists on the first row,
		// so don't bother looking for them.  Just remove the one we are using and plug away.
		REMOVE_CHAR_FROM_STRING(LettersToWorkWith, X, NumberOfLetters - X);
		DawgAnagrammerRecurse(MasterDawg, CurrentChar - '@', TheWordSoFar, 0, LettersToWorkWith,
		NumberOfLetters - 1, ForTheCount);
		INSERT_CHAR_IN_STRING(LettersToWorkWith, X, CurrentChar, NumberOfLetters - X);
		PreviousChar = CurrentChar;
	}
	Result = *ForTheCount;
	free(ForTheCount);
	free(TheWordSoFar);
	free(LettersToWorkWith);
	return Result;
}

int main() {
	int NumberOfNodes;
	unsigned int *TheDawgArray;
	FILE *Lexicon;
	FILE *WordList;
	char *DecisionInput;
	Bool FetchData = TRUE;
	char FirstChar;
	int InputSize;
	
	DecisionInput = (char*)malloc(MAX_INPUT*sizeof(char));
	Lexicon = fopen(DAWG_DATA, "rb");
	fread(&NumberOfNodes, sizeof(int), 1, Lexicon);
	printf("The lexicon DAWG contains |%d| nodes.\n", NumberOfNodes);
	TheDawgArray = (unsigned int*)malloc(NumberOfNodes*sizeof(unsigned int));
	fread(TheDawgArray, sizeof(unsigned int), NumberOfNodes, Lexicon);
	fclose(Lexicon);
	printf("\nLexicon data file TWL06 has been read into memory.\n\n");
	
	// This program relies on a compressed lexicon data file,
	// so allow the user to see a readable word list in an output file.
	while ( FetchData == TRUE ) {
		printf("\nWould you like to print the Dawg word list into a file?(Y/N):  ");
		fgets(DecisionInput, MAX_INPUT, stdin);
		FirstChar = DecisionInput[0];
		if ( FirstChar == 'Y' || FirstChar == 'y' ) {
			WordList = fopen(WORD_LIST_OUTPUT, "w");
			DawgTraverseLexicon(TheDawgArray, WordList);
			fclose(WordList);
			FetchData = FALSE;
		}
		else if ( FirstChar == 'N' || FirstChar == 'n' ) FetchData = FALSE;
	}
	FetchData = TRUE;
	
	// Now the user can enter strings of letters for anagramming, to see the words that can be made from them.
	while ( FetchData == TRUE ) {
		printf("\nEnter the string of letters that you want to anagram(2 or more letters):  ");
		fgets(DecisionInput, MAX_INPUT, stdin);
		CUT_OFF_NEW_LINE(DecisionInput);
		RemoveIllegalChars(DecisionInput);
		MakeMeAllCapital(DecisionInput);
		Alphabetize(DecisionInput);
		InputSize = strlen(DecisionInput);
		if ( InputSize >= 2 ) {
			printf("\nThis is the set of letters that you just input |%s|.\n\n", DecisionInput);
			printf("\n|%d| Words were found in the lexicon Dawg.\n", DawgAnagrammer(TheDawgArray, DecisionInput));
		}
		else FetchData = FALSE;
	}
	printf("\nThank you for playing the TraditionalDawgAnagrammer.  GAME OVER.\n\n");
	return 0;
}
