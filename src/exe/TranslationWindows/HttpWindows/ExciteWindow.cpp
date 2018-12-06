#include <Shared/Shrink.h>
#include "ExciteWindow.h"
#include <Shared/StringUtil.h>
#include "../../util/HttpUtil.h"

ExciteWindow::ExciteWindow() : HttpWindow(L"Excite", L"http://www.excite.co.jp") {
	host = L"www.excite.co.jp";
	path = L"/world/english/";
	//postPrefixTemplate = "wb_lp=%s%s&after=start=+%%96%%7C+%%96%%F3+&before=";
	postPrefixTemplate = "wb_lp=%s%s&before=";
}

char *ExciteWindow::GetLangIdString(Language lang, int src) {
	switch(lang) {
		case LANGUAGE_ENGLISH:
			return "EN";
		case LANGUAGE_JAPANESE:
			return "JA";
		default:
			return 0;
	}
}

wchar_t *ExciteWindow::FindTranslatedText(wchar_t* html) {
	int slen;
	wchar_t *start = GetSubstring(html, L"name=\"after\"", L"</textarea>", &slen);
	if (!slen) {
		start = GetSubstring(html, L"name=after", L"</textarea>", &slen);
		if (!slen) return 0;
	}
	wchar_t *out = wcschr(start, L'>');
	if (out && out < start+slen) {
		start[slen] = 0;
		return out+1;
	}
	return html;
}

wchar_t* ExciteWindow::StripResponse(wchar_t* html) {
	UnescapeHtml(html, eatBRs);
	SpiffyUp(html);
	wchar_t *out = html, *in = html;
	while (*in) {
		if (*in != '\n') {
			out++[0] = in++[0];
		}
		else if (in[1] != '\n') {
			in++;
		}
		else {
			while (*in == '\n') {
				out++[0] = in++[0];
			}
		}
	}
	*out = 0;
	return wcsdup(html);
}
