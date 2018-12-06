#include <Shared/Shrink.h>
#include "JdicWindow.h"
#include <Shared/StringUtil.h>
#include "../../util/HttpUtil.h"
#include "../../Config.h"

#include "../../resource.h"

#define WWWJDIC_DEFAULT L"http://www.aa.tufs.ac.jp/~jwb/cgi-bin/wwwjdic.cgi?1C"

void JdicWindow::LoadConfig() {
	free(host);
	free(path);
	wchar_t url[1000];
	GetPrivateProfileStringW(L"WWWJDIC", L"Mirror", WWWJDIC_DEFAULT, url, sizeof(url)/sizeof(wchar_t), config.ini);
	int base = 0;
	if (!wcsnicmp(url, L"http://", 7)) base += 7;
	if (!wcschr(url+base, '/')) {
		wcscpy(url+base, WWWJDIC_DEFAULT);
	}
	wchar_t *p = wcschr(url+base, '/');
	wchar_t *p2 = wcsstr(p, L"?9U");
	if (!p2) {
		wchar_t *p2 = wcsstr(p, L"?1C");
		if (!p2) p2 = wcschr(p, 0);
		wcscpy(p2, L"?9U");
	}
	path = wcsdup(p);
	*p = 0;
	host = wcsdup(url+base);
}

JdicWindow::JdicWindow() : HttpWindow(L"WWWJDIC", L"http://www.aa.tufs.ac.jp/~jwb/cgi-bin/wwwjdic.cgi?1C", TWF_RICH_TEXT | TWF_SEPARATOR | TWF_CONFIG_BUTTON) {
	//host = L"127.0.0.1";
	//path = L"/wwwjdic.cgi.htm?";
	host = 0;
	path = 0;

	LoadConfig();

	//host = L"www.csse.monash.edu.au";
	//path = L"/~jwb/cgi-bin/wwwjdic.cgi?9U";
	// EUC-JP
	postCodePage = 20932;
	replyCodePage = 20932;
	eatBRs = 0;
}

char *JdicWindow::GetTranslationPrefix(Language src, Language dst) {
	if (src == LANGUAGE_JAPANESE && dst == LANGUAGE_ENGLISH)
		return strdup("glleng=60&dicsel=9&gloss_line=");
	return 0;
}


JdicWindow::~JdicWindow() {
	free(host);
	free(path);
	host = 0;
	path = 0;
}

wchar_t *JdicWindow::FindTranslatedText(wchar_t* html) {
	int slen;
	wchar_t *start = GetSubstring(html, L"&nbsp;</font><br>", L"<p>", &slen);
	if (!slen) return 0;

	start[slen] = 0;
	return start;
}

wchar_t *JdicWindow::StripResponse(wchar_t* html) {
	wchar_t *pos=html;
	int inFont = 0;
	wchar_t *color = (wchar_t*)calloc(2, sizeof(wchar_t));
	int colorLen = 0;

	COLORREF last = 0;
	while (*pos) {
		if (!wcsnicmp(pos, L"<font", 5)) {
			COLORREF c = 0;
			if (!wcsnicmp(pos+5, L" color=\"blue\">", 14)) {
				pos += 19;
				c = RGB(0,0,255);
				if (last == c) c ^= RGB(0,0x80,0);
			}
			else if (!wcsnicmp(pos+5, L" color=\"red\">", 13)) {
				pos += 18;
				c = RGB(255,0,0);
				if (last == c) c = RGB(255,128,0);
			}
			last = c;
			wchar_t *start = pos;
			if (c) {
				while (*pos && pos[0] != '<') {
					pos++;
				}
				if (wcsnicmp(pos, L"</font>", 7) || pos == start) continue;
				color = (wchar_t*) realloc(color, sizeof(wchar_t) * (pos - start + 4 + colorLen));
				wcsncpy(color+colorLen, start, pos-start);
				colorLen += pos-start;
				color[colorLen++] = 0;
				*(COLORREF*)(color + colorLen) = c;
				colorLen += 2;
				color[colorLen] = 0;
			}
		}
		pos++;
	}
	UnescapeHtml(html, eatBRs);
	SpiffyUp(html);
	int len = wcslen(html);

	wchar_t *out = (wchar_t*)malloc((len+1 + colorLen+1)*sizeof(wchar_t));
	wcscpy(out, html);
	memcpy(out+len+1, color, sizeof(wchar_t)*(colorLen+1));
	free(color);
	return out;
}


void JdicWindow::DisplayResponse(wchar_t* text) {
	if (hWndEdit) {
		SetWindowTextW(hWndEdit, text);
		wchar_t *blockStart = text;
		wchar_t *color = wcschr(text, 0) + 1;

		CHARFORMAT cf;
		memset(&cf, 0, sizeof(cf));
		cf.cbSize = sizeof(cf);
		cf.dwMask = CFM_COLOR;
		cf.dwEffects = 0;
		while (1) {
			while (*blockStart==' ' || *blockStart == '\n' || *blockStart == '\t') blockStart++;
			wchar_t *lineStart = blockStart;
			wchar_t *lineEnd = wcsstr(blockStart, L"\n\n");
			if (!lineEnd) {
				lineEnd = wcschr(blockStart, '\n');
				if (!lineEnd) break;
			}
			lineEnd += 2;
			wchar_t *colorDefStart = lineEnd;
			wchar_t *pos = lineStart;
			while (*color && lineEnd) {
				pos = wcsstr(pos, color);
				if (!pos || pos >= lineEnd) break;
				int len = wcslen(color);
				wchar_t *color2 = color+len+1;
				cf.crTextColor = *(COLORREF*)color2;
				SendMessage(hWndEdit, EM_SETSEL, pos-text, pos-text + len);
				SendMessage(hWndEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
				//break;
				while (*colorDefStart && *colorDefStart < 0x100) colorDefStart++;
				wchar_t *colorDefStop = colorDefStart;
				while (*colorDefStop >= 0x100) colorDefStop++;
				SendMessage(hWndEdit, EM_SETSEL, colorDefStart-text, colorDefStop-text);
				SendMessage(hWndEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
				while(*colorDefStart && *colorDefStart != '\n') colorDefStart++;
				if (!*colorDefStart) break;

				pos += len;
				color = color2+2;
			}
			if (!pos || !*color) break;
			blockStart = wcsstr(lineEnd, L"\n\n");
			if (!blockStart) break;
		}//*/

		wchar_t *pos = text;
		while (pos = wcsstr(pos+1, L"[Partial Match!]")) {
			cf.crTextColor = RGB(255,0,0);
			SendMessage(hWndEdit, EM_SETSEL, pos-text, pos-text + 16);
			SendMessage(hWndEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
		}

		pos = wcschr(text,  0);
		wchar_t *lastParen = 0;
		while (pos >= text) {
			if (*pos == ')') lastParen = pos+1;
			if (!wcsnicmp(pos, L"\n Possible ", 11) && lastParen) {
				SendMessage(hWndEdit, EM_SETSEL, lastParen-text, lastParen-text);
				SendMessage(hWndEdit, EM_REPLACESEL, 0, (LPARAM)L"\n\t");
			}
			if (*pos == '\n') lastParen = 0;
			pos --;
		}
		SendMessage(hWndEdit, EM_SETSEL, 0, 0);
	}
	free(text);
}

INT_PTR CALLBACK JdicDialogProc(HWND hWndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	HWND hWndCombo = GetDlgItem(hWndDlg, IDC_WWWJDICT_MIRROR_COMBO);
	switch (uMsg) {
		case WM_INITDIALOG:
			{
				const wchar_t *mirrors[] = {
					L"http://www.csse.monash.edu.au/~jwb/cgi-bin/wwwjdic.cgi?9U",
					L"http://ryouko.imsb.nrc.ca/cgi-bin/wwwjdic?9U",
					L"http://jp.msmobiles.com/cgi-bin/wwwjdic?9U",
					L"http://www.aa.tufs.ac.jp/~jwb/cgi-bin/wwwjdic.cgi?9U",
					L"http://wwwjdic.sys5.se/cgi-bin/wwwjdic.cgi?9U",
					L"http://www.edrdg.org/cgi-bin/wwwjdic/wwwjdic?9U",
				};
				for (int i=0; i<sizeof(mirrors)/sizeof(mirrors[0]); i++) {
					SendMessage(hWndCombo, CB_ADDSTRING, 0, (LPARAM)mirrors[i]);
				}
				JdicWindow * jdic = (JdicWindow*)lParam;
				wchar_t temp[10000];
				wsprintf(temp, L"http://%s%s", jdic->host, jdic->path);
				int w = SendMessage(hWndCombo, CB_FINDSTRINGEXACT, -1, (LPARAM) temp);
				if (w < 0) {
					SendMessage(hWndCombo, CB_ADDSTRING, 0, (LPARAM)temp);
				}
				SendMessage(hWndCombo, CB_SELECTSTRING, 0, (LPARAM)temp);
			}
			break;
		case WM_COMMAND:
			if (HIWORD(wParam) == BN_CLICKED) {
				int i = LOWORD(wParam);
				if (i == IDOK) {
					wchar_t temp[10000];
					int len = GetWindowTextW(hWndCombo, temp, sizeof(temp)/sizeof(wchar_t));
					if (len > 0 && len < sizeof(temp)/sizeof(wchar_t)) {
						WritePrivateProfileStringW(L"WWWJDIC", L"Mirror", temp, config.ini);
					}
					EndDialog(hWndDlg, 0);
				}
				else if (i==IDCANCEL) {
					EndDialog(hWndDlg, 0);
				}
			}
			break;
			/*
			if (HIWORD(wParam) == BN_CLICKED) {
				i = LOWORD(wParam);
				if (i == IDOK || i == IDC_APPLY) {
					for (int j=0; j<3; j++) {
						if (GetDlgItemText(hWndDlg, IDC_DEFAULT_COLOR + 2*j, str, 30)) {
							wchar_t *end = 0;
							unsigned int color = wcstoul(str, &end, 16);
							mecabWindow->colors[j] = RGBFlip(color);
						}
					}
					mecabWindow->normalFontSize = GetDlgItemInt(hWndDlg, IDC_NORMAL_FONT_SIZE, 0, 1);
					mecabWindow->furiganaFontSize = GetDlgItemInt(hWndDlg, IDC_FURIGANA_FONT_SIZE, 0, 1);
					mecabWindow->characterType = HIRAGANA;
					if (BST_CHECKED == IsDlgButtonChecked(hWndDlg, IDC_KATAKANA)) {
						mecabWindow->characterType = KATAKANA;
					}
					else if (BST_CHECKED == IsDlgButtonChecked(hWndDlg, IDC_ROMAJI)) {
						mecabWindow->characterType = ROMAJI;
					}
					mecabWindow->SetFont();
					mecabWindow->SaveWindowTypeConfig();
					if (i == IDOK)
						EndDialog(hWndDlg, 1);
				}
				else if (i == IDCANCEL) {
					EndDialog(hWndDlg, 0);
				}
				else if (i >= IDC_DEFAULT_COLOR && i < IDC_DEFAULT_COLOR+6 && ((i-IDC_DEFAULT_COLOR)&1)) {
					int index = (i-IDC_DEFAULT_COLOR)/2;
					unsigned int color = 0;
					if (GetDlgItemText(hWndDlg, i-1, str, 30)) {
						color = wcstoul(str, 0, 16);
					}
					CHOOSECOLOR cc;
					cc.lStructSize = sizeof(cc);
					cc.hwndOwner = hWndDlg;
					cc.rgbResult = color;
					static COLORREF customColors[16] = {
						0xFFFFFF,0xFFFFFF,0xFFFFFF,0xFFFFFF,
						0xFFFFFF,0xFFFFFF,0xFFFFFF,0xFFFFFF,
						0xFFFFFF,0xFFFFFF,0xFFFFFF,0xFFFFFF,
						0xFFFFFF,0xFFFFFF,0xFFFFFF,0xFFFFFF,
					};
					cc.lpCustColors = customColors;
					cc.Flags = CC_ANYCOLOR | CC_RGBINIT | CC_FULLOPEN;
					if (ChooseColor(&cc)) {
						wsprintfW(str, L"%06X", RGBFlip(cc.rgbResult));
						SetDlgItemText(hWndDlg, i-1, str);
					}
				}
			}
			break;
			//*/
		default:
			break;
	}
	return 0;
}

int JdicWindow::WndProc(LRESULT *output, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if (uMsg == WM_COMMAND) {
		if (HIWORD(wParam) == BN_CLICKED) {
			if (LOWORD(wParam) == ID_CONFIG) {
				DialogBoxParam(ghInst, MAKEINTRESOURCE(IDD_WWWJDIC_CONFIG), hWnd, JdicDialogProc, (LPARAM)this);
				LoadConfig();
				return 1;
			}
		}
	}
	return HttpWindow::WndProc(output, uMsg, wParam, lParam);;
}