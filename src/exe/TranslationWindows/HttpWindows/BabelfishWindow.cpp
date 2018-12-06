#include <Shared/Shrink.h>
#include "BabelfishWindow.h"
#include <Shared/StringUtil.h>

BabelfishWindow::BabelfishWindow() : HttpWindow(L"Babel Fish", L"http://babelfish.yahoo.com") {
	host = L"babelfish.yahoo.com";
	path = L"/translate_txt";
	postPrefixTemplate = "ei=UTF-8&doit=done&fr=bf-res&intl=1&tt=urltext&lp=%s_%s&btnTrTxt=Translate&trtext=";
}

wchar_t *BabelfishWindow::FindTranslatedText(wchar_t* html) {
	int slen;
	wchar_t *start = GetSubstring(html, L"<div id=\"result\">", L"</div>", &slen);
	if (!slen) {
		wchar_t* start = GetSubstring(html, L"<div id=result>", L"</div>", &slen);
		if (!slen) return 0;
	}
	start[slen] = 0;
	return start;
}

char *BabelfishWindow::GetLangIdString(Language lang, int src) {
	switch(lang) {
		case LANGUAGE_ENGLISH:
			return "en";
		case LANGUAGE_JAPANESE:
			return "ja";
		case LANGUAGE_Chinese_Simplified:
			return "zh";
		case LANGUAGE_Chinese_Traditional:
			return "zt";
		case LANGUAGE_Dutch:
			return "nl";
		case LANGUAGE_French:
			return "fr";
		case LANGUAGE_German:
			return "de";
		case LANGUAGE_Greek:
			return "el";
		case LANGUAGE_Italian:
			return "it";
		case LANGUAGE_Korean:
			return "ko";
		case LANGUAGE_Portuguese:
			return "pt";
		case LANGUAGE_Russian:
			return "ru";
		case LANGUAGE_Spanish:
			return "es";
		default:
			return 0;
	}
}

char *BabelfishWindow::GetTranslationPrefix(Language src, Language dst) {
	int happy = 1;
	if (src == LANGUAGE_Chinese_Simplified && dst == LANGUAGE_ENGLISH) happy = 1;
	else if (src == LANGUAGE_Chinese_Simplified && dst == LANGUAGE_Chinese_Traditional) happy = 1;
	else if (src == LANGUAGE_Chinese_Traditional && dst == LANGUAGE_ENGLISH) happy = 1;
	else if (src == LANGUAGE_Chinese_Traditional && dst == LANGUAGE_Chinese_Simplified) happy = 1;
	else if (src == LANGUAGE_ENGLISH && dst == LANGUAGE_Chinese_Traditional) happy = 1;
	else if (src == LANGUAGE_ENGLISH && dst == LANGUAGE_Dutch) happy = 1;
	else if (src == LANGUAGE_ENGLISH && dst == LANGUAGE_French) happy = 1;
	else if (src == LANGUAGE_ENGLISH && dst == LANGUAGE_German) happy = 1;
	else if (src == LANGUAGE_ENGLISH && dst == LANGUAGE_Greek) happy = 1;
	else if (src == LANGUAGE_ENGLISH && dst == LANGUAGE_Italian) happy = 1;
	else if (src == LANGUAGE_ENGLISH && dst == LANGUAGE_JAPANESE) happy = 1;
	else if (src == LANGUAGE_ENGLISH && dst == LANGUAGE_Korean) happy = 1;
	else if (src == LANGUAGE_ENGLISH && dst == LANGUAGE_Portuguese) happy = 1;
	else if (src == LANGUAGE_ENGLISH && dst == LANGUAGE_Russian) happy = 1;
	else if (src == LANGUAGE_ENGLISH && dst == LANGUAGE_Spanish) happy = 1;

	else if (src == LANGUAGE_Dutch && dst == LANGUAGE_ENGLISH) happy = 1;
	else if (src == LANGUAGE_Dutch && dst == LANGUAGE_French) happy = 1;

	else if (src == LANGUAGE_French && dst == LANGUAGE_Dutch) happy = 1;
	else if (src == LANGUAGE_French && dst == LANGUAGE_ENGLISH) happy = 1;
	else if (src == LANGUAGE_French && dst == LANGUAGE_German) happy = 1;
	else if (src == LANGUAGE_French && dst == LANGUAGE_Greek) happy = 1;
	else if (src == LANGUAGE_French && dst == LANGUAGE_Italian) happy = 1;
	else if (src == LANGUAGE_French && dst == LANGUAGE_Portuguese) happy = 1;
	else if (src == LANGUAGE_French && dst == LANGUAGE_Spanish) happy = 1;

	else if (src == LANGUAGE_German && dst == LANGUAGE_ENGLISH) happy = 1;
	else if (src == LANGUAGE_German && dst == LANGUAGE_French) happy = 1;

	else if (src == LANGUAGE_Greek && dst == LANGUAGE_ENGLISH) happy = 1;
	else if (src == LANGUAGE_Greek && dst == LANGUAGE_French) happy = 1;

	else if (src == LANGUAGE_Italian && dst == LANGUAGE_ENGLISH) happy = 1;
	else if (src == LANGUAGE_Italian && dst == LANGUAGE_French) happy = 1;

	else if (src == LANGUAGE_JAPANESE && dst == LANGUAGE_ENGLISH) happy = 1;
	else if (src == LANGUAGE_Korean && dst == LANGUAGE_ENGLISH) happy = 1;

	else if (src == LANGUAGE_Portuguese && dst == LANGUAGE_ENGLISH) happy = 1;
	else if (src == LANGUAGE_Portuguese && dst == LANGUAGE_French) happy = 1;

	else if (src == LANGUAGE_Russian && dst == LANGUAGE_ENGLISH) happy = 1;

	else if (src == LANGUAGE_Spanish && dst == LANGUAGE_ENGLISH) happy = 1;
	else if (src == LANGUAGE_Spanish && dst == LANGUAGE_French) happy = 1;

	if (!happy) return 0;

	return HttpWindow::GetTranslationPrefix(src, dst);
}
