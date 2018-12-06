#pragma once
#include "HttpWindow.h"

class GoogleWindow : public HttpWindow {
public:
	GoogleWindow();
	char* GetLangIdString(Language lang, int src);
	wchar_t *FindTranslatedText(wchar_t* html);
};
