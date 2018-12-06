#pragma once
#include "HttpWindow.h"

class OCNWindow : public HttpWindow {
public:
	OCNWindow();
	wchar_t *FindTranslatedText(wchar_t* html);
	char *GetTranslationPrefix(Language src, Language dst);
};
