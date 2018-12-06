#include <Shared/Shrink.h>
#include "AtlasWindow.h"
#include <Shared/StringUtil.h>
#include <Shared/ProcessUtil.h>
#include "../../config.h"
#include <Shared/Atlas.h>

#include "../../resource.h"

AtlasWindow *atlasWindow;


AtlasWindow::AtlasWindow() : TranslationWindow(L"ATLAS", 0, L"http://www.fujitsu.com/global/services/software/translation/atlas/", TWF_CONFIG_BUTTON|TWF_SEPARATOR) {

	hThread = 0;
	forceHalt = 0;

	atlasWindow = this;
}

void AtlasWindow::Halt() {
	ClearQueuedTask();
	forceHalt = 1;
}

AtlasWindow::~AtlasWindow() {
	atlasWindow = 0;
	ClearQueuedTask();
	if (hThread) {
		forceHalt = 1;
		if (WAIT_OBJECT_0 != WaitForSingleObject(hThread, 30000)) {
			TerminateThread(hThread, 0);
		}
		CloseHandle(hThread);
		hThread = 0;
	}
	UninitAtlas();
}

struct TaskInfo {
	AtlasWindow *window;
	wchar_t *text;
};

int NeedAbort(int line, int lines, void *data) {
	AtlasWindow *window = (AtlasWindow*)data;
	if (lines >= 100 && line%25 == 0) {
		int countDown = lines;
		int digits = 1;
		while (countDown>9) {
			countDown/=10;
			digits++;
		}
		char layout[100], complete[100];
		sprintf(layout, "Translating... [%%%ii/%%%ii]", digits, digits);
		sprintf(complete, layout, line, lines);
		SetWindowTextA(window->hWndEdit, complete);
	}
	return window->forceHalt;
}

DWORD WINAPI AtlasThreadProc(void *taskInfo) {
	AtlasWindow *window = ((TaskInfo*) taskInfo)->window;
	wchar_t *text = TranslateFull(((TaskInfo*) taskInfo)->text, 0, 1, NeedAbort, window);
	if (window->hWndEdit) {
		SetWindowTextW(window->hWndEdit, text);
	}
	free(text);
	if (window->hWnd) {
		PostMessage(window->hWnd, WMA_THREAD_DONE, 0, 0);
	}
	else if (!window->forceHalt) {
		CloseHandle(window->hThread);
		window->hThread = 0;
	}
	return 0;
};

int AtlasWindow::SetUpAtlas() {
	int transDirection = ATLAS_JAP_TO_ENG;
	if (config.langDst == LANGUAGE_JAPANESE)
		transDirection = ATLAS_ENG_TO_JAP;
	if (GetAtlasTransDirection() == transDirection)
		return 1;
	InjectionSettings settings;
	if (!LoadInjectionSettings(settings, 0, 0, 0))
		return 0;
	if (!InitAtlas(settings.atlasConfig, transDirection))
		return 0;
	return 1;
}

void AtlasWindow::TryStartTask() {
	if (!queuedString) {
		if (busy && !hThread)
			Stopped();
		return;
	}
	if (hThread) {
		forceHalt = 1;
		return;
	}
	forceHalt = 0;
	static TaskInfo taskInfo;
	taskInfo.window = this;
	taskInfo.text = wcsdup(queuedString->string);
	queuedString->Release();
	queuedString = 0;
	busy = 1;
	char env[1000];
	wchar_t wenv[1000];
	GetPrivateProfileStringW(L"ATLAS", L"Environment", L"General", wenv, sizeof(wenv)/sizeof(wchar_t), config.ini);
	if (!WideCharToMultiByte(932, 0, wenv, -1, env, sizeof(env), 0, 0) ||
		!SetUpAtlas() ||
		!(hThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)AtlasThreadProc, &taskInfo, 0, 0))) {
		Stopped();
		if (hWndEdit) SetWindowTextA(hWndEdit, "Failed to initialize Fujitsu ATLAS v14.\n");
		return;
	}
}

void AtlasWindow::Translate(SharedString *string) {
	ClearQueuedTask();
	if (!CanTranslate(config.langSrc, config.langDst)) {
		SetWindowText(hWndEdit, L"");
		return;
	}
	string->AddRef();
	queuedString = string;
	TryStartTask();
}

void LaunchAtlasDictSearch() {
	InjectionSettings settings;
	if (LoadInjectionSettings(settings, 0, 0, 0) && InitAtlas(settings.atlasConfig, ATLAS_JAP_TO_ENG)) {
		AwuWordDel(0, "General", 1, "");
	}
	else {
		MessageBoxA(0, "Unable to open Atlas.", "Error", MB_OK | MB_ICONERROR);
	}
}
void AtlasWindow::AddClassButtons() {
	TBBUTTON tbButtons[] =
    {
        {              2, ID_SEARCH_DICT,   TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0, (INT_PTR)L"ATLAS Dictionary"},
        {              5, ID_ATLAS_TRANSLATION_EDITOR,   TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0, (INT_PTR)L"ATLAS Translation Editor"},
    };
    SendMessage(hWndToolbar, TB_ADDBUTTONS, (WPARAM)sizeof(tbButtons)/sizeof(tbButtons[0]), (LPARAM)&tbButtons);
}

DWORD WINAPI DelayedDeleteFile(wchar_t *file) {
	Sleep(10000);
	DeleteFileW(file);
	free(file);
	return 0;
}


static AtlasConfig *cfg;

INT_PTR CALLBACK AtlasDialogProc(HWND hWndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	int i;
	switch (uMsg) {
		case WM_INITDIALOG:
			{
				wchar_t temp[MAX_PATH*2];
				for (i = IDC_ELLIPSES; i <= IDC_SINGLE_LINE_BREAKS; i++) {
					if (cfg->flags & (1<<(i - IDC_ELLIPSES))) {
						CheckDlgButton(hWndDlg, i, BST_CHECKED);
					}
				}
				{
					HWND hWndCombo = GetDlgItem(hWndDlg, IDC_RULE_SET);
					WIN32_FIND_DATAW data;
					HANDLE hFind = FindFirstFileW(L"Rule Sets\\*.trs", &data);
					SendMessage(hWndCombo, CB_ADDSTRING, 0, (LPARAM) L" None");
					if (hFind !=INVALID_HANDLE_VALUE) {
						do {
							if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
							SendMessage(hWndCombo, CB_ADDSTRING, 0, (LPARAM)data.cFileName);
						}
						while (FindNextFile(hFind, &data));
						FindClose(hFind);
					}
					int i = SendMessage(hWndCombo, CB_FINDSTRINGEXACT, -1, (LPARAM)cfg->trsPath);
					if (i<0) i = 0;
					SendMessage(hWndCombo, CB_SETCURSEL, i, 0);
				}
				{
					if (S_OK == SHGetFolderPathW(0, CSIDL_APPDATA, 0, SHGFP_TYPE_CURRENT, temp)) {
						wchar_t *end = wcschr(temp, 0);
						wsprintf(end, L"\\Fujitsu\\ATLAS\\V%i.0\\transenv.ini", GetAtlasVersion());
						HANDLE hFile = CreateFile(temp, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
						int added = 0;
						HWND hWndCombo = GetDlgItem(hWndDlg, IDC_ENVIRONMENT);
						if (hFile) {
							DWORD size = GetFileSize(hFile, 0);
							if (size && size != INVALID_FILE_SIZE && size < 10*1024*1024) {
								char *data = (char*) malloc(size * 5+5);
								wchar_t *wdata = (wchar_t*)(data + ((size + 3)&~1));
								if (ReadFile(hFile, data, size, &size, 0)) {
									data[size] = 0;
									size = MultiByteToWideChar(932, 0, data, size+1, wdata, size*4+1);
									int lineStart = 1;
									while (*wdata) {
										if (lineStart && wdata[0] == '[') {
											wchar_t *end = ++wdata;
											while (*end && *end != '\r' && *end != '\n' && *end != ']') end++;
											if (end && *end == ']') {
												*end = 0;
												SendMessage(hWndCombo, CB_ADDSTRING, 0, (LPARAM)wdata);
												if (!wcsicmp(cfg->environment, wdata)) added = 1;
												wdata = end+1;
											}
											lineStart = 0;
										}
										else if (wdata[0] == '\r' || wdata[0] == '\n') {
											lineStart = 1;
										}
										else if (wdata[0] != ' ') lineStart = 0;
										wdata++;
									}
								}
								free(data);
							}
							CloseHandle(hFile);
						}
						if (!added) {
							SendMessage(hWndCombo, CB_ADDSTRING, 0, (LPARAM)cfg->environment);
						}
						SendMessage(hWndCombo, CB_SELECTSTRING, -1, (LPARAM)cfg->environment);
					}
				}
			}
			break;
		case WM_COMMAND:
			if (HIWORD(wParam) == BN_CLICKED) {
				i = LOWORD(wParam);
				if (i == IDOK) {
					cfg->flags = 0;
					for (i = IDC_ELLIPSES; i <= IDC_SINGLE_LINE_BREAKS; i++) {
						if (BST_CHECKED == IsDlgButtonChecked(hWndDlg, i))
							cfg->flags |= (1<<(i - IDC_ELLIPSES));
					}
					if (!GetWindowTextW(GetDlgItem(hWndDlg, IDC_ENVIRONMENT), cfg->environment, sizeof(cfg->environment)/sizeof(wchar_t))) {
						wcscpy(cfg->environment, L"General");
					}
					if (!GetWindowTextW(GetDlgItem(hWndDlg, IDC_RULE_SET), cfg->trsPath, sizeof(cfg->trsPath)/sizeof(wchar_t)) || !wcsicmp(cfg->trsPath, L" None")) {
						cfg->trsPath[0] = 0;
					}
					EndDialog(hWndDlg, 1);
				}
				else if (i == IDCANCEL) {
					EndDialog(hWndDlg, 0);
				}
			}
			break;
		default:
			break;
	}
	return 0;
}

int ConfigureAtlas(HWND hWnd, AtlasConfig &atlasConfig) {
	cfg = &atlasConfig;
	return DialogBoxParam(ghInst, MAKEINTRESOURCE(IDD_ATLAS_CONFIG), hWnd, AtlasDialogProc, 0);
};

int AtlasWindow::WndProc (LRESULT *output, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
		case WM_COMMAND:
			if (HIWORD(wParam) == BN_CLICKED) {
				if (LOWORD(wParam) == ID_SEARCH_DICT) {
					wchar_t me[2*MAX_PATH];
					if (GetModuleFileNameW(0, me, sizeof(me)/2) <= sizeof(me)/2) {
						LaunchJap(me, L"/ATLAS:DICTSEARCH");
					}
				}
				else if (LOWORD(wParam) == ID_CONFIG) {
					InjectionSettings inj;
					if (!LoadInjectionSettings(inj,0,0,0)) return 0;
					if (ConfigureAtlas(hWnd, inj.atlasConfig)) {
						SaveInjectionSettings(inj, 0, 0);
						UninitAtlas();
					}
					return 1;
				}
				else if (LOWORD(wParam) == ID_ATLAS_TRANSLATION_EDITOR) {
					if (LoadAtlasDlls()) {
						SharedString *text = GetSourceText();
						wchar_t tempFile[MAX_PATH*2];
						char *jis = 0;
						wchar_t *tempFileName = 0;
						if (text) {
							int len = text->length * 4+1;
							char *jis = (char*)malloc(len);
							if (WideCharToMultiByte(932, 0, text->string, -1, jis, len, 0, 0)) {
								int q = GetEnvironmentVariableW(L"temp", tempFile, sizeof(tempFile)/sizeof(wchar_t));
								if (q && q < sizeof(tempFile)/sizeof(wchar_t) - 20) {
									wchar_t name[20] = L"\\Trans-";
									static int count = 0;
									_itow(count, name+7, 10);
									count = (count + 100) % 100;
									wcscat(tempFile, name);
									wcscat(tempFile, L".tmp");
									HANDLE hTemp = CreateFileW(tempFile, GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL, 0);
									if (hTemp) {
										DWORD junk;
										WriteFile(hTemp, jis, strlen(jis), &junk, 0);
										CloseHandle(hTemp);
										tempFileName = wcsdup(tempFile);
									}
								}
							}
							free(jis);
							text->Release();
						}
						wchar_t path[2*MAX_PATH];
						wcscpy(path, AtlasPath);
						wcscat(path, L"Atledit.exe");
						LaunchJap(path, tempFileName);
						if (tempFileName) {
							HANDLE hThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)DelayedDeleteFile, tempFileName, 0, 0);
							CloseHandle(hThread);
						}

					}
				}
			}
			break;
		case WMA_THREAD_DONE:
			CloseHandle(hThread);
			hThread = 0;
			TryStartTask();
			return 1;
		default:
			break;
	}
	return 0;
}


