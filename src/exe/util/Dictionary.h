#pragma once

// Max recursive verb conjugations
#define MAX_CONJ_DEPTH 5

#define POS_DUNNO		0
#define POS_NOUN		1
#define POS_VERB		2
#define POS_AUX_VERB	3
#define POS_ADJ			4
#define POS_ADV			5
#define POS_PART		6
#define POS_SYMBOL		7
// hack.
#define POS_LINEBREAK	8

#define MATCH_IS_NAME		0x0001

// Used for Kanji entry in Kanji-only words and
// Hiragana in Hiraga-only words.  Used in word sorting.
// Possibly scoring, too.  Currently set exactly when
// JAP_WORD_PRONOUNCE is nt.
#define JAP_WORD_PRIMARY		0x0001
// Pronounciation for something with a Kanji spelling.
// Currently no clue how common these are.  May
// penalize slightly.
#define JAP_WORD_PRONOUNCE		0x0002
// Crude and often inaccurate frequency info.
#define JAP_WORD_COMMON			0x0004
// Given a slight bonus over other words.
#define JAP_WORD_PART			0x0008

// Set for final Japanese string, just before english.
#define JAP_WORD_FINAL			0x8000

void LoadDicts();
void CleanupDicts();

struct ConjInfo {
	// Index into verb type array
	unsigned short verbType;
	// Non-past, past, etc.  Index into tense array.
	unsigned short verbTense;
	// Non-past, past, etc.  Index into conjugation array of type.
	unsigned short verbConj;
	// formal/informal, pos/neg.
	unsigned char verbForm;
};

// Note:  Total size must be a multiple of 4 bytes for dictionary code to work right,
// so jstring's size must be adjusted according to size of earlier bytes.
struct JapString {
	// Distance first Japanese string for this entry is before current string.
	unsigned short startOffset;

	// Index into verb type array.
	unsigned short verbType;

	unsigned int flags;

	wchar_t jstring[2];
};

struct Match {
	int start;
	short len;
	// Not len for verbs.
	short srcLen;
	// Specific to matches
	unsigned short matchFlags;
	// Copied from source japanese word.
	unsigned short japFlags;
	unsigned short dictIndex;

	// First jstring of entry.
	JapString *firstJString;

	// Actual string hit.
	JapString *jString;

	// int entry;

	ConjInfo conj[MAX_CONJ_DEPTH];
	// If hiragana/katakana don't match.  Must be last.
	int inexactMatch;
};

void FindMatches(wchar_t *string, int len, Match *&matches, int &numMatches);
void FindBestMatches(wchar_t *string, int len, Match *&matches, int &numMatches, int useMecab);
int DictsLoaded();

struct EntryData {
	char *english;
	int numJap;
	// More than I'll ever need...
	JapString *jap[256];
	wchar_t kuruHack[4];
};

struct POSData {
	// For quick reference.  These are 0-based indices, in pos array, they
	// start at POS_LINEBREAK+1
	// Note:  I only find the first string match, not all matching types.
	int numVerbTypes;
	int verbTypes[10];
	// Each verb type is counted as a POS, so better to have a little extra space.
	unsigned char pos[300];
};

int GetDictEntry(int dictIndex, JapString *jString, EntryData *out);

void FindExactMatches(wchar_t *string, int len, Match *&matches, int &numMatches);
void GetPartsOfSpeech(char *eng, POSData *pos);
int GetConjString(wchar_t *temp, Match *match);

// Also merges identical verb conjugations.
void SortMatches(Match *&matches, int &numMatches);

int GetDictEntry(Match &match, EntryData *out);
