#pragma once
#include "FuriganaWindow.h"

class MecabWindow : public FuriganaWindow {
	// Do the work in another thread.
	// It'll pass a message to the hWnd of the Atlas window in the main thread when done.
	void TryStartTask();

public:

	MecabWindow();
	~MecabWindow();
	void Translate(SharedString *text);
	void DisplayResponse(wchar_t *text);
	int WndProc(LRESULT *output, UINT uMsg, WPARAM wParam, LPARAM lParam);

	inline int CanTranslate(Language src, Language dst) {
		return src == LANGUAGE_JAPANESE && dst == LANGUAGE_ENGLISH;
	}
};

