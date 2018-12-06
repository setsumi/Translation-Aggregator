#pragma once
#include "HttpWindow.h"

class HonyakuWindow : public HttpWindow {
public:
	HonyakuWindow();
	wchar_t *FindTranslatedText(wchar_t* html);
	char *GetTranslationPrefix(Language src, Language dst);
};
