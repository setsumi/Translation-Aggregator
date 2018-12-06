#include "../TranslationWindow.h"



class AtlasWindow : public TranslationWindow {
	// Do the work in another thread.
	// It'll pass a message to the hWnd of the Atlas window in the main thread when done.
	HANDLE hThread;
	void TryStartTask();
	friend DWORD WINAPI AtlasThreadProc(void *taskInfo);
	void AddClassButtons();
public:
	int forceHalt;

	AtlasWindow();
	~AtlasWindow();
	int WndProc (LRESULT *output, UINT uMsg, WPARAM wParam, LPARAM lParam);
	void Translate(SharedString *text);
	void Halt();
	//void SaveWindowTypeConfig();
	int SetUpAtlas();

	inline int CanTranslate(Language src, Language dst) {
		return (src == LANGUAGE_JAPANESE && dst == LANGUAGE_ENGLISH) || (src == LANGUAGE_ENGLISH && dst == LANGUAGE_JAPANESE);
	}
};

void LaunchAtlasDictSearch();
int ConfigureAtlas(HWND hWnd, AtlasConfig &atlasConfig);

