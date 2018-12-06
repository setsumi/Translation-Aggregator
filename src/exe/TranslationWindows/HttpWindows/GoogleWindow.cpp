#include <Shared/Shrink.h>
#include "GoogleWindow.h"
#include <Shared/StringUtil.h>

GoogleWindow::GoogleWindow() : HttpWindow(L"Google", L"http://translate.google.com") {
	host = L"translate.google.com";
	path = L"/translate_t";
	postPrefixTemplate = "ie=UTF8&langpair=%s|%s&text=";
	replyCodePage = 932;
	eatBRs = 0;
}

char* GoogleWindow::GetLangIdString(Language lang, int src) {
	if (src && lang == LANGUAGE_Chinese_Traditional) {
		lang = LANGUAGE_Chinese_Simplified;
	}
	switch(lang) {
		case LANGUAGE_AUTO:
			return "auto";
		case LANGUAGE_ENGLISH:
			return "en";
		case LANGUAGE_JAPANESE:
			return "ja";
		case LANGUAGE_Afrikaans:
			return "af";
		case LANGUAGE_Albanian:
			return "sq";
		case LANGUAGE_Arabic:
			return "ar";
		case LANGUAGE_Belarusian:
			return "be";
		case LANGUAGE_Bulgarian:
			return "bg";
		case LANGUAGE_Catalan:
			return "ca";
		case LANGUAGE_Chinese_Simplified:
			return "zh-CN";
		case LANGUAGE_Chinese_Traditional:
			return "zh-TW";
		case LANGUAGE_Croatian:
			return "hr";
		case LANGUAGE_Czech:
			return "cs";
		case LANGUAGE_Danish:
			return "da";
		case LANGUAGE_Dutch:
			return "nl";
		case LANGUAGE_Estonian:
			return "et";
		case LANGUAGE_Filipino:
			return "tl";
		case LANGUAGE_Finnish:
			return "fi";
		case LANGUAGE_French:
			return "fr";
		case LANGUAGE_Galician:
			return "gl";
		case LANGUAGE_German:
			return "de";
		case LANGUAGE_Greek:
			return "el";
		case LANGUAGE_Haitian_Creole:
			return "ht";
		case LANGUAGE_Hebrew:
			return "iw";
		case LANGUAGE_Hindi:
			return "hi";
		case LANGUAGE_Hungarian:
			return "hu";
		case LANGUAGE_Icelandic:
			return "Is";
		case LANGUAGE_Indonesian:
			return "id";
		case LANGUAGE_Irish:
			return "ga";
		case LANGUAGE_Italian:
			return "it";
		case LANGUAGE_Korean:
			return "ko";
		case LANGUAGE_Latvian:
			return "lv";
		case LANGUAGE_Lithuanian:
			return "lt";
		case LANGUAGE_Macedonian:
			return "mk";
		case LANGUAGE_Malay:
			return "ms";
		case LANGUAGE_Maltese:
			return "mt";
		case LANGUAGE_Norwegian:
			return "no";
		case LANGUAGE_Persian:
			return "fa";
		case LANGUAGE_Polish:
			return "pl";
		case LANGUAGE_Portuguese:
			return "pt";
		case LANGUAGE_Romanian:
			return "ro";
		case LANGUAGE_Russian:
			return "ru";
		case LANGUAGE_Serbian:
			return "sr";
		case LANGUAGE_Slovak:
			return "sk:";
		case LANGUAGE_Slovenian:
			return "sl";
		case LANGUAGE_Spanish:
			return "es";
		case LANGUAGE_Swahili:
			return "sw";
		case LANGUAGE_Swedish:
			return "sv";
		case LANGUAGE_Thai:
			return "th";
		case LANGUAGE_Turkish:
			return "tr";
		case LANGUAGE_Ukrainian:
			return "uk";
		case LANGUAGE_Vietnamese:
			return "vi";
		case LANGUAGE_Welsh:
			return "cy";
		case LANGUAGE_Yiddish:
			return "yi";
		default:
			return 0;
	}
}


wchar_t *GoogleWindow::FindTranslatedText(wchar_t* html) {
	int slen;
	wchar_t *start = GetSubstring(html, L"<span id=result_box ", L"</div>", &slen);
	if (start) {
		start[slen] = 0;
		start = wcschr(start, '>');
		if (start) start++;
		return start;
	}
	/*if (!slen) {
		start = GetSubstring(html, "<span id=result_box class=\"long_text\">", "</div>", &slen);
		if (!slen) {
			return 0;
			/*
			start = GetSubstring(html, " dir=\"ltr\">", "</div>", &slen);
			if (!slen) {
				// Sometimes the preceding value has quotes, sometimes not
				// (I think), so I'm a bit wary.
				start = GetSubstring(html, " dir=ltr>", "</div>", &slen);
				if (!slen) return 0;
			}
			//*/
/*		}
	}
	start[slen] = 0;
	return start;//*/
	return 0;
}
