#include <Shared/Shrink.h>
#include "FreeTranslationsWindow.h"
#include <Shared/StringUtil.h>

FreeTranslationsWindow::FreeTranslationsWindow() : HttpWindow(L"FreeTranslation", L"www.freetranslation.com") {
	host = L"tets9.freetranslation.com";
	path = L"/";
	postPrefixTemplate = "sequence=core&mode=html&charset=UTF-8&template=results_en-us.htm&language=%s/%s&Submit=FREE%20Translation&srctext=";
}

wchar_t *FreeTranslationsWindow::FindTranslatedText(wchar_t* html) {
	int slen;
	wchar_t *start = GetSubstring(html, L"id=\"resultsBox\">", L"</textarea>", &slen);
	if (!slen) return 0;
	start[slen] = 0;
	return start;
}

char *FreeTranslationsWindow::GetLangIdString(Language lang, int src) {
	switch(lang) {
		case LANGUAGE_AUTO:
			return "";
		case LANGUAGE_ENGLISH:
			return "English";
		case LANGUAGE_JAPANESE:
			return "Japanese";
		case LANGUAGE_Arabic:
			return "Arabic";
		case LANGUAGE_Bulgarian:
			return "Bulgarian";
		case LANGUAGE_Chinese_Simplified:
			return "Chinese";
		case LANGUAGE_Chinese_Traditional:
			return "Chinese";
		case LANGUAGE_Czech:
			return "Czech";
		case LANGUAGE_Danish:
			return "Danish";
		case LANGUAGE_Dutch:
			return "Dutch";
		case LANGUAGE_Finnish:
			return "Finnish";
		case LANGUAGE_French:
			return "French";
		case LANGUAGE_German:
			return "German";
		case LANGUAGE_Greek:
			return "Greek";
		case LANGUAGE_Hausa:
			return "Hausa";
		case LANGUAGE_Hebrew:
			return "Hebrew";
		case LANGUAGE_Hindi:
			return "Hindi";
		case LANGUAGE_Hungarian:
			return "Hungarian";
		case LANGUAGE_Italian:
			return "Italian";
		case LANGUAGE_Korean:
			return "Korean";
		case LANGUAGE_Norwegian:
			return "Norwegian";
		case LANGUAGE_Persian:
			return "Persian";
		case LANGUAGE_Polish:
			return "Polish";
		case LANGUAGE_Portuguese:
			return "Portuguese";
		case LANGUAGE_Pashto:
			return "Pashto";
		case LANGUAGE_Romanian:
			return "Romanian";
		case LANGUAGE_Russian:
			return "Russian";
		case LANGUAGE_Serbian:
			return "Serbian";
		case LANGUAGE_Somali:
			return "Somali";
		case LANGUAGE_Spanish:
			return "Spanish";
		case LANGUAGE_Swedish:
			return "Swedish";
		case LANGUAGE_Thai:
			return "Thai";
		case LANGUAGE_Turkish:
			return "Turkish";
		case LANGUAGE_Urdu:
			return "Urdu";
		default:
			return 0;
	}
}

char *FreeTranslationsWindow::GetTranslationPrefix(Language src, Language dst) {
	int happy = 0;
	if (src == LANGUAGE_JAPANESE && dst == LANGUAGE_ENGLISH) happy = 1;
	else if (src == LANGUAGE_ENGLISH && dst == LANGUAGE_JAPANESE) happy = 1;
	else if (src == LANGUAGE_ENGLISH && dst == LANGUAGE_German) happy = 1;
	if (!happy) return 0;
	return HttpWindow::GetTranslationPrefix(src, dst);
}
