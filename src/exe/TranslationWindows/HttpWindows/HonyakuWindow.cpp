#include <Shared/Shrink.h>
#include "HonyakuWindow.h"
#include <Shared/StringUtil.h>

HonyakuWindow::HonyakuWindow() : HttpWindow(L"Honyaku", L"http://honyaku.yahoo.co.jp") {
	host = L"honyaku.yahoo.co.jp";
	path = L"/transtext";
}

wchar_t *HonyakuWindow::FindTranslatedText(wchar_t* html) {
	int slen;
	wchar_t *start = GetSubstring(html, L"id=\"trn_textText\" class=\"percent\">", L"</textarea>", &slen);
	if (!slen) return 0;
	start[slen] = 0;
	return start;
}

char *HonyakuWindow::GetTranslationPrefix(Language src, Language dst) {
	char *string = 0;
	if (src == LANGUAGE_JAPANESE && dst == LANGUAGE_ENGLISH) string = "CR-JE";
	else if (src == LANGUAGE_ENGLISH && dst == LANGUAGE_JAPANESE) string = "CR-EJ";
	else if (src == LANGUAGE_Chinese_Simplified && dst == LANGUAGE_JAPANESE) string = "CR-CJ";
	else if (src == LANGUAGE_Chinese_Traditional && dst == LANGUAGE_JAPANESE) string = "CR-CJ";
	else if (src == LANGUAGE_JAPANESE && dst == LANGUAGE_Chinese_Simplified) string = "CR-JC-CN";
	else if (src == LANGUAGE_JAPANESE && dst == LANGUAGE_Chinese_Traditional) string = "CR-JC";
	else if (src == LANGUAGE_Korean && dst == LANGUAGE_JAPANESE) string = "CR-KJ";
	else if (src == LANGUAGE_JAPANESE && dst == LANGUAGE_Korean) string = "CR-JK";
	if (!string) return 0;
	char *temp = (char*)malloc(50);
	sprintf(temp, "both=TH&eid=%s&text=", string);
	return temp;
}
