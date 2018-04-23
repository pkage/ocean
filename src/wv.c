#define WEBVIEW_IMPLEMENTATION
#include "webview.h"

void open_window(char* url, char* title, int width, int height) {
	webview(title, url, width, height, 1);
}

