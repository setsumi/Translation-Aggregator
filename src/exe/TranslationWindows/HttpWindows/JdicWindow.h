#pragma once
#include "HttpWindow.h"

class JdicWindow : public HttpWindow {
public:
	JdicWindow();
	~JdicWindow();
	void LoadConfig();

	wchar_t *FindTranslatedText(wchar_t* html);
	wchar_t *StripResponse(wchar_t* html);
	void DisplayResponse(wchar_t* text);
	int WndProc(LRESULT *output, UINT uMsg, WPARAM wParam, LPARAM lParam);

	friend INT_PTR CALLBACK JdicDialogProc(HWND hWndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	char *GetTranslationPrefix(Language src, Language dst);
};
