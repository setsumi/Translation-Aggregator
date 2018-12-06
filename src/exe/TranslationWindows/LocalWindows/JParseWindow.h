#pragma once
#include "FuriganaWindow.h"

class JParseWindow : public FuriganaWindow {
public:
	JParseWindow();
	~JParseWindow();
	void Translate(SharedString *text);
	void DisplayResponse(wchar_t *text);
	int WndProc(LRESULT *output, UINT uMsg, WPARAM wParam, LPARAM lParam);
	void SaveWindowTypeConfig();

	inline int CanTranslate(Language src, Language dst) {
		return src == LANGUAGE_JAPANESE && dst == LANGUAGE_ENGLISH;
	}
};

