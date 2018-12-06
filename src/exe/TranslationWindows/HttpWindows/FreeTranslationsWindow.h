#pragma once
#include "HttpWindow.h"

class FreeTranslationsWindow : public HttpWindow {
public:
	FreeTranslationsWindow();
	wchar_t *FindTranslatedText(wchar_t* html);

	char *GetLangIdString(Language lang, int src);
	char *GetTranslationPrefix(Language src, Language dst);
};
