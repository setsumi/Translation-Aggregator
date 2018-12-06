#include <Shared/Shrink.h>
#include "OCNWindow.h"
#include <Shared/StringUtil.h>

OCNWindow::OCNWindow() : HttpWindow(L"OCN", L"http://www.ocn.ne.jp/translation/") {
	host = L"cgi01.ocn.ne.jp";
	path = L"/cgi-bin/translation/index.cgi";
	replyCodePage = 932;
	eatBRs = 1;
}

wchar_t *OCNWindow::FindTranslatedText(wchar_t* html) {
	wchar_t *w = wcsstr(html, L"NAME=\"responseText\"");
	if (!w)
		return 0;
	int slen;
	wchar_t *start = GetSubstring(w, L">", L"</TEXTAREA>", &slen);
	if (!start)
		return 0;
	start[slen] = 0;
	return start;
}

char *OCNWindow::GetTranslationPrefix(Language src, Language dst) {
	char *string = 0;
	if (src == LANGUAGE_JAPANESE && dst == LANGUAGE_ENGLISH) string = "jaen";
	else if (src == LANGUAGE_ENGLISH && dst == LANGUAGE_JAPANESE) string = "enja";
	else if (src == LANGUAGE_Chinese_Simplified && dst == LANGUAGE_JAPANESE) string = "jazh";
	else if (src == LANGUAGE_Chinese_Traditional && dst == LANGUAGE_JAPANESE) string = "jazh";
	else if (src == LANGUAGE_JAPANESE && dst == LANGUAGE_Chinese_Simplified) string = "zhja";
	else if (src == LANGUAGE_JAPANESE && dst == LANGUAGE_Chinese_Traditional) string = "zhja";
	else if (src == LANGUAGE_Korean && dst == LANGUAGE_JAPANESE) string = "koja";
	else if (src == LANGUAGE_JAPANESE && dst == LANGUAGE_Korean) string = "jako";
	if (!string) return 0;
	char *temp = (char*)malloc(50);
	sprintf(temp, "langpair=%s&sourceText=", string);
	return temp;
}
