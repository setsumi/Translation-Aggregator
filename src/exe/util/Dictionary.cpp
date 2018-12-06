#include <Shared/Shrink.h>
#include "Dictionary.h"
#include <Shared/StringUtil.h>
#include "Mecab.h"

#define DICT_MAGIC 'TCD\00'
#define DICT_VERSION 0x0009

#define DICT_FLAG_NAMES		0x0001

/*
struct DictionaryEntry {
	unsigned char numJStrings;
	unsigned char reserved[3];
	// Each entry is actually just a set of JapStrings
	// followed by a single EngString.  Currently,
	// all strings are only 2-byte aligned, so
	// only "entryIndex" crossed a byte boundary.
	unsigned int firstString;
};
//*/

struct FileSig {
	__int64 size;
	__int64 modTime;
	__int64 createTime;
};

struct DictionaryHeader {
	int magic;
	int version;
	FileSig srcSig;
	FileSig conjSig;

	//int numEntries;
	int numJStrings;

	unsigned int flags;
};

struct Dictionary {
	HANDLE hFile;
	DictionaryHeader *header;

	// int numEntries;
	int numJStrings;

	// DictionaryEntry *entries;

	// jStrings are a pre-sorted list of Japanese strings.
	unsigned int *jStrings;

	char *strings;
	wchar_t fileName[MAX_PATH];
};

void CleanupDict(Dictionary *dict) {
	UnmapViewOfFile(dict->header);
	CloseHandle(dict->hFile);
	free(dict);
}
struct VerbConjugation {
	// Index of string in tenses array.
	unsigned short tenseID;
	// If can be conjugated multiple times
	unsigned short nextVerbType;
	// Affirmative/negative/singular/plural
	unsigned char form;

	wchar_t suffix[2];
};

struct VerbType {
	char type[15];
	// 1 if adjective.
	unsigned char adj;
	int numConj;
	VerbConjugation **conj;
};

struct ConjugationTable {
	VerbType *verbTypes;
	wchar_t **tenses;
	int numVerbTypes;
	int numTenses;
	FileSig sig;
};

#define TENSE_REMOVE 0
#define TENSE_NON_PAST 1
#define TENSE_STEM 2
#define TENSE_POTENTIAL 3

const static wchar_t *staticTenses[] = {
	L"Remove",
	L"Non-past",
	L"Stem",
	L"Potential",
	L"Past",
	L"Te-form",
	L"Conditional",
	L"Provisional",
	L"Passive",
	L"Causative",
	L"Caus-Pass",
	L"Volitional",
	L"Conjectural",
	L"Adverbal",
	L"Alternative",
	L"Imperative",
	L"Imperfective",
	L"Continuative",
	L"Hypothetical",
	L"Prenominal",
	// L"Attributive",
	// to do accidentally/to finish completely
};

int numDicts = 0;
Dictionary **dicts = 0;

ConjugationTable *conjTable = 0;

Dictionary *LoadDict(wchar_t *path) {
	HANDLE hFile = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if (hFile == INVALID_HANDLE_VALUE) return 0;
	DWORD high;
	DWORD low = GetFileSize(hFile, &high);
	if ((low == INVALID_FILE_SIZE && GetLastError() != NO_ERROR) || (!high && low < sizeof(DictionaryHeader))) {
		CloseHandle(hFile);
		return 0;
	}
	HANDLE hMapping = CreateFileMapping(hFile, 0, PAGE_READONLY, high, low, 0);
	DictionaryHeader *d;
	size_t size = (size_t)(low + (((__int64)high)<<32));
	if (!hMapping || !(d = (DictionaryHeader*)MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, size))) {
		CloseHandle(hFile);
		return 0;
	}
	// Should be safe.
	CloseHandle(hMapping);
	Dictionary *dict = (Dictionary*) calloc(1, sizeof(Dictionary));
	dict->hFile = hFile;
	dict->header = d;
	if (d->magic != DICT_MAGIC || d->version != DICT_VERSION) {
		CleanupDict(dict);
		return 0;
	}
	wcscpy(dict->fileName, path);

	// dict->numEntries = dict->header->numEntries;
	dict->numJStrings = dict->header->numJStrings;

	// dict->entries = (DictionaryEntry*) (dict->header+1);
	dict->jStrings = (unsigned int*)(dict->header+1);
	dict->strings = (char*)(dict->jStrings + dict->numJStrings);
	void *start = (d+1);
	void *end = ((char*)d)+size;
	// Very minimal sanity check.
	if (/*dict->numEntries <= 0 ||*/ dict->numJStrings <= 0 ||
		/*dict->entries < start || dict->entries > end ||*/
		dict->jStrings < start || dict->jStrings > end ||
		dict->strings < start || dict->strings > end) {
			CleanupDict(dict);
			return 0;
	}
	return dict;
}

int __cdecl CompareWcharJap(const void* s1, const void* s2) {
	wchar_t *w1 = *(wchar_t**)s1;
	wchar_t *w2 = *(wchar_t**)s2;
	return wcsijcmp(w1, w2);
}

// Like wcstok, only more evil and stupid.
__forceinline wchar_t* __cdecl NextEvilString(wchar_t *s1, const wchar_t *delim) {
	static wchar_t *temp;
	if (s1) temp = s1;
	size_t len;
	while (1) {
		len = 0;
		while (1) {
			if (wcschr(delim, temp[len])) break;
			else if (temp[len] == '(') {
				while (temp[len] && temp[len]!=')') {
					len++;
				}
			}
			len++;
		}
		if (!len) {
			if (!*temp) return 0;
			temp++;
			continue;
		}

		wchar_t *out = temp;
		if (temp[len]) {
			temp[len] = 0;
			temp += len+1;
		}
		else temp+=len;
		return out;
	}
}

void RemoveEntLJunk(wchar_t *line) {
	while (line = wcsstr(line, L"/EntL")) {
		wchar_t *e = wcschr(line+5, '/');
		if (!e)
			break;
		/*if (e-line < 12 || e-line > 15) {
			line++;
			continue;
		}//*/
		wcscpy(line, e);
	}
}

int CreateDict(wchar_t *inPath, FileSig &srcSig, wchar_t *path) {
	wchar_t *data;
	int len;
	LoadFile(inPath, &data, &len);
	if (!data) return 0;
	if (len >= 4) {
		int i;
		for (i=0; i<4; i++) {
			if (data[i] != L'\xFF1F' && data[i] != L'\x3000')
				break;
		}
		if (i == 4 || 1) {
			//DictionaryEntry *entries = 0;
			//int numEntries = 0;
			//int maxEntries = 0;

			int *jStrings = 0;
			int numJStrings = 0;
			int maxJStrings = 0;

			char *strings = 0;
			int stringLen = 0;
			int maxStringLen = 0;

			wchar_t *line = data;
			//if (line) line++;
			while (1) {
				int lineLen = wcscspn(line, L"\r\n");
				if (!lineLen) break;
				wchar_t *next = line+lineLen;
				while (next[0] == '\n' || next[0] == '\r') {
					*next = 0;
					next++;
				}

				unsigned char flags = JAP_WORD_PRIMARY;
				unsigned char primaryFlags = 0;

				RemoveEntLJunk(line);

				if (line[0] == 31169) {
					line=line;
				}
				wchar_t *p = wcsstr(line, L"/(P)");
				if (p) {
					if (p+1 == wcsstr(line, L"(P)")) {
						flags |= JAP_WORD_COMMON;
					}
					else {
						primaryFlags |= JAP_WORD_COMMON;
					}
				}
				//hack - set custom word top priority flag (1)
				if (wcsstr(line, L"(T)")) {
					flags |= JAP_WORD_TOP;
					primaryFlags |= JAP_WORD_TOP;
				}
				//hackend

				/*
				if (numEntries == maxEntries) {
					maxEntries += maxEntries + 1024;
					entries = (DictionaryEntry*) realloc(entries, sizeof(*entries) * maxEntries);
				}//*/
				if (lineLen * 160 + stringLen >= maxStringLen) {
					maxStringLen += maxStringLen + 8096 + lineLen*160;
					strings = (char*) realloc(strings, maxStringLen);
				}

				wchar_t *jap = line;
				wchar_t *eng = wcschr(line, '/');
				if (jap && eng && jap[0] !=0x3000 && jap[0] != 0xFF1F) {
					*eng = 0;
					eng++;

					int engLen = wcslen(eng);

					// Need the open bracket.
					wchar_t *japString = NextEvilString(jap, L"; ]");
					if (engLen > 1 && japString) {
						if (eng[engLen-1] == '/') {
							eng[--engLen] = 0;
						}
						/*DictionaryEntry *entry = entries + numEntries;
						entry->firstString = stringLen;
						entry->numJStrings = 0;
						//*/

						WideCharToMultiByte(CP_UTF8, 0, eng, -1, strings+stringLen, maxStringLen-stringLen, 0, 0);
						POSData POS;
						GetPartsOfSpeech(strings+stringLen, &POS);
						if (POS.pos[POS_PART]) {
							flags |= JAP_WORD_PART;
						}

						int firstJStringPos = stringLen;
						int firstJString = numJStrings;
						unsigned int *lastFlags = 0;
						do {
							if (japString[0] == '[') {
								flags |= JAP_WORD_PRONOUNCE;
								flags &= ~JAP_WORD_PRIMARY;
								japString++;
							}
							if (numJStrings+20 >= maxJStrings) {
								maxJStrings += maxJStrings + 1024;
								jStrings = (int*) realloc(jStrings, sizeof(int) * maxJStrings);
							}
							JapString * jStringData = (JapString*) (strings+stringLen);
							jStrings[numJStrings++] = stringLen;
							//jStringData->entryIndex = numEntries;
							jStringData->startOffset = stringLen - firstJStringPos;
							jStringData->flags = flags;
							if (flags & JAP_WORD_PRIMARY) {
								jStringData->flags |= primaryFlags;
							}
							lastFlags = &jStringData->flags;
							jStringData->verbType = 0;

							// Remove extra annotations.
							int w = wcscspn(japString, L"(");
							if (japString[w]) {
								if (wcsstr(japString+w, L"(P)"))
									jStringData->flags |= JAP_WORD_COMMON;
								//hack - set custom word top priority flag (2)
								if (wcsstr(japString+w, L"(T)"))
									jStringData->flags |= JAP_WORD_TOP;
								//hackend
								japString[w] = 0;
							}

							wcscpy(jStringData->jstring, japString);
							int len = (int) wcslen(japString);
							stringLen += (len+1)*sizeof(wchar_t) + sizeof(JapString) - sizeof(jStringData->jstring);
							// entry->numJStrings ++;

							// Hack for the copula
							if (!POS.numVerbTypes && wcsstr(eng, L"plain copula") && !wcscmp(japString, L"\x3060")) {
								for (int i=0; i<conjTable->numVerbTypes; i++) {
									if (!stricmp(conjTable->verbTypes[i].type, "copula")) {
										POS.numVerbTypes ++;
										POS.verbTypes[0] = i;
										break;
									}
								}
							}

							// Really nasty verb conjugation stuff.
							for (int vindex=0; vindex<POS.numVerbTypes; vindex++) {
								char *typeString = conjTable->verbTypes[POS.verbTypes[vindex]].type;
								//for (int vt = POS.verbTypes[vindex]; vt<conjTable->numVerbTypes; vt++) {
								// Slightly slower, but needed for fix below.
								for (int vt = 0; vt<conjTable->numVerbTypes; vt++) {
									VerbType *type = conjTable->verbTypes+vt;
									if (strcmp(type->type, typeString)) {
										// Fix a couple dozen incorrectly annotated verbs.  Doesn't get them all, but gets a lot.
										// Seems to be only one verb I still have trouble with after this hack.
										if (strncmp(typeString, "v5",2) || strncmp(type->type, "v5",2) ||
											strlen(typeString) != strlen(type->type)) {
												continue;
										}
									}
									int match = -1;
									int foundRemove = 0;
									for (int cj=0; cj<type->numConj; cj++) {
										VerbConjugation * conj = type->conj[cj];
										if (conj->tenseID == TENSE_REMOVE) {
											if (!foundRemove) {
												foundRemove = 1;
												match = -1;
											}
										}
										else if (conj->tenseID == TENSE_NON_PAST && conj->form == 0) {
											if (foundRemove) continue;
										}
										else continue;
										int sufLen = (int)wcslen(conj->suffix);
										if (sufLen > len) continue;
										if (wcsijcmp(conj->suffix, japString+len-sufLen)) {
											continue;
										}
										match = cj;
									}
									if (match < 0) continue;

									VerbConjugation * conj = type->conj[match];
									int sufLen = (int)wcslen(conj->suffix);
									int js;
									JapString* jStringData2;
									for (js=firstJString; js<numJStrings; js++) {
										jStringData2 = (JapString*)(strings+js);
										// Sometimes two different initial strings result in identical conjugations.  Not too common.
										if (jStringData2->verbType != vt+1 || wcslen(jStringData2->jstring) != len-sufLen || !wcsnijcmp(jStringData2->jstring, japString, len-sufLen)) continue;
									}
									if (js < numJStrings) break;

									jStringData2 = (JapString*) (strings+stringLen);
									jStrings[numJStrings++] = stringLen;

									*jStringData2 = *jStringData;
									jStringData2->verbType = vt+1;
									wcscpy(jStringData2->jstring, japString);
									jStringData2->jstring[len-sufLen] = 0;
									jStringData2->startOffset = stringLen - firstJStringPos;

									stringLen += (len+1-sufLen)*sizeof(wchar_t) + sizeof(JapString) - sizeof(jStringData2->jstring);
									// entry->numJStrings ++;

									lastFlags = &jStringData2->flags;

									break;
								}
							}
						}
						while (japString = NextEvilString(0, L"; ]"));
						lastFlags[0] |= JAP_WORD_FINAL;
						int elen = WideCharToMultiByte(CP_UTF8, 0, eng, -1, strings+stringLen, maxStringLen-stringLen, 0, 0);

						stringLen += (elen+1)&~1;
					}
				}
				line = next;
				// numEntries++;
			}
			free(data);

			wchar_t **SortedStrings = (wchar_t**) malloc(sizeof(wchar_t*) * numJStrings);
			for (int i=0; i<numJStrings; i++) {
				SortedStrings[i] = ((JapString*)(strings + jStrings[i]))->jstring;
			}
			qsort(SortedStrings,numJStrings,sizeof(wchar_t*),CompareWcharJap);
			// Needed for sizeof operator.
			JapString * jStringData = 0;
			for (int i=0; i<numJStrings; i++) {
				jStrings[i] = (int) (((char*) SortedStrings[i])-strings) - (sizeof(JapString) - sizeof(jStringData->jstring));
			}
			free(SortedStrings);

			DictionaryHeader header;
			header.magic = DICT_MAGIC;
			header.version = DICT_VERSION;
			header.srcSig = srcSig;
			header.conjSig = conjTable->sig;
			// header.numEntries = numEntries;
			header.numJStrings = numJStrings;
			header.flags = 0;
			wchar_t *fname = wcsrchr(inPath, '\\');

			if (fname) fname++;
			else fname = inPath;

			if (!wcsnicmp(fname, L"enam", 4)) {
				header.flags |= DICT_FLAG_NAMES;
			}

			HANDLE hFile = CreateFile(path, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
			if (hFile != INVALID_HANDLE_VALUE) {
				DWORD junk;
				WriteFile(hFile, &header, sizeof(header), &junk, 0);
				//WriteFile(hFile, entries, sizeof(entries[0]) * numEntries, &junk, 0);
				WriteFile(hFile, jStrings, sizeof(jStrings[0]) * numJStrings, &junk, 0);
				WriteFile(hFile, strings, stringLen, &junk, 0);
				CloseHandle(hFile);
			}

			// free(entries);
			free(strings);
			free(jStrings);
			return 1;
		}
	}
	free(data);
	return 0;
}

void DictCheck() {
	CreateDirectoryW(L"dictionaries",0);
	WIN32_FIND_DATAW data;
	HANDLE hFind = FindFirstFileW(L"dictionaries\\*", &data);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
			wchar_t *ext = wcschr(data.cFileName, '.');
			if (ext && wcsicmp(ext, L".gz")) continue;
			wchar_t srcPath[MAX_PATH*2], dstPath[MAX_PATH*2];
			wsprintfW(srcPath, L"dictionaries\\%s", data.cFileName);
			wsprintfW(dstPath, L"dictionaries\\%s.bin", data.cFileName);

			FileSig srcSig;
			srcSig.createTime = *(__int64*)&data.ftCreationTime;
			srcSig.modTime = *(__int64*)&data.ftLastWriteTime;
			srcSig.size = data.nFileSizeLow + (((__int64)data.nFileSizeHigh)<<32);

			int i;
			for (i=0; i<numDicts; i++) {
				if (!wcsicmp(dicts[i]->fileName, dstPath)) break;
			}
			if (i < numDicts) {
				Dictionary *dict = dicts[i];
				if (srcSig.createTime == dict->header->srcSig.createTime &&
					srcSig.modTime == dict->header->srcSig.modTime &&
					srcSig.size == dict->header->srcSig.size &&
					conjTable->sig.createTime == dict->header->conjSig.createTime &&
					conjTable->sig.modTime == dict->header->conjSig.modTime &&
					conjTable->sig.size == dict->header->conjSig.size) {
						// Up to date.
						continue;
				}
				CleanupDict(dict);
				dicts[i] = dicts[--numDicts];
			}
			CreateDict(srcPath, srcSig, dstPath);
			dicts = (Dictionary**) realloc(dicts, sizeof(Dictionary*) * (numDicts+1));
			if (dicts[numDicts] = LoadDict(dstPath)) {
				numDicts++;
			}
		}
		while (FindNextFileW(hFind, &data));
		FindClose(hFind);
	}
}


int LoadConjugationTable() {
	wchar_t *data;
	int size;
	LoadFile(L"dictionaries\\Conjugations.txt", &data, &size);
	if (!data) return 0;
	wchar_t *d = data;

	ConjugationTable table = {0};

	WIN32_FILE_ATTRIBUTE_DATA fileInfo;
	GetFileAttributesExW(L"dictionaries\\Conjugations.txt", GetFileExInfoStandard, &fileInfo);
	table.sig.createTime = *(__int64*)&fileInfo.ftCreationTime;
	table.sig.modTime = *(__int64*)&fileInfo.ftLastWriteTime;
	table.sig.size = fileInfo.nFileSizeLow + (((__int64)fileInfo.nFileSizeHigh)<<32);

	VerbType *type = 0;

	int totalConjSize = 0;
	int totalNumConj = 0;
	int totalTenseSize = 0;

	table.tenses = (wchar_t**) malloc(256 * sizeof(wchar_t*));
	for (int i=0; i<sizeof(staticTenses)/sizeof(staticTenses[0]); i++) {
		table.tenses[table.numTenses++] = wcsdup(staticTenses[i]);
		totalTenseSize += sizeof(wchar_t*) + sizeof(wchar_t) * (1+wcslen(staticTenses[i]));
	}

	while (*d) {
		int endPos = wcscspn(d, L"\r\n");
		if (!endPos) {
			d++;
			continue;
		}
		if (d[endPos]) {
			d[endPos] = 0;
			endPos++;
		}
		wchar_t *strings[5] = {0,0,0,0,0};
		strings[0] = d;
		int i = 1;
		while (i<5 && strings[i-1][0] && (strings[i] = wcschr(strings[i-1], '\t'))) {
			strings[i][0] = 0;
			strings[i]++;
			i++;
		}
		for (int j=0; j<i; j++) {
			// Remove any extra whitespace.
			while (strings[j][0] == ' ') strings[j]++;
			wchar_t *q = wcschr(strings[j], 0);
			while (q > strings[j] && q[-1] == ' ') {
				q--;
				q[0] = 0;
			}
		}
		if (strings[0] && strings[0][0] && strings[0][0] != '/') {
			if (!wcscmp(strings[0], L"Verb") || !wcscmp(strings[0], L"Adj") && strings[1] && strings[1][0]) {
				if (table.numVerbTypes % 256 == 0) {
					table.verbTypes = (VerbType*) realloc(table.verbTypes, sizeof(VerbType) * (256 + table.numVerbTypes));
				}
				type = table.verbTypes + table.numVerbTypes;
				memset(type, 0, sizeof(*type));
				if (!wcscmp(strings[0], L"Adj")) type->adj = 1;
				table.numVerbTypes++;
				WideCharToMultiByte(CP_UTF8, 0, strings[1], -1, type->type, sizeof(type->type), 0, 0);
			}
			else if (type) {
				int tense;
				for (tense=0; tense<table.numTenses; tense++) {
					if (!wcsicmp(table.tenses[tense], strings[0])) {
						break;
					}
				}
				if (tense == table.numTenses) {
					if (table.numTenses % 256 == 0)
						table.tenses = (wchar_t**) realloc(table.tenses, sizeof(wchar_t*) * (256 + table.numTenses));
					table.tenses[table.numTenses++] = wcsdup(strings[0]);
					totalTenseSize += sizeof(wchar_t*) + sizeof(wchar_t) * (1+wcslen(strings[0]));
				}
				for (i=0; i<4; i++) {
					wchar_t *s = strings[i+1];
					if (s && s[0]) {
						if (s[0] == ',') {
							s++;
						}
						if (s[0]) {
							s = mywcstok(s, L" ,");
						}
						else {
							// prevents crash on next mywcstok.
							mywcstok(s, L" ,");
						}
						do {
							type->conj = (VerbConjugation**)realloc(type->conj, sizeof(type->conj[0]) * (type->numConj+1));
							VerbConjugation **conj = type->conj + type->numConj;
							int size = sizeof(VerbConjugation) - sizeof(conj[0]->suffix) + (wcslen(s) + 1)*sizeof(wchar_t);
							conj[0] = (VerbConjugation*) calloc(1, size);
							totalConjSize += size;
							conj[0]->tenseID = tense;
							conj[0]->form = i;
							wcscpy(conj[0]->suffix, s);

							totalNumConj++;
							type->numConj++;
						}
						while (s = mywcstok(0, L" ,"));
					}
				}
			}
		}
		d += endPos;
	}
	free(data);

	// Handle stacking verb forms.
	for (int vt=0; vt<table.numVerbTypes; vt++) {
		VerbType *type = table.verbTypes+vt;
		for (int ct=0; ct<type->numConj; ct++) {
			VerbConjugation * conj = type->conj[ct];
			int e = wcscspn(conj->suffix, L"(");
			if (conj->suffix[e]) {
				int n = e;
				while (n > 0 && conj->suffix[n-1]==' ') n--;
				e++;
				conj->suffix[e+wcscspn(conj->suffix+e, L")")] = 0;
				char type2String[50];
				if (WideCharToMultiByte(CP_UTF8, 0, conj->suffix+e, -1, type2String, 50, 0, 0)) {
					for (int vt2=0; vt2<table.numVerbTypes && !conj->nextVerbType; vt2++) {
						VerbType *type2 = table.verbTypes+vt2;
						if (stricmp(type2->type, type2String)) continue;
						int removeFound = 0;
						for (int ct2=0; ct2<type2->numConj; ct2++) {
							VerbConjugation * conj2 = type2->conj[ct2];
							if (conj2->tenseID == TENSE_REMOVE) {
								removeFound = 1;
								break;
							}
						}
						for (int ct2=0; ct2<type2->numConj; ct2++) {
							VerbConjugation * conj2 = type2->conj[ct2];
							if (conj2->tenseID != TENSE_REMOVE && (removeFound || conj2->tenseID != TENSE_NON_PAST)) continue;
							int len = (int)wcslen(conj2->suffix);
							if (len > n) continue;
							if (!wcsnijcmp(conj->suffix+n-len, conj2->suffix, len)) {
								n -= len;
								conj->nextVerbType = vt2+1;
								break;
							}
						}
					}
				}

				totalConjSize -= sizeof(wchar_t) * wcslen(conj->suffix+n);
				conj->suffix[n] = 0;
			}
		}
	}

	int memSize = totalTenseSize + totalConjSize + totalNumConj * sizeof(VerbConjugation*) + sizeof(VerbType) * table.numVerbTypes + sizeof(ConjugationTable);
	conjTable = (ConjugationTable*) calloc(1, memSize);
	*conjTable = table;
	conjTable->verbTypes = (VerbType*)(conjTable+1);
	conjTable->numVerbTypes = table.numVerbTypes;
	VerbConjugation **conjPtrs = (VerbConjugation **) (conjTable->verbTypes + conjTable->numVerbTypes);
	conjTable->tenses = (wchar_t**) (conjPtrs + totalNumConj);
	wchar_t *tenseOut = (wchar_t*)(conjTable->tenses + table.numTenses);
	VerbConjugation *conj = (VerbConjugation*) (((char*)conjTable->tenses) + totalTenseSize);

	for (int i=0; i<table.numTenses; i++) {
		conjTable->tenses[i] = tenseOut;
		wcscpy(tenseOut, table.tenses[i]);
		free(table.tenses[i]);
		tenseOut += 1 + wcslen(tenseOut);
	}
	free(table.tenses);

	for (int i=0; i<table.numVerbTypes; i++) {
		VerbType *typed = conjTable->verbTypes+i;
		VerbType *types = table.verbTypes+i;
		*typed = *types;
		typed->conj = conjPtrs;
		conjPtrs += typed->numConj;
		for (int j=0; j<types->numConj; j++) {
			int size = sizeof(VerbConjugation) - sizeof(conj->suffix) + (1+wcslen(types->conj[j]->suffix)) * sizeof(wchar_t);
			memcpy(conj, types->conj[j], size);
			typed->conj[j] = conj;
			conj = (VerbConjugation *)(((char*)conj) + size);
			free(types->conj[j]);
		}
		free(types->conj);
	}
	free(table.verbTypes);
	return 1;
}

void LoadDicts() {
	if (!numDicts) {
		// Not really needed, but frees up old conjugation table.
		CleanupDicts();
		if (!LoadConjugationTable()) return;

		WIN32_FIND_DATAW data;
		HANDLE hFind = FindFirstFileW(L"dictionaries\\*.bin", &data);
		if (hFind != INVALID_HANDLE_VALUE) {
			do {
				if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
				wchar_t path[MAX_PATH*2];
				wsprintfW(path, L"dictionaries\\%s", data.cFileName);
				Dictionary *dict = LoadDict(path);
				if (!dict) continue;
				dicts = (Dictionary**) realloc(dicts, sizeof(Dictionary*) * (numDicts+1));
				dicts[numDicts++] = dict;
			}
			while (FindNextFileW(hFind, &data));
			FindClose(hFind);
		}
	}
	DictCheck();
}

void CleanupDicts() {
	free(conjTable);
	conjTable = 0;

	for (int i=0; i<numDicts; i++) {
		CleanupDict(dicts[i]);
	}
	free(dicts);
	dicts = 0;
	numDicts = 0;
}

JapString *GetJapString(Dictionary *dict, int mid) {
	return (JapString*)(dict->strings + dict->jStrings[mid]);
}

int FindVerbMatches(wchar_t *string, int slen, int vtype, Match* &matches, int &numMatches, int depth, int inexact) {
	VerbType *type = &conjTable->verbTypes[vtype-1];
	int added = 0;
	for (int cj=0; cj<type->numConj; cj++) {
		VerbConjugation *conj = type->conj[cj];
		if (conj->tenseID == TENSE_REMOVE) continue;
		int suffixLen = wcslen(conj->suffix);
		if (!wcsnijcmp(conj->suffix, string+slen, suffixLen)) {
			int inexact2 = inexact;
			if (wcsnicmp(conj->suffix, string+slen, suffixLen))
				inexact2 = 1;
			if (!conj->nextVerbType) {
				if (numMatches % 32 == 0) {
					matches = (Match*) realloc(matches, sizeof(Match) * (numMatches + 32));
				}
				memset(matches[numMatches].conj, 0, sizeof(matches[numMatches].conj));
				matches[numMatches].conj[depth].verbType = vtype;
				matches[numMatches].conj[depth].verbForm = conj->form;
				matches[numMatches].conj[depth].verbTense = conj->tenseID;
				matches[numMatches].conj[depth].verbConj = cj;
				matches[numMatches].inexactMatch = inexact2;
				matches[numMatches].len = slen + suffixLen;

				numMatches++;
				added++;
			}
			else {
				// If not first, don't count stems towards limit, and don't add to list.
				if (depth && conj->tenseID == TENSE_STEM && !conj->form) {
					added += FindVerbMatches(string, slen+suffixLen, conj->nextVerbType, matches, numMatches, depth, inexact2);
				}
				else {
					if (depth < MAX_CONJ_DEPTH-1) {
						int newlyAdded = FindVerbMatches(string, slen+suffixLen, conj->nextVerbType, matches, numMatches, depth+1, inexact2);
						for (int m = numMatches-newlyAdded; m<numMatches; m++) {
							if (matches[m].conj[depth+1].verbTense == conj->tenseID) {
								/* Remove the lovely "Potential Potential" results.
								 */
								if (conj->tenseID == TENSE_POTENTIAL) {
									matches[m] = matches[numMatches-1];
									newlyAdded--;
									numMatches--;
									m--;

									continue;
								}
							}
							matches[m].conj[depth].verbType = vtype;
							matches[m].conj[depth].verbForm = conj->form;
							matches[m].conj[depth].verbConj = cj;
							matches[m].conj[depth].verbTense = conj->tenseID;
						}
						added += newlyAdded;
					}
				}
			}
		}
	}
	return added;
}

void FindMatches(wchar_t *string, int stringLen, Match *&matches, int &numMatches) {
	matches = 0;
	numMatches = 0;
	for (int i=0; i<numDicts; i++) {
		Dictionary *dict = dicts[i];
		int first = 0;
		int last = dict->numJStrings-1;
		// len 0 is for verbs which have 0 characters after removing the suffix.
		int len = 0;

		int min = first;
		int max = last;
		JapString *jap;
		// Find the last possible match.
		// Makes searches a bit faster, as I only have to look at one character at a time and
		// have to search less stuff at each pass.
		while (min < max) {
			int mid = (min+max+1)/2;
			jap = GetJapString(dict, mid);
			int cmp = wcsijcmp(string, jap->jstring);
			if (cmp >= 0) min = mid;
			else if (cmp < 0) max = mid-1;
		}
		last = max;
		first = 0;

		while (1) {
			if (len > stringLen) break;

			if (len) {
				int min = first;
				int max = last;
				JapString *jap;
				while (min < max) {
					int mid = (min+max)/2;
					jap = GetJapString(dict, mid);
					// All magically long enough for this and first len-1 characters all match.
					int cmp = wcsnijcmp(string+len-1, jap->jstring+len-1, 1);
					if (cmp > 0) min = mid+1;
					else if (cmp <= 0) max = mid;
				}
				first = min;
			}

			while (first <= last) {
				jap = GetJapString(dict, first);
				int slen = (int) wcslen(jap->jstring);
				int cmp = wcsnijcmp(string, jap->jstring, slen);
				first++;
				int inexactMatch = wcsnicmp(string, jap->jstring, slen);
				if (!cmp) {
					int j;

					if (!jap->verbType) {
						if (numMatches % 32 == 0) {
							matches = (Match*) realloc(matches, sizeof(Match) * (numMatches + 32));
						}
						memset(matches[numMatches].conj, 0, sizeof(matches[numMatches].conj));
						matches[numMatches].jString = jap;
						matches[numMatches].firstJString = (JapString*)(((char*)jap) - jap->startOffset);
						matches[numMatches].inexactMatch = inexactMatch;
						matches[numMatches].srcLen = matches[numMatches].len = slen;
						matches[numMatches].start = 0;
						matches[numMatches].japFlags = jap->flags;
						matches[numMatches].dictIndex = i;
						matches[numMatches].matchFlags = 0;
						if (dict->header->flags & DICT_FLAG_NAMES) {
							matches[numMatches].matchFlags |= MATCH_IS_NAME;
						}
						for (j=0; j<numMatches; j++) {
							if (!memcmp(matches+numMatches, matches+j, sizeof(Match)-sizeof(int))) {
								// If have exact and inexact matches, exact overwrite inexact.
								if (!matches[numMatches].inexactMatch)
									matches[j] = matches[numMatches];
								break;
							}
						}
						if (j == numMatches && (!inexactMatch || !(dict->header->flags & DICT_FLAG_NAMES)))
							numMatches++;
					}
					else {
						int addedMatches = FindVerbMatches(string, slen, jap->verbType, matches, numMatches, 0, inexactMatch);
						if (addedMatches) {
							for (int m = numMatches-addedMatches; m<numMatches; m++) {
								matches[m].jString = jap;
								matches[m].firstJString = (JapString*)(((char*)jap) - jap->startOffset);
								matches[m].inexactMatch |= inexactMatch;
								matches[m].srcLen = slen;
								matches[m].start = 0;
								matches[m].japFlags = jap->flags;
								matches[m].dictIndex = i;
								matches[m].matchFlags = 0;

								for (j=0; j<m; j++) {
									if (!memcmp(matches+m, matches+j, sizeof(Match)-sizeof(int))) {
										// If have exact and inexact matches, exact overwrite inexact.
										if (!matches[m].inexactMatch) {
											matches[j] = matches[m];
										}
										numMatches--;
										m--;
										break;
									}
								}
							}
						}
					}
				}
				else if (cmp > 0) {
					break;
				}
				else {
					// Minor optimization.
					while (len < slen && len < stringLen && !wcsnijcmp(string+len, jap->jstring+len, 2)) {
						len++;
					}
					break;
				}
			}
			if (first > last) break;
			len++;
		}
	}
}

void FindAllMatches(wchar_t *string, int len, Match *&matches, int &numMatches) {
	matches = 0;
	numMatches = 0;
	int maxMatches = 0;
	for (int i=0; i<len; i++) {
		Match *matches2;
		int numMatches2;
		FindMatches(string+i, len-i, matches2, numMatches2);
		for (int j=0; j<numMatches2; j++) {
			matches2[j].start = i;
		}
		if (numMatches + numMatches2 > maxMatches) {
			maxMatches += 1024 + numMatches2;
			matches = (Match*)realloc(matches, sizeof(Match)*maxMatches);
		}
		memcpy(matches+numMatches, matches2, numMatches2*sizeof(Match));
		numMatches += numMatches2;
		free(matches2);
	}
}

struct BestMatchInfo {
	// -2 means no match/failure, should only have it on first position.  -1 means that
	// residue matches nothing, go to previous.  Could just use the same value for both...
	int matchIndex;
	// redundant, so not really needed.
	int matchLen;
	int score;
};

// Used to sync up inexactMatch value of different matches to same word.
// should only be an issue with verb conjugations.
int __cdecl CompareIdenticalMatches(const void *v1, const void *v2) {
	Match *m1 = (Match*) v1;
	Match *m2 = (Match*) v2;
	if (m1->start != m2->start) {
		return m1->start - m2->start;
	}
	if (m1->dictIndex != m2->dictIndex) {
		return m1->dictIndex - m2->dictIndex;
	}
	if (m1->firstJString != m2->firstJString) {
		return m1->firstJString - m2->firstJString;
	}
	return (m1->conj[0].verbType - (int)m2->conj[0].verbType) * (1<<16) + (m1->conj[0].verbTense - (int)m2->conj[0].verbTense) * 4 + m1->conj[0].verbForm - m2->conj[0].verbForm;
}

int __cdecl CompareMatches(const void *v1, const void *v2) {
	Match *m1 = (Match*) v1;
	Match *m2 = (Match*) v2;
	if (m1->start != m2->start) {
		return m1->start - m2->start;
	}
	if (m1->inexactMatch != m2->inexactMatch) {
		// Could theoretically have inexact and exact matches for the same verb or adjective, unless fixed first.
		return m1->inexactMatch - m2->inexactMatch;
	}
	int name = (m1->matchFlags & MATCH_IS_NAME) - (m2->matchFlags & MATCH_IS_NAME);
	if (name) return name;

	//hack - ???
	int mask = JAP_WORD_TOP | JAP_WORD_COMMON | JAP_WORD_PART | JAP_WORD_PRIMARY;
	//int mask = JAP_WORD_COMMON | JAP_WORD_PART | JAP_WORD_PRIMARY;
	//hackend
	int flagDiff = (m2->japFlags & mask) - (m1->japFlags & mask);
	if (flagDiff) return flagDiff;
	// May do something better later.
	if (m1->dictIndex != m2->dictIndex) {
		return m2->dictIndex - m1->dictIndex;
	}
	if (m1->firstJString != m2->firstJString) {
		return m2->firstJString - m1->firstJString;
	}
	return m1->conj[0].verbForm - m2->conj[0].verbForm;
}

void SortMatches(Match *&matches, int &numMatches) {
	qsort(matches, numMatches, sizeof(Match), CompareIdenticalMatches);
	for (int i=1; i<numMatches; i++) {
		int j = i;
		while (j && matches[j].dictIndex == matches[j-1].dictIndex &&
					matches[j].firstJString == matches[j-1].firstJString &&
					matches[j].start == matches[j-1].start &&
					matches[j].len == matches[j-1].len &&
					matches[j].inexactMatch != matches[j-1].inexactMatch) {
						matches[j].inexactMatch = matches[j-1].inexactMatch = 0;
						j--;
		}
		// If have multiple matches using the same conjugation of different base verb forms,
		// remove duplicates.  Not sure how likely this is.  If any adj-na's have both
		// purely hiragana and katakana entries, they could result in this.
		if (!memcmp(&matches[i], &matches[i-1], sizeof(Match))) {
			numMatches--;
			memmove(matches+i, matches+i+1, sizeof(Match)*(numMatches - i));
			i--;
			continue;
		}
		if (matches[i].dictIndex == matches[i-1].dictIndex &&
			matches[i].firstJString == matches[i-1].firstJString &&
			matches[i].start == matches[i-1].start &&
			matches[i].len == matches[i-1].len &&
			matches[i].conj[0].verbType && !matches[i-1].conj[0].verbType &&
			matches[i].conj[0].verbForm == 0 && matches[i].conj[0].verbTense == TENSE_NON_PAST &&
				(i+1 == numMatches ||
				matches[i+1].dictIndex != matches[i].dictIndex ||
				 matches[i+1].firstJString != matches[i].firstJString ||
				 matches[i+1].len != matches[i-1].len ||
				 matches[i+1].start != matches[i-1].start)) {
					numMatches--;
					memmove(matches+i, matches+i+1, sizeof(Match)*(numMatches - i));
					i--;
		}
	}
	qsort(matches, numMatches, sizeof(Match), CompareMatches);
}

#define MECAB_BAD_END	0x01
#define MECAB_BAD_START	0x02

void FindBestMatches(wchar_t *string, int len, Match *&matches, int &numMatches, int useMecab) {
	FindAllMatches(string, len, matches, numMatches);
	if (!numMatches) return;
	BestMatchInfo *best = (BestMatchInfo*) malloc(sizeof(BestMatchInfo) * (len+1) + sizeof(char) * len);

	unsigned char *posFlags = (unsigned char*) (best + len + 1);
	memset(posFlags, 0, sizeof(char) * len);
	wchar_t *mecab;
	if (useMecab && (mecab = MecabParseString(string, len, 0))) {
		int pos = 0;
		wchar_t *line = wcstok(mecab, L"\r\n");
		while (line) {
			if (wcscmp(line, L"EOS")) {
				wchar_t *word = line, *end;
				if (word && (end = wcschr(word, '\t'))) {
					*end = 0;
					int matchLen = 0;
					int oldPos = pos;
					while (string[pos]) {
						if (!word[matchLen]) break;
						if (string[pos] == word[matchLen]) {
							matchLen++;
						}
						pos++;
					}
					if (!word[matchLen] && !wcsnijcmp(&string[pos-matchLen], word, matchLen)) {
						wchar_t *srcWord = end+1;
						// If katakana is '*' or does not exist, not real word, so don't penalize.
						for (int i=0; i<7; i++) {
							if (srcWord) srcWord = wcschr(srcWord, ',');
							if (!srcWord) break;
							srcWord++;
						}
						if (srcWord && srcWord[0] != '*' && srcWord[0] != ',') {
							for (int i=0; i<matchLen-1; i++) {
								posFlags[pos-matchLen + i] |= MECAB_BAD_END;
								posFlags[pos-matchLen + i + 1] |= MECAB_BAD_START;
							}
						}
					}
					else {
						// I don't trust mecab all that much.
						pos = oldPos;
					}
				}
			}
			line = wcstok(0,  L"\r\n");
		}
		free(mecab);
	}

	// Note:  High score is bad, low is good.  Currently doesn't have to be signed,
	// but if I add enough bonuses, that could change.
	best[0].score = 0;
	best[0].matchLen = 0;
	best[0].matchIndex = -2;
	for (int i=1; i <= len; i++) {
		// Worst possible score.
		best[i].score = 0x7FFFFFFF;
		// Don't really have to be initialized, but can't hurt.
		best[i].matchLen = 0;
		best[i].matchIndex = -2;
	}
	int pos = 0;
	int matchIndex = 0;
	while (pos < len) {
		// Calculate score if current character is not in a match.
		// Note that I store the score up to and including the pos character
		// in best[pos+1].  As matches are sorted by first, not last, character,
		// I do things out of order, so need the first check.
		int score = best[pos].score + 100;
		// Kanji.
		if (string[pos] >= 0x4E00 && string[pos] <= 0x9FBF)
			score += 400;
		int nextPos = pos+1;
		if (best[nextPos].score > score) {
			best[nextPos].score = score;
			best[nextPos].matchIndex = -1;
			best[nextPos].matchLen = 0;
		}

		while (matchIndex < numMatches && matches[matchIndex].start <= pos) {
			Match *match = matches + matchIndex;
			int score = best[pos].score + 10;
			if (posFlags[pos] & MECAB_BAD_START) {
				score += 10;
			}
			if (posFlags[pos + matches[matchIndex].len-1] & MECAB_BAD_END) {
				score += 10;
			}
			// Favor breaking words around particles, though not as much as entries with combined particles
			// and neighboring words.
			if (match->japFlags & JAP_WORD_PART) {
				score -= 2;
			}
			// Hack to discourage things like matching hiragana "ku" alone...
			else if (match->len == 1) {
				score += 1;
			}
			// Slightly favor common words.  Bonus is so small because common word annotations don't seem to
			// be all that good.
			if (match->japFlags & JAP_WORD_COMMON) {
				score -= 3;
			}
			// Penalize for inexact matches (Hiragana/Katakana matched to each other).
			if (match->inexactMatch) {
				score += 10;
			}
			if (dicts[match->dictIndex]->header->flags & DICT_FLAG_NAMES) {
				int mad = match->inexactMatch;
				if (!mad) {
					wchar_t *base = string + matches[matchIndex].start;
					for (int i=0; i<matches[matchIndex].len; i++) {
						if (!IsKatakana(base[i])) {
							mad = 1;
							break;
						}
					}
					if (matches[matchIndex].start && IsKatakana(base[-1]))
						mad = 1;
					int last = matches[matchIndex].start+matches[matchIndex].len;
					if (last < len && IsKatakana(base[last]))
						mad = 1;
				}
				if (mad) {
					score += 500 * match->len;
				}
				else {
					score += 5;
				}
			}

			//hack - set custom word top priority score
			if (match->japFlags & JAP_WORD_TOP) {
				//Beep(500,50);
				score = -999999;
			}
			//hackend
			/*
			// Bonus for exactly matching a primary entry - basically discourage
			// Hiragana entries for entries with kanji, in favor of words with
			// no Kanji.  Helps with some long runs or hiragana.
			if (match->flags & JAP_WORD_PRIMARY) {
				score -= 1;
			}//*/

			int nextPos = pos + match->len;
			if (best[nextPos].score >= score) {
				best[nextPos].score = score;
				best[nextPos].matchLen = match->len;
				best[nextPos].matchIndex = matchIndex;
			}
			matchIndex++;
		}

		pos ++;
	}

	// Find all matches that align with the start/stop positions of the best scoring
	// set of matches, even if they didn't contribute to the score.
	int index = numMatches-1;
	int outIndex = index;
	while (pos > 0) {
		if (best[pos].matchIndex < 0) {
			while (pos > 0 && best[pos].matchIndex < 0) {
				pos--;
			}
			continue;
		}
		int start = pos-best[pos].matchLen;
		int len = best[pos].matchLen;
		while (index >= 0 && matches[index].start >= start) {
			if (matches[index].start == start && matches[index].len == len) {
				matches[outIndex--] = matches[index];
			}
			index--;
		}
		pos = start;
	}
	free(best);
	numMatches = numMatches-1 - outIndex;
	memmove(matches, matches+outIndex+1, sizeof(Match) * numMatches);
	matches = (Match*) realloc(matches, sizeof(Match) * numMatches);
}

int DictsLoaded() {
	LoadDicts();
	return numDicts;
}

int GetDictEntry(Match &match, EntryData *out) {
	if (!GetDictEntry(match.dictIndex, match.firstJString, out)) return 0;
	if (match.conj[0].verbType) {
		VerbType *type = conjTable->verbTypes+match.conj[0].verbType-1;
		VerbConjugation *conj = type->conj[match.conj[0].verbConj];
		wchar_t *suffix = conj->suffix;
		if (suffix[0] >= 0x4E00 && suffix[0] <= 0x9FBF) {
			int len = (int)wcslen(suffix);
			for (int i=0; i<conjTable->numVerbTypes; i++) {
				VerbType *type2 = conjTable->verbTypes+i;
				if (stricmp(type2->type, type->type) || type2 == type) continue;
				int best = 10000;
				wchar_t *bestString = 0;
				for (int j=0; j<type2->numConj; j++) {
					VerbConjugation *conj2 = type2->conj[j];
					if (conj->tenseID != conj2->tenseID || conj->form != conj2->form || conj->nextVerbType != conj2->nextVerbType) continue;
					int len2 = (int)wcslen(conj2->suffix);
					if (len2 < len || len2 >= best) continue;
					if (!wcsnijcmp(suffix+1, conj2->suffix + len2-len+1, len-1)) {
						best = len2;
						bestString = conj2->suffix;
					}
				}
				if (bestString) {
					int lenWant = wcslen(bestString)-len+1;
					if (lenWant <= 3) {
						memcpy(out->kuruHack, bestString, sizeof(wchar_t)*lenWant);
						out->kuruHack[lenWant] = 0;
						break;
					}
				}
			}
			type=type;
		}
	}
	return 1;
}

int GetDictEntry(int dictIndex, JapString *jap, EntryData *out) {
	if (dictIndex >= numDicts || dictIndex<0) return 0;
	out->kuruHack[0] = 0;
	JapString *jap2 = (JapString*)(((char*)jap) - jap->startOffset);
	int i=0;
	while (1) {
		int flags = jap2->flags;
		if (i<256) {
			out->numJap = i+1;
			out->jap[i] = jap2;
		}
		i++;
		jap2 = (JapString*) (((char*)jap2) + sizeof(JapString) - sizeof(jap2->jstring) + sizeof(wchar_t) * (wcslen(jap2->jstring)+1));
		if (flags & JAP_WORD_FINAL) break;
	}
	out->english = (char*) jap2;
	return 1;
}

void FindExactMatches(wchar_t *string, int len, Match *&matches, int &numMatches) {
	FindMatches(string, len, matches, numMatches);
	if (numMatches) {
		int p1 = 0;
		for (int p2=0; p2<numMatches; p2++) {
			if (matches[p2].len == len) {
				matches[p1++] = matches[p2];
			}
		}
		numMatches = p1;
		matches = (Match*) realloc(matches, numMatches*sizeof(Match));
	}
}

struct POSList {
	char name[7];
	unsigned char pos;
};

void GetPartsOfSpeech(char *eng, POSData *pos) {
	static const POSList posList[] = {
		"prt", POS_PART,
	};
	memset(pos, 0, sizeof(*pos));
	if (eng[0] == '(') {
		char *start = eng+1;
		while (1) {
			while (*start == ',' || *start==' ' || *start==';') start++;
			if (*start ==')') break;
			char *end = start;
			while (*end != ',' && *end!=' ' && *end!=';' && *end!=')' && *end) end++;
			int len = (int)(end-start);
			int i;
			for (i=0; i<sizeof(posList)/sizeof(posList[0]); i++) {
				if (!strncmp(posList[i].name, start, len) && len == strlen(posList[i].name)) {
					pos->pos[posList[i].pos] = 1;
					break;
				}
			}
			if (i == sizeof(posList)/sizeof(posList[0])) {
				for (i=0; i<conjTable->numVerbTypes; i++) {
					if (!strncmp(conjTable->verbTypes[i].type, start, len) && len == strlen(conjTable->verbTypes[i].type)) {
						if (!pos->pos[POS_LINEBREAK + 1 + i]) {
							pos->pos[POS_LINEBREAK + 1 + i] = 1;
							if (pos->numVerbTypes < 10) {
								pos->verbTypes[pos->numVerbTypes++] = i;
							}
						}
						break;
					}
				}
			}
			start = end;
		}
	}
}

int GetConjString(wchar_t *temp, Match *match) {
	*temp = 0;
	for (int i=0; i<MAX_CONJ_DEPTH && match->conj[i].verbType; i++) {
		if (match->conj[i].verbForm & 2) {
			wcscat(temp, L"Negative ");
		}
		if (match->conj[i].verbForm & 1) {
			wcscat(temp, L"Formal ");
		}
		if (i && match->conj[i].verbTense == TENSE_NON_PAST &&
			(i>1 || match->conj[0].verbTense != TENSE_STEM)) continue;
		if (match->conj[i].verbTense == TENSE_STEM) continue;
		wcscat(temp, conjTable->tenses[match->conj[i].verbTense]);
		wcscat(temp, L" ");
	}
	int q = wcslen(temp);
	if (q && temp[q-1]== ' ') {
		temp[--q] = 0;
	}
	return q;
}
