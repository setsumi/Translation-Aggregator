#pragma once
// Global file.  Takes care of memory allocation functions and includes
// Windows.h.

#ifndef _DEBUG
// #define NO_CRT
#endif

#ifdef NO_CRT
#define _CRT_ALLOCATION_DEFINED
#define _USE_32BIT_TIME_T
#define strnicmp UBERGOAT
#define wcsnicmp UBERCHICKEN
#define stricmp UBERCHICKEN2
#define wcsicmp UBERCHICKEN3
#endif

#ifndef UNICODE
#define UNICODE
#endif

#define _WIN32_WINNT 0x0501
#define INITGUID
#include <WinSock2.h>
#include <Windows.h>
#include <stdio.h>

// Needed for typedefs used by http windows.
#include <Winhttp.h>

// Needed all over the place
#include <CommCtrl.h>

// Here because so many files use its messages.
#include <Richedit.h>

// Used by most of the external dll-dependent cpp files (Both Atlas ones, Mecab)
#include <shlobj.h>

#include <time.h>

#include <Psapi.h>

//#include <D2d1.h>
//#include <Dwrite.h>

#ifdef NO_CRT
#undef stricmp
#undef wcsicmp
#undef strnicmp
#undef wcsnicmp
#endif

// Simple way of ensuring a clean build before release.
#define APP_NAME L"Translation Aggregator"
#define APP_VERSION L"0.4.3"

//#define HTTP_REQUEST_ID L"TRAG/" APP_VERSION
#define HTTP_REQUEST_ID L"Mozilla/4.0 (compatible; MSIE 8.0; Windows NT 5.1; Trident/4.0; .NET CLR 2.0.50727; .NET CLR 3.0.04506.648; .NET CLR 3.5.21022; .NET CLR 3.0.4506.2152; .NET CLR 3.5.30729; .NET CLR 1.1.4322; Creative AutoUpdate v1.40.01)"

#define MAIN_WINDOW_TITLE APP_NAME L" " APP_VERSION
#define MAIN_WINDOW_CLASS APP_NAME L" Main Window"
#define TRANSLATION_WINDOW_CLASS APP_NAME L" Translation Window"
#define DRAG_WINDOW_CLASS APP_NAME L" Drag Window"

// Makes different versions incompatible with each other's dlls,
// often unnecessarily, but saves me the effort of any other versioning.

extern HINSTANCE ghInst;
extern HWND hWndSuperMaster;

#define WMA_TRANSLATE_ALL				(WM_APP + 0x11)
#define WMA_TRANSLATE					(WM_APP + 0x12)
#define WMA_AUTO_TRANSLATE_CLIPBOARD	(WM_APP + 0x13)
#define WMA_TRANSLATE_HIGHLIGHTED		(WM_APP + 0x14)
#define WMA_AUTO_COPY					(WM_APP + 0x15)
#define WMA_AUTO_TRANSLATE_CONTEXT		(WM_APP + 0x16)
#define WMA_THREAD_DONE					(WM_APP + 0x20)
#define WMA_DRAGSTART					(WM_APP + 0x30)
#define WMA_CLEANUP_REQUEST				(WM_APP + 0x40)
#define WMA_COMPLETE_REQUEST			(WM_APP + 0x41)
#define WMA_SOCKET_MESSAGE				(WM_APP + 0x50)

#ifndef _DEBUG
	#if (_MSC_VER<1300)
		#pragma comment(linker,"/RELEASE")
		#pragma comment(linker,"/opt:nowin98")
	#endif
#endif

//#pragma comment(linker,"/MERGE:.data=.rdata")
//#pragma comment(linker,"/MERGE:.rdata=.text")


#ifdef NO_CRT

#define wcschr mywcschr
__forceinline wchar_t *wcschr(const wchar_t *s, wchar_t c) {
	while (*s != c) {
		if (!*s) return 0;
		s++;
	}
	return (wchar_t *)s;
}

#define strchr mystrchr
__forceinline char *strchr(const char *s, char c) {
	while (*s != c) {
		if (!*s) return 0;
		s++;
	}
	return (char *)s;
}

inline void * malloc(size_t size) {
	return HeapAlloc(GetProcessHeap(), 0, size);
}

inline void * calloc(size_t num, size_t size) {
	//	return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size*num);
	size_t s = num*size;
	void *res = malloc(s);
	if (res) memset(res, 0, s);
	return res;
}

inline void free(__inout_opt void * mem) {
	if (mem) HeapFree(GetProcessHeap(), 0, mem);
}

inline void * realloc(void *mem, size_t size) {
	if (!mem) {
		return malloc(size);
	}

	if (!size) {
		free(mem);
		return 0;
	}
	return HeapReAlloc(GetProcessHeap(), 0, mem, size);
}

inline wchar_t * __cdecl wcsdup(const wchar_t *in) {
	size_t size = sizeof(wchar_t) * (1+wcslen(in));
	wchar_t *out = (wchar_t*) malloc(size);
	if (out)
		memcpy(out, in, size);
	return out;
}

inline char *strdup(char *in) {
	size_t size = sizeof(char) * (1+strlen(in));
	char *out = (char*) malloc(size);
	if (out)
		memcpy(out, in, size);
	return out;
}

__forceinline void * __cdecl operator new(size_t lSize)
{
	return HeapAlloc(GetProcessHeap(), 0, lSize);
}

__forceinline void __cdecl operator delete(void *pBlock)
{
	HeapFree(GetProcessHeap(), 0, pBlock);
}

__forceinline char* __cdecl strdup(const char *s) {
	size_t len = strlen(s);
	char *out = (char*)malloc((len+1) * sizeof(char));
	strcpy(out, s);
	return out;
}

__forceinline char* __cdecl strchr(const char *s) {
	size_t len = strlen(s);
	char *out = (char*)malloc((len+1) * sizeof(char));
	strcpy(out, s);
	return out;
}

#endif

__forceinline unsigned short PASCAL FAR ntohs(unsigned short s) {
	return (s<<8) + (s>>8);
}

__forceinline int ToCOutput(int v) {
	if (v) return v-2;
	return 0;
}

__forceinline int __cdecl strnicmp(const char *s1, const char *s2, size_t len) {
	return ToCOutput(CompareStringA(LOCALE_INVARIANT, NORM_IGNORECASE, s1, (int)len, s2, (int)len));
}

__forceinline int __cdecl stricmp(const char *s1, const char *s2) {
	return ToCOutput(CompareStringA(LOCALE_INVARIANT, NORM_IGNORECASE, s1, -1, s2, -1));
}
#define wcscmp MyWcsCmp
__forceinline int __cdecl wcscmp(const wchar_t *s1, const wchar_t *s2) {
	return ToCOutput(CompareStringW(LOCALE_INVARIANT, 0, s1, -1, s2, -1));
}

__forceinline int __cdecl wcsncmp(const wchar_t *s1, const wchar_t *s2, size_t len) {
	return ToCOutput(CompareStringW(LOCALE_INVARIANT, 0, s1, (int)len, s2, (int)len));
}

__forceinline int __cdecl wcsnicmp(const wchar_t *s1, const wchar_t *s2, size_t len) {
	return ToCOutput(CompareStringW(LOCALE_INVARIANT, NORM_IGNORECASE, s1, (int)len, s2, (int)len));
}

__forceinline int __cdecl wcsicmp(const wchar_t *s1, const wchar_t *s2) {
	return ToCOutput(CompareStringW(LOCALE_INVARIANT, NORM_IGNORECASE, s1, -1, s2, -1));
}

__forceinline int __cdecl wcsijcmp(const wchar_t *s1, const wchar_t *s2) {
	while (*s1 || *s2) {
		// Have to do this character by character so comparing entire strings is consistent with comparing substrings.
		int w = ToCOutput(CompareStringW(LOCALE_INVARIANT, NORM_IGNOREKANATYPE | NORM_IGNORECASE, s1, 1, s2, 1));
		if (w) return w;
		// Fix for long Japanese dash matching every other funky Japanese symbol.
		// Dash is the only one common enough to be annoying.
		if (*s1 != *s2) {
			if (*s1 == 0x30fc)
				return -1;
			if (*s2 == 0x30fc)
				return 1;
		}
		s1++;
		s2++;
	}
	return 0;
	// Not consistent.  MS is the devil.
	// return ToCOutput(CompareStringW(LOCALE_USER_DEFAULT, NORM_IGNOREKANATYPE | NORM_IGNORECASE, s1, -1, s2, -1));
}

__forceinline int __cdecl wcsnijcmp(const wchar_t *s1, const wchar_t *s2, size_t len) {
	while ((*s1 || *s2) && len) {
		// Have to do this character by character so comparing entire strings is consistent with comparing substrings.
		int w = ToCOutput(CompareStringW(LOCALE_INVARIANT, NORM_IGNOREKANATYPE | NORM_IGNORECASE, s1, 1, s2, 1));
		if (w) return w;
		// Fix for long Japanese dash matching every other funky Japanese symbol.
		// Dash is the only one common enough to be annoying.
		if (*s1 != *s2) {
			if (*s1 == 0x30fc)
				return -1;
			if (*s2 == 0x30fc)
				return 1;
		}
		s1++;
		s2++;
		len--;
	}
	return 0;
	// Not consistent.  MS is the devil.
	// return ToCOutput(CompareStringW(LOCALE_INVARIANT, NORM_IGNOREKANATYPE | NORM_IGNORECASE, s1, (int)len, s2, (int)len));
}

__forceinline wchar_t* __cdecl mywcstok(wchar_t *s1, const wchar_t *delim) {
	static wchar_t *temp;
	if (s1) temp = s1;
	size_t len;
	while (1) {
		len = wcscspn(temp, delim);
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

/*
#define strnicmp(s1, s2, len) ToCOutput(CompareStringA(LOCALE_INVARIANT, NORM_IGNORECASE, s1, (int)len, s2, (int)len))

#define wcsnicmp(s1, s2, len) ToCOutput(CompareStringW(LOCALE_INVARIANT, NORM_IGNORECASE, s1, (int)len, s2, (int)len))

#define wcsicmp(s1, s2) ToCOutput(CompareStringW(LOCALE_INVARIANT, NORM_IGNORECASE, s1, -1, s2, -1))

//*/
/*
inline void WriteLogLine(char *s) {
	HANDLE hFile = CreateFileA("Temp.txt", FILE_APPEND_DATA , 0, 0, OPEN_ALWAYS, 0, 0);
	DWORD d;
	WriteFile(hFile, s, strlen(s), &d, 0);
	WriteFile(hFile, "\n", 1, &d, 0);
	CloseHandle(hFile);
}
inline void WriteLogLineW(wchar_t *s) {
	HANDLE hFile = CreateFileA("Temp.txt", FILE_APPEND_DATA , 0, 0, OPEN_ALWAYS, 0, 0);
	DWORD d;
	for (int i=0; s[i]; i++) {
		WriteFile(hFile, s+i, 1, &d, 0);
	}
	WriteFile(hFile, "\n", 1, &d, 0);
	CloseHandle(hFile);
}
//*/

