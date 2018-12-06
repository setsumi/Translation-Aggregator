#pragma once
#include "HttpWindow.h"

class ExciteWindow : public HttpWindow {
public:
	ExciteWindow();
	wchar_t *FindTranslatedText(wchar_t* html);
	wchar_t *StripResponse(wchar_t* html);

	char *GetLangIdString(Language lang, int src);
};
