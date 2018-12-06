#include <Shared/Shrink.h>

#include "LocalWindows/UntranslatedWindow.h"
#include "LocalWindows/AtlasWindow.h"

#include "HttpWindows/GoogleWindow.h"
#include "HttpWindows/OCNWindow.h"
#include "HttpWindows/BabelfishWindow.h"
#include "HttpWindows/FreeTranslationsWindow.h"
#include "HttpWindows/ExciteWindow.h"
#include "HttpWindows/JdicWindow.h"
#include "HttpWindows/HonyakuWindow.h"

#include "LocalWindows/MecabWindow.h"
#include "LocalWindows/JParseWindow.h"

int MakeTranslationWindows(TranslationWindow** &windows, TranslationWindow* &srcWindow) {
	int numWindows = 0;
	windows = (TranslationWindow**) malloc(sizeof(TranslationWindow*) * 20);
	windows[numWindows++] = srcWindow = new UntranslatedWindow();
	windows[numWindows++] = new AtlasWindow();
	windows[numWindows++] = new GoogleWindow();
	windows[numWindows++] = new OCNWindow();
	windows[numWindows++] = new HonyakuWindow();
	windows[numWindows++] = new BabelfishWindow();
	windows[numWindows++] = new FreeTranslationsWindow();
	windows[numWindows++] = new ExciteWindow();
	windows[numWindows++] = new JdicWindow();
	windows[numWindows++] = new MecabWindow();
	windows[numWindows++] = new JParseWindow();
	return numWindows;
}
