#include <Shared/Shrink.h>
#include "HttpWindow.h"
#include <Shared/StringUtil.h>
#include "../../util/HttpUtil.h"

int numHttpWindows = 0;
HINTERNET hHttpSession = 0;

void CALLBACK HttpCallback(HINTERNET hInternet, DWORD_PTR dwContext, DWORD dwInternetStatus, LPVOID lpvStatusInformation, DWORD dwStatusInformationLength);

int MakeInternet() {
	if (!hHttpSession) {
		hHttpSession = WinHttpOpen(HTTP_REQUEST_ID, WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, 0, 0, WINHTTP_FLAG_ASYNC);
	}
	return hHttpSession != 0;
}

void HttpWindow::CleanupRequest() {
	if (hRequest) {
		// Just in case...
		WinHttpSetStatusCallback(hRequest, 0, 0, 0);
		if (hRequest) WinHttpCloseHandle(hRequest);
		hRequest = 0;
	}
	// Don't keep it around because of error handling...
	// would have to detect its timeout, etc, and
	// MS's WinHttp specs are crap.  And testing timeouts
	// is a pain.
	if (hConnect) {
		WinHttpCloseHandle(hConnect);
		hConnect = 0;
	}
	if (buffer) free(buffer);
	Stopped();
	buffer = 0;
	reading = 0;
	bufferSize = 0;
	if (postData) {
		free(postData);
		postData = 0;
	}

	// See if anything queued up while we were occupied.
	TryMakeRequest();
}

struct ContentCoding {
	int codePage;
	char *string;
};

const ContentCoding contentCodings[] = {
	{932, "SHIFT_JIS"},
	{CP_UTF8, "UTF-8"},
	{20932, "EUC-JP"},
	{28591, "ISO-8859-1"},
	{28591, "Latin-1"},
	{28595, "ISO-8859-5"}
};

char *HttpWindow::GetTranslationPrefix(Language src, Language dst) {
	char *srcString, *dstString;
	if (src == dst || !(srcString = GetLangIdString(src, 1)) || !(dstString = GetLangIdString(dst, 0))) {
		return 0;
	}
	char *out = (char*) malloc(strlen(postPrefixTemplate) + strlen(srcString) + strlen(dstString) + 1);
	sprintf(out, postPrefixTemplate, srcString, dstString);
	return out;
}

int HttpWindow::WndProc (LRESULT *output, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if (uMsg == WMA_CLEANUP_REQUEST) {
		CleanupRequest();
		return 1;
	}
	else if (uMsg == WMA_COMPLETE_REQUEST) {
		CompleteRequest();
		return 1;
	}
	return 0;
}

void HttpWindow::CompleteRequest() {
	if (hWnd) {
		if (!buffer || bufferSize < 4) {
			SetWindowTextA(hWndEdit, "Connection Error.\n");
		}
		else {
			wchar_t *html = (wchar_t*)buffer;
			int len = 0;
			// UTF16, cannabalize buffer.
			if (html[0] == 0xFFFE || html[0] == 0xFEFF) {
				len = bufferSize / 2;
				// BE, need to flip.
				if (html[0] == 0xFFFE) {
					for (int i=1; i<len; i++) {
						html[i-1] = ntohs(html[i]);
					}
				}
				// LE, just need to move data back one.  Makes cleanup more uniform.
				else {
					for (int i=1; i<len; i++) {
						html[i-1] = html[i];
					}
				}
				html[--len] = 0;
				bufferSize = 0;
				buffer = 0;
			}
			else {
				int p;
				buffer[bufferSize] = 0;
				int detectedCharset = 0;
				for (p=0; p < bufferSize - 40; p++) {
					if (!strnicmp(buffer+p, "<META", 5)) {
						char *e = strchr(buffer+p, '>');
						if (!e) break;
						char *s = buffer+p;
						int flags = 0;
						detectedCharset = 0;
						while (s < e) {
							if (!strnicmp(s, ("content-type"), 12)) flags |=1;
							else if (!strnicmp(s, ("http-equiv"), 10)) flags |=2;
							else if (!strnicmp(s, "content", 7)) {
								while (s < e && *s != '=') s++;
								while (s < e) {
									if (!strnicmp(s, "charset", 7)) {
										s+=7;
										while (s < e && !isalnum((unsigned char)*s)) {
											s++;
										}
										if (s<e) {
											for (int i=0; i<sizeof(contentCodings)/sizeof(contentCodings[0]); i++) {
												if (!strnicmp(s, contentCodings[i].string, strlen(contentCodings[i].string))) {
													detectedCharset = contentCodings[i].codePage;
													break;
												}
											}
											break;
										}
									}
									s++;
								}

							}
							s++;
						}
						if (flags == 3 && detectedCharset) break;
						detectedCharset = 0;
						p = (int) (e - buffer);
					}
				}
				if (!detectedCharset) {
					detectedCharset = replyCodePage;
				}

				len = 1+bufferSize;
				html = ToWideChar(buffer, detectedCharset, len);

				if (len) {
					len--;
				}
			}
			if (!html) {
				SetWindowTextA(hWndEdit, "Invalid Data Received.\n");
			}
			else {
				for (int i=len-1; i >= 0; i--) {
					if (html[i] == 0) html[i] = ' ';
				}
				wchar_t * data = FindTranslatedText(html);
				if (!data) {
					wchar_t *temp = (wchar_t*)malloc(sizeof(wchar_t)*(50 + len));
					wcscpy(temp, L"Unrecognized response received:\n\n");
					memcpy(wcschr(temp, 0), html, sizeof(wchar_t) * (wcslen(html)+1));
					SetWindowTextW(hWndEdit, temp);
					free(temp);
				}
				else {
					// Get rid of CRs, for simpler parsing.
					SpiffyUp(data);
					wchar_t *text = StripResponse(data);
					DisplayResponse(text);
				}
			}
			free(html);
		}
	}
	CleanupRequest();
}

wchar_t* HttpWindow::StripResponse(wchar_t* html) {
	UnescapeHtml(html, eatBRs);
	SpiffyUp(html);
	return wcsdup(html);
}

void HttpWindow::DisplayResponse(wchar_t* text) {
	SetWindowTextW(hWndEdit, text);
	free(text);
}

void CALLBACK HttpCallback(HINTERNET hInternet, DWORD_PTR dwContext, DWORD dwInternetStatus, LPVOID lpvStatusInformation, DWORD dwStatusInformationLength) {
	HttpWindow *w = (HttpWindow *)dwContext;

	if (!w || hInternet != w->hRequest) return;

	if (dwInternetStatus == WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE) {
		if (!WinHttpReceiveResponse(hInternet, 0)) {
			PostMessage(w->hWnd, WMA_CLEANUP_REQUEST, 0, 0);
		}
	}
	else if (dwInternetStatus == WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE) {
		if (!w->reading && !WinHttpQueryDataAvailable(hInternet, 0)) {
			PostMessage(w->hWnd, WMA_COMPLETE_REQUEST, 0, 0);
		}
	}
	else if (dwInternetStatus == WINHTTP_CALLBACK_STATUS_DATA_AVAILABLE) {
		if (!w->reading) {
			int bytes = ((DWORD*)lpvStatusInformation)[0];
			if (bytes <= 0) {
				PostMessage(w->hWnd, WMA_COMPLETE_REQUEST, 0, 0);
			}
			else {
				w->buffer = (char*) realloc(w->buffer, w->bufferSize + bytes+4);
				w->reading = 1;
				if (!WinHttpReadData(hInternet, w->buffer+w->bufferSize, bytes, 0)) {
					w->reading = 0;
					PostMessage(w->hWnd, WMA_COMPLETE_REQUEST, 0, 0);
				}
			}
		}
	}
	else if (dwInternetStatus == WINHTTP_CALLBACK_STATUS_READ_COMPLETE) {
		w->reading = 0;
		if (dwStatusInformationLength <= 0) dwStatusInformationLength = 0;
		w->bufferSize += dwStatusInformationLength;
		w->buffer[w->bufferSize] = 0;
		if (!dwStatusInformationLength || !WinHttpQueryDataAvailable(hInternet, 0)) {
			PostMessage(w->hWnd, WMA_COMPLETE_REQUEST, 0, 0);
		}
	}
	else if (dwInternetStatus == WINHTTP_CALLBACK_STATUS_REQUEST_ERROR) {
		//WINHTTP_ASYNC_RESULT *statusInfo = (WINHTTP_ASYNC_RESULT*)lpvStatusInformation;
		PostMessage(w->hWnd, WMA_COMPLETE_REQUEST, 0, 0);
	}
}

wchar_t *HttpEscapeParamW(wchar_t *src, int len) {
	wchar_t *out = (wchar_t*) malloc(sizeof(wchar_t)*(3*len+1));
	wchar_t *dst = out;
	while (len) {
		len--;
		wchar_t c = *src;
		if (c <= 0x26 || (0x3A <= c && c <= 0x40) || 
			c == '\\' || (0x5B <= c && c <= 0x60) || (0x7B <= c && c <= 0x7E)) {
				wsprintfW(dst, L"%%%02X", c);
				dst += 3;
				src++;
				continue;
		}
		dst++[0] = c;
		src++;
	}
	*dst = 0;
	return out;
}

char *HttpEscapeParamA(char *src, int len) {
	char *out = (char*) malloc(sizeof(char)*(3*strlen(src)+1));
	char *dst = out;
	while (len) {
		len--;
		char c = *src;
		if (c <= 0x26 || (0x3A <= c && c <= 0x40) || 
			c == '\\' || (0x5B <= c && c <= 0x60) || (0x7B <= c && c <= 0x7E)) {
				sprintf(dst, "%%%02X", (unsigned char) c);
				dst += 3;
				src++;
				continue;
		}
		dst++[0] = c;
		src++;
	}
	*dst = 0;
	return out;
}

void HttpWindow::TryMakeRequest() {
	if (hRequest || hConnect) return;
	if (!queuedString) {
		Stopped();
		return;
	}

	if (!CanTranslate(config.langSrc, config.langDst)) {
		ClearQueuedTask();
		CleanupRequest();
		SetWindowText(hWndEdit, L"");
		return;
	}

	if (!MakeInternet()) {
		hConnect = 0;
		SetWindowTextA(hWndEdit, "Failed to create connection.\n");
		ClearQueuedTask();
		CleanupRequest();
		return;
	}

	hConnect = WinHttpConnect(hHttpSession, host, port, 0);
	if (!hConnect) {
		SetWindowTextA(hWndEdit, "Failed to create connection.\n");
		ClearQueuedTask();
		CleanupRequest();
		return;
	}

	busy = 1;

	postData = 0;
	wchar_t *header = L"Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.7";
	int postLen = 0;

	char *postPrefix = this->GetTranslationPrefix(config.langSrc, config.langDst);

	// GET
	/*if (!postPrefix) {
		wchar_t *string = HttpEscapeParamW(queuedString->string, queuedString->length);
		ClearQueuedTask();
		wchar_t *temp = (wchar_t*) malloc(sizeof(wchar_t) * (1+wcslen(path) + wcslen(string)));
		wcscpy(temp, path);
		wcscat(temp, string);
		free(string);
		hRequest = WinHttpOpenRequest(hConnect, L"GET", path, 0, 0, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
		free(temp);
	}*/
	// POST
	//else {
		int len = -1;
		char *postDataTemp = ToMultiByte(queuedString->string, postCodePage, len);
		ClearQueuedTask();
		if (!len) {
			free(postDataTemp);
			CleanupRequest();
			return;
		}
		char *postDataEscaped = HttpEscapeParamA(postDataTemp, len-1);
		free(postDataTemp);

		postLen = strlen(postDataEscaped) + strlen(postPrefix);
		postData = (char*) malloc((postLen+1) * sizeof(char));

		strcpy(postData, postPrefix);
		strcat(postData, postDataEscaped);
		free(postDataEscaped);

		hRequest = WinHttpOpenRequest(hConnect, L"POST", path, 0, 0, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
		header = L"Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.7\r\nContent-Type: application/x-www-form-urlencoded";
	//}//*/
	free(postPrefix);

	if (!hRequest || 
		WINHTTP_INVALID_STATUS_CALLBACK == WinHttpSetStatusCallback(hRequest, HttpCallback, WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS | WINHTTP_CALLBACK_FLAG_DATA_AVAILABLE | WINHTTP_CALLBACK_FLAG_REQUEST_ERROR | WINHTTP_CALLBACK_FLAG_CLOSE_CONNECTION, 0) ||
		!WinHttpSendRequest(hRequest, header, wcslen(header), postData, postLen, postLen, (DWORD_PTR) this)) {
			CleanupRequest();
			SetWindowTextA(hWndEdit, "Failed to create connection.\n");
	}
}

void HttpWindow::Translate(SharedString *string) {
	ClearQueuedTask();
	busy = 1;
	queuedString = string;
	string->AddRef();
	if (!hRequest) {
		TryMakeRequest();
		if (hRequest) SetWindowTextA(hWndEdit, "Requesting Data.\n");
	}
	else {
		SetWindowTextA(hWndEdit, "Busy.  Translation Queued.\n");
	}
}

void AddHttpWindow() {
	MakeInternet();
	numHttpWindows++;
}

HttpWindow::HttpWindow(wchar_t *type, wchar_t *srcUrl, unsigned int flags) :
TranslationWindow(type, 1, srcUrl, flags) {
	eatBRs = 1;
	hRequest = 0;
	hConnect = 0;
	buffer = 0;
	bufferSize = 0;
	reading = 0;

	postData = 0;

	postPrefixTemplate = 0;
	replyCodePage = CP_UTF8;
	postCodePage = CP_UTF8;

	port = 80;

	AddHttpWindow();
};

HttpWindow::~HttpWindow() {
	ClearQueuedTask();
	CleanupRequest();
	numHttpWindows--;
	WinHttpCloseHandle(hConnect);
	if (!numHttpWindows && hHttpSession) {
		WinHttpCloseHandle(hHttpSession);
		hHttpSession = 0;
	}
}

void HttpWindow::Halt() {
	if (hRequest && hWndEdit) {
		SetWindowText(hWndEdit, L"Cancelled");
	}
	ClearQueuedTask();
	CleanupRequest();
}


