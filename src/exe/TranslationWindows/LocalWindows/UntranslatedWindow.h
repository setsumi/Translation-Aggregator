#include "../TranslationWindow.h"

class UntranslatedWindow : public TranslationWindow {
	int transHighlighted;
public:
	UntranslatedWindow();
	int WndProc (LRESULT *output, UINT uMsg, WPARAM wParam, LPARAM lParam);
	inline void Translate(SharedString *text) {}
	void SaveWindowTypeConfig();
	void AddClassButtons();
	inline int CanTranslate(Language src, Language dst) {
		return 1;
	}
};
