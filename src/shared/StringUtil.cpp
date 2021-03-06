#include <Shared\Shrink.h>
#include <Shared\StringUtil.h>
#include "ConversionTable.h"


#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>

#ifndef NO_ZLIB
// Not building a library, don't export/import the symbols, for the love of god.
#define ZEXTERN
#include <3rd Party/zlib/zlib.h>
#endif

struct RomajiTable {
	wchar_t jap[3];
	char ascii[4];
};

static RomajiTable romajiTable[] = {
	{L"キャ", "kya"},
	{L"キュ", "kyu"},
	{L"キョ", "kyo"},
	{L"シャ", "sha"},
	{L"シュ", "shu"},
	{L"ショ", "sho"},
	{L"チャ", "cha"},
	{L"チュ", "chu"},
	{L"チョ", "cho"},
	{L"ニャ", "nya"},
	{L"ニュ", "nyu"},
	{L"ニョ", "nyo"},
	{L"ヒャ", "hya"},
	{L"ヒュ", "hyu"},
	{L"ヒョ", "hyo"},
	{L"ミャ", "mya"},
	{L"ミュ", "myu"},
	{L"ミョ", "myo"},
	{L"リャ", "rya"},
	{L"リュ", "ryu"},
	{L"リョ", "ryo"},
	{L"ヰャ", "wya"},
	{L"ヰュ", "wyu"},
	{L"ヰョ", "wyo"},
	{L"ギャ", "gya"},
	{L"ギュ", "gyu"},
	{L"ギョ", "gyo"},
	{L"ジャ", "ja"},
	{L"ジュ", "ju"},
	{L"ジョ", "jo"},
	{L"ヂャ", "ja"},
	{L"ヂュ", "ju"},
	{L"ヂョ", "jo"},
	{L"ビャ", "bya"},
	{L"ビュ", "byu"},
	{L"ビョ", "byo"},
	{L"ピャ", "pya"},
	{L"ピュ", "pyu"},
	{L"ピョ", "pyo"},
	{L"イェ", "ye"},
	{L"ユェ", "ye"},
	{L"ヴァ", "va"},
	{L"ヴィ", "vi"},
	{L"ヴェ", "ve"},
	{L"ヴォ", "vo"},
	{L"ヴャ", "vya"},
	{L"ヴュ", "vyu"},
	{L"ヴョ", "vyo"},
	{L"シェ", "she"},
	{L"ジェ", "je"},
	{L"チェ", "che"},
	{L"スィ", "si"},
	{L"スャ", "sya"},
	{L"スュ", "syu"},
	{L"スョ", "syo"},
	{L"ズィ", "zi"},
	{L"ズャ", "zya"},
	{L"ズュ", "zyu"},
	{L"ズョ", "zyo"},
	{L"ティ", "ti"},
	{L"トゥ", "tu"},
	{L"テャ", "tya"},
	{L"テュ", "tyu"},
	{L"テョ", "tyo"},
	{L"ディ", "di"},
	{L"ドゥ", "du"},
	{L"デャ", "dya"},
	{L"デュ", "dyu"},
	{L"デョ", "dyo"},
	{L"ツァ", "tsa"},
	{L"ツィ", "tsi"},
	{L"ツェ", "tse"},
	{L"ツォ", "tso"},
	{L"ファ", "fa"},
	{L"フィ", "fi"},
	{L"ホゥ", "hu"},
	{L"フェ", "fe"},
	{L"フォ", "fo"},
	{L"フャ", "fya"},
	{L"フュ", "fyu"},
	{L"フョ", "fyo"},
	{L"リェ", "rye"},
	{L"ウァ", "wa"},
	{L"ウィ", "wi"},
	{L"ウェ", "we"},
	{L"ウォ", "wo"},
	{L"ウャ", "wya"},
	{L"ウュ", "wyu"},
	{L"ウョ", "wyo"},
	{L"クァ", "kwa"},
	{L"クヮ", "kwa"},
	{L"クィ", "kwi"},
	{L"クゥ", "kwu"},
	{L"クェ", "kwe"},
	{L"クォ", "kwo"},
	{L"グァ", "gwa"},
	{L"グヮ", "gwa"},
	{L"グィ", "gwi"},
	{L"グゥ", "gwu"},
	{L"グェ", "gwe"},
	{L"グォ", "gwo"},
	{L"ァ", "a"},
	{L"ィ", "i"},
	{L"ゥ", "u"},
	{L"ェ", "e"},
	{L"ォ", "o"},
	{L"ャ", "ya"},
	{L"ュ", "yu"},
	{L"ョ", "yo"},
	{L"ヮ", "wa"},
	{L"ア", "a"},
	{L"イ", "i"},
	{L"ウ", "u"},
	{L"エ", "e"},
	{L"オ", "o"},
	{L"カ", "ka"},
	{L"キ", "ki"},
	{L"ク", "ku"},
	{L"ケ", "ke"},
	{L"コ", "ko"},
	{L"サ", "sa"},
	{L"シ", "shi"},
	{L"ス", "su"},
	{L"セ", "se"},
	{L"ソ", "so"},
	{L"タ", "ta"},
	{L"チ", "chi"},
	{L"ツ", "tsu"},
	{L"テ", "te"},
	{L"ト", "to"},
	{L"ナ", "na"},
	{L"ニ", "ni"},
	{L"ヌ", "nu"},
	{L"ネ", "ne"},
	{L"ノ", "no"},
	{L"ハ", "ha"},
	{L"ヒ", "hi"},
	{L"フ", "fu"},
	{L"ヘ", "he"},
	{L"ホ", "ho"},
	{L"マ", "ma"},
	{L"ミ", "mi"},
	{L"ム", "mu"},
	{L"メ", "me"},
	{L"モ", "mo"},
	{L"ヤ", "ya"},
	{L"ユ", "yu"},
	{L"ヨ", "yo"},
	{L"ラ", "ra"},
	{L"リ", "ri"},
	{L"ル", "ru"},
	{L"レ", "re"},
	{L"ロ", "ro"},
	{L"ワ", "wa"},
	{L"ヰ", "wi"},
	{L"ヱ", "we"},
	{L"ヲ", "wo"},
	{L"ン", "n"},
	{L"ガ", "ga"},
	{L"ギ", "gi"},
	{L"グ", "gu"},
	{L"ゲ", "ge"},
	{L"ゴ", "go"},
	{L"ザ", "za"},
	{L"ジ", "ji"},
	{L"ズ", "zu"},
	{L"ゼ", "ze"},
	{L"ゾ", "zo"},
	{L"ダ", "da"},
	{L"ヂ", "ji"},
	{L"ヅ", "dzu"},
	{L"デ", "de"},
	{L"ド", "do"},
	{L"バ", "ba"},
	{L"ビ", "bi"},
	{L"ブ", "bu"},
	{L"ベ", "be"},
	{L"ボ", "bo"},
	{L"パ", "pa"},
	{L"ピ", "pi"},
	{L"プ", "pu"},
	{L"ペ", "pe"},
	{L"ポ", "po"},
	{L"ヴ", "vu"},
	{L"ー", ""},
	{L"ュ", ""},
};

int ChunkToRomaji(wchar_t *in, wchar_t *out, int *eaten) {
	int c1 = in[0];
	*eaten = 0;
	if (c1 >= 0x3041 && c1 <= 0x3093) {
		c1 += 0x60;
	}
	// 0x30FC is 'ー'
	if (c1 < 0x30A1 || c1 > 0x30FC || c1 == 0x30FB) {
		*eaten = 0;
		return 0;
	}
	int c2 = in[1];
	if (c2 >= 0x3041 && c2 <= 0x3093) {
		c2 += 0x60;
	}
	if (c1 == L'ッ') {
		for (int i=0; i<sizeof(romajiTable)/sizeof(romajiTable[0]); i++) {
			if (c2 == romajiTable[i].jap[0] && !romajiTable[i].jap[1]) {
				wchar_t L = romajiTable[i].ascii[0];
				if (L != 'a' && L != 'e' && L != 'i' && L != 'o' && L != 'u') {
					out[0] = L;
					*eaten = 1;
					return 1;
				}
			}
		}
		*eaten = 0;
		return 0;
	}
	for (int i=0; i<sizeof(romajiTable)/sizeof(romajiTable[0]); i++) {
		if (c1 == romajiTable[i].jap[0] &&
			(romajiTable[i].jap[1] == c2 || !romajiTable[i].jap[1])) {
				int count = 0;
				while (romajiTable[i].ascii[count]) {
					out[count] = romajiTable[i].ascii[count];
					count++;
				}
				*eaten = 2 - !romajiTable[i].jap[1];
				return count;
		}
	}
	*eaten = 0;
	return 0;
}


int ChunkToKatakana(wchar_t *in, wchar_t *out, int *eaten) {
	*eaten = 0;
	int count = 0;
	wchar_t c[4];
	do {
		c[0] = in[eaten[0]];
		eaten[0] ++;
	}
	while (c[0] == ' ' || c[0] == '\t');
	if (c[0] < 0x80)
		c[0] = tolower(c[0]);
	if (c[0] < 'a' || c[0] > 'z') {
		if (c[0] >= 0x3041 && c[0] <= 0x3096) {
			out[0] = c[0] + 0x60;
			return 1;
		}
		out[0] = 0;
		switch (c[0]) {
			case ',':
				out[0] = L'、';
				return 1;
			case '.':
				out[0] = L'。';
				return 1;
			case '?':
				out[0] = L'？';
				return 1;
			case '!':
				out[0] = L'！';
				return 1;
			default:
				break;
		}
		out[0] = in[0];
		eaten[0] = 1;
		return eaten[0];
	}
	char vowels[5] = {'a','i','u','e','o'};
	wchar_t jvowels[5] = {L'ア', L'イ', L'ウ', L'エ', L'オ'};
	int w;
	for (w=0; w<5; w++) {
		if (c[0] == vowels[w]) {
			out[0] = jvowels[w];
			return 1;
		}
	}

	c[1] = tolower(in[eaten[0]]);

	while (c[0] == c[1] && c[0] != 'n') {
		out[count] = L'ッ';
		count ++;
		c[0] = c[1];
		eaten[0]++;
		c[1] = tolower(in[eaten[0]]);
	}
	for (w=0; w<5; w++) {
		if (c[1] == vowels[w]) {
			break;
		}
	}
	int len = 2;
	if (w==5) {
		if (c[1] != 'y') {
			if (c[0] == 'n') {
				out[0] = L'ン';
				return 1;
			}
			/*
			out[0] = in[0];
			*eaten = 1;
			return 1;
			//*/
		}
		c[2] = tolower(in[eaten[0]+1]);
		len++;
	}
	c[len] = 0;
	char a[4];
	for (int i=0; c[i-1]; i++) {
		a[i] = (char)c[i];
	}
	for (int i=sizeof(romajiTable)/sizeof(romajiTable[0])-1; i>=0; i--) {
		if (!strcmp(a, romajiTable[i].ascii)) {
			wcscpy(out+count, romajiTable[i].jap);
			eaten[0] += len-1;
			return count+wcslen(out+count);
		}
	}
	out[0] = in[0];
	*eaten = 1;
	return 1;
}

int ToKatakana(wchar_t *in, wchar_t *out) {
	int i = 0;
	int j = 0;
	while (in[i]) {
		int read;
		j += ChunkToKatakana(in+i, out+j, &read);
		i+=read;
	}
	out[j] = 0;
	return j;
}

int ToHiragana(wchar_t *in, wchar_t *out) {
	ToKatakana(in, out);
	int i;
	for (i=0; out[i]; i++) {
		if (out[i] >= 0x3041+0x60 && out[i] <= 0x3096+0x60) {
			out[i] -= 0x60;
		}
	}
	return i;
}

int ToRomaji(wchar_t *in, wchar_t *out) {
	int i = 0;
	int j = 0;
	while (in[i]) {
		int read;
		int written = ChunkToRomaji(in+i, out+j, &read);
		if (!read) {
			out[j++] = in[i++];
			continue;
		}
		j += written;
		i+=read;
	}
	out[j] = 0;
	return j;
}
/*
int IsOpenBracket(wchar_t c) {
	return  c == L'（' || c == L'）' ||
			c == L'('  || c == L')'  ||
			c == L'['  || c == L']'  ||
			c == L'［' || c == L'］' ||
			c == L'〔' || c == L'〕' ||
			c == L'【' || c == L'】' ||
			c == L'〚' || c == L'〛';
}

int IsCloseBracket(wchar_t c) {
}//*/

/*
static inline int IsMajorPunctuation(wchar_t c, DWORD flags) {
	return (c == L'。' || c == L'？' || c == L'.' || c == L'?' || c == L'！' || c == '!' ||
			((flags & BREAK_ON_ELLIPSES) && (c == L'…' || c == L'‥')) ||
			((flags & BREAK_ON_COMMAS) && c == L'、') ||
			((flags & BREAK_ON_QUOTES) && (c == L'「' || c == L'」')) ||
			((flags & BREAK_ON_DOUBLE_QUOTES) && (c == L'『' || c == L'』')) ||
			((flags & BREAK_ON_PARENTHESES) && (c == L'（' || c == L'）' || c == L'(' || c == L')')) ||
			// First two sets are different unicode characters, they just look the same
			// on a lost of fonts.
			((flags & BREAK_ON_SQUARE_BRACKETS) && (c == L'[' || c == L']' ||
													c == L'［' || c == L'］' ||
													c == L'〔' || c == L'〕' ||
													c == L'【' || c == L'】' ||
													c == L'〚' || c == L'〛')) ||
			((flags & BREAK_ON_SINGLE_LINE_BREAKS) && (c == L'\n' || c == L'\r'))
		);
}


// Not a complete list - things that can appear in the middle of sentences as well as around them.
// Only exclude when at the ends of text.
static inline int IsMinorPunctuation(wchar_t c) {
	return (c == L'（' || c == L'）' || c == L'「' || c == L'」' || c == L'(' || c == L')' || c == L'"' || c == L'\'');
}//*/

int IsMajorPunctuation(wchar_t c, DWORD flags) {
	return (c == L'。' || c == L'？' || c == L'.' || c == L'?' || c == L'！' || c == '!' ||
			((flags & BREAK_ON_ELLIPSES) && (c == L'…' || c == L'‥')) ||
			((flags & BREAK_ON_COMMAS) && c == L'、') ||
			((flags & BREAK_ON_QUOTES) && (c == L'「' || c == L'」' ||  c == L'"')) ||
			((flags & BREAK_ON_DOUBLE_QUOTES) && (c == L'『' || c == L'』' || c == L'\'')) ||
			((flags & BREAK_ON_PARENTHESES) && (c == L'（' || c == L'）' || c == L'(' || c == L')')) ||
			// First two sets are different unicode characters, they just look the same
			// on a lost of fonts.
			((flags & BREAK_ON_SQUARE_BRACKETS) && (c == L'[' || c == L']' ||
													c == L'［' || c == L'］' ||
													c == L'〔' || c == L'〕' ||
													c == L'【' || c == L'】' ||
													c == L'〚' || c == L'〛')) ||
			((flags & BREAK_ON_SINGLE_LINE_BREAKS) && (c == L'\n' || c == L'\r'))
		);
}


// Not a complete list - things that can appear in the middle of sentences as well as around them.
// Only exclude when at the ends of text.
int IsPunctuation(wchar_t c) {
	return (IsMajorPunctuation(c, ~BREAK_ON_SINGLE_LINE_BREAKS));
}

int IsKatakana(wchar_t c) {
	return (0x30A0 <= c && c < 0x3100);
}

int IsHiragana(wchar_t c) {
	return (0x3040 <= c && c < 0x30A0);
}

int IsHalfWidthKatakana(wchar_t c) {
	return (0xFF65 <= c && c < 0xFF9C);
}

int IsKanji(wchar_t c) {
	// 0x3005 is repeat kanji character.
	return (0x4E00 <= c && c < 0x9FC0) || c == 0x3005;
}

int IsJap(wchar_t c) {
	return (IsHiragana(c) || IsKatakana(c) || IsKanji(c) || IsHalfWidthKatakana(c));
}

int HasJap(wchar_t *s) {
	while (*s) {
		if (IsJap(*s))
			return 1;
		s++;
	}
	return 0;
}

wchar_t *GetSubstring(wchar_t *string, wchar_t *prefix, wchar_t* suffix, int *len) {
	*len = 0;
	wchar_t *start = wcsstr(string, prefix);
	if (!start) return 0;
	start += wcslen(prefix);
	wchar_t *end = wcsstr(start, suffix);
	if (end) {
		*len = (int)(end-start);
	}
	return start;
}

wchar_t *ToWideChar(const char *s, int cp, int &len) {
	if (!s) {
		len = -1;
		return 0;
	}
	if (cp == -1) {
		// Hack to make -1 mean it's really already utf16.
		// Used for mecab.
		if (len<0) len = wcslen((wchar_t*)s);
		else len--;
		return wcsdup((wchar_t*)s);
	}
	if (len < 0) len = 1+strlen(s);
	wchar_t *out = (wchar_t*) malloc(sizeof(wchar_t) * (len+1));

	ConversionTable *cTable;
	if (cp == 20932 && (cTable = GetEucJpTable())) {
		len = cTable->DecodeString(s, len, out);
	}
	else {
		len = MultiByteToWideChar(cp, 0, s, len, out, len);
	}

	if (!len) {
		free(out);
		return 0;
	}
	return (wchar_t*) realloc(out, len*sizeof(wchar_t));
}

char *ToMultiByte(const wchar_t *s, int cp, int &len) {
	if (cp == -1) {
		// Hack to make -1 mean it's really already utf16.
		// Used for mecab.
		if (len<0) len = 1 + wcslen(s);
		return (char*) wcsdup(s);
	}
	if (len < 0) len = 1+wcslen(s);
	char *out = (char*) malloc(sizeof(char) * 4 * len);
	len = WideCharToMultiByte(cp, 0, s, len, out, 4 * len, 0, 0);
	if (!len) {
		free(out);
		return 0;
	}
	return (char*) realloc(out, len);
}

void SpiffyUp(wchar_t *s) {
	wchar_t *start = s;
	wchar_t *out = s;
	while (MyIsWspace(*s)) s++;
	while (*s) {
		if (*s != '\r') {
			*out = *s;
			out++;
			s++;
		}
		else if (s[1] == '\n') {
			s++;
			continue;
		}
		else {
			*out = '\n';
			out++;
			s++;
		}
		if (out[-1] == '\n') {
			while (MyIsWspace(*s) && *s != '\n' && *s != '\r') s++;
		}
	}
	int tbr = 0;
	while (out > start) {
		if (out[-1] == '\n') {
			out --;
			tbr = 1;
		}
		else if (out[-1] == '\t' || out[-1] == ' ') out--;
		else break;
	}
	if (tbr) out++[0] = '\n';
	*out = 0;
}

#ifdef NO_ZLIB
#define gzFile int
#define gzdopen(x, y) (x)
#define gzread(x, y, z) read(x, y, z)
#define gzclose(x) close(x)
#endif

void LoadFile(wchar_t *path, wchar_t ** data, int *size, int raw) {
	*data = 0;
	*size = 0;
	int fd = _wopen(path, _O_RDONLY | _O_BINARY);
	if (fd == -1) return;
	int fsize = 10000;

	gzFile file = gzdopen(fd, "rb");
	if (!file) {
		close(fd);
		return;
	}

	char *temp = 0;
	int cursize = 0;
	int maxsize = fsize;
	while (1) {
		if (maxsize > 500*1024*1024) {
			free(temp);
			temp = 0;
			break;
		}
		if (maxsize < 100*1024*1024)
			maxsize = maxsize*2 + 10000;
		else
			maxsize = maxsize/2*3;
		// +4 is so I can throw on a couple terminating nulls.
		// Not really needed here, but means final realloc really shouldn't fail.
		char *temp2 = (char*) realloc(temp, maxsize+4);

		// Grow until death.  I'm lazy.
		if (!temp2) continue;

		temp = temp2;
		int r = gzread(file, temp+cursize, maxsize-cursize);
		if (r <= 0) break;
		cursize += r;
		if (cursize < maxsize) break;
	}
	gzclose(file);
	if (!temp) return;

	temp = (char*) realloc(temp, cursize+4);

	for (int i=0; i<4; i++)
		temp[cursize + i] = 0;

	if (raw) {
		*size = cursize;
		*data = (wchar_t*)temp;
		return;
	}

	wchar_t* wtemp = (wchar_t *) temp;
	fsize = cursize;
	int utf16 = 0;
	if (!(fsize&1) && fsize) {
		int i = 0;
		for (i=0; i<2 && i<fsize; i++) {
			if (!temp[i]) break;
		}
		if (i<2) utf16 = 1;
		if (wtemp[0] == 0xFFFE || (i <fsize && i < 2 && !(i&1))) {
			for (int j=0; j<fsize; j+=2) {
				char c = temp[j];
				temp[j] = temp[j+1];
				temp[j+1] = c;
			}
			utf16 = 1;
		}
		if (wtemp[0] == 0xFEFF) {
			utf16 = 1;
			// Include terminating null.
			memmove(wtemp, wtemp+1, fsize);
			fsize-=2;
		}
		if (utf16) {
			*data = wtemp;
			*size = fsize/2;
		}
	}

	if (!utf16) {
		char *checkPos = temp;
		char *created = 0;
		if (created = strstr(temp, "/Created: ")) {
			while (created > temp && created[-1] != '\n' && created[-1] != '\r')
				created--;
			checkPos = created;
		}

		// Include terminating null;
		*size = cursize+1;
		if ((unsigned char)checkPos[0] == 0xA1 && ((unsigned char)checkPos[1]&~0x8) == 0xA1 && (unsigned char)checkPos[2] == 0xA1 && ((unsigned char)checkPos[3]&~0x8) == 0xA1) {
			*data = ToWideChar(temp, 20932, *size);
		}
		else if ((unsigned char)checkPos[0] == 0x81 && ((unsigned char)checkPos[1]&~0x8) == 0x40 && (unsigned char)checkPos[2] == 0x81 && ((unsigned char)checkPos[3]&~0x8) == 0x40) {
			*data = ToWideChar(temp, 932, *size);
		}
		else if ((unsigned char)checkPos[0] == 0xEF && (unsigned char)checkPos[1] == 0xBB && (unsigned char)temp[2] == 0xBF) {
			*size -= 3;
			*data = ToWideChar(temp+3, CP_UTF8, *size);
		}
		else {
			*data = ToWideChar(temp, CP_UTF8, *size);
		}
		free(temp);
	}
	int inPos = 0, outPos = 0;
	while (inPos < *size) {
		if (data[0][inPos] != '\r') {
			data[0][outPos++] = data[0][inPos++];
			continue;
		}
		data[0][outPos++] = '\n';
		inPos++;
		if (data[0][inPos] == '\n') inPos++;
	}
	data[0][outPos] = 0;
	*size = outPos;
}
