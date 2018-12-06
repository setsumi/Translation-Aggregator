#pragma once
#include "../TranslationWindow.h"
// Needed for language enum.

class HttpWindow : public TranslationWindow {
protected:
	// Both UTF8 by default.
	int postCodePage;
	int replyCodePage;

	wchar_t *host;
	wchar_t *path;
	unsigned short port;
	char eatBRs;

	// Cached posted strings, as per WebHttp specs.
	char *postData;

	char *postPrefixTemplate;
	// Finds the text to be translated from a response and null terminates it.
	// Returns null on failure.  Doesn't remove HTML formatting.
	inline virtual wchar_t *FindTranslatedText(wchar_t* html) {return 0;}
	virtual wchar_t *StripResponse(wchar_t* html);
	virtual void DisplayResponse(wchar_t* text);

	char *buffer;
	int bufferSize;
	int reading;
	HINTERNET hRequest;
	HINTERNET hConnect;
	void CleanupRequest();
	void CompleteRequest();

	virtual char * GetLangIdString(Language lang, int src) {return 0;}
	// If this is non-zero, language pair isn't supported.
	// Note that this must be in whatever format the server wants posted,
	// and posting using wide characters is not yet supported, as no
	// site needs it.
	virtual char *GetTranslationPrefix(Language src, Language dst);

	// Lets me use same code to come up with necessary request as
	// checks if languages are supported.
	inline int CanTranslate(Language src, Language dst) {
		char *prefix = GetTranslationPrefix(src, dst);
		if (!prefix) return 0;
		free(prefix);
		return 1;
	}
public:
	int WndProc (LRESULT *output, UINT uMsg, WPARAM wParam, LPARAM lParam);
	friend void CALLBACK HttpCallback(HINTERNET hInternet2, DWORD_PTR dwContext, DWORD dwInternetStatus, LPVOID lpvStatusInformation, DWORD dwStatusInformationLength);
	HttpWindow(wchar_t *type, wchar_t *srcUrl, unsigned int flags=0);
	virtual ~HttpWindow();

	void Halt();

	void Translate(SharedString *text);
	// void Translate(wchar_t *text, char len);
	void TryMakeRequest();
};

