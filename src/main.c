/**
 * Ocean main thread
 * @author Patrick Kage
 */

#include "web.h"
#include "wv.h"
#include "cfg.h"
#include "plugin.h"


void init_web(char* app_root, int port) {
	char manifest[100];
	char root[100];
	strcpy(manifest, app_root);
	path_cat(manifest, "manifest.cfg");
	if (!cfg_get(manifest, "root", root)) {
		printf("Plugin load failure: %s\n", manifest);
		return;
	}

	char static_root[256];
	strcpy(static_root, app_root);
	path_cat(static_root, root);

	// patch path
	if (static_root[strlen(static_root) - 1] == '/') {
		static_root[strlen(static_root) - 1] = '\0';
	}

	char plugin[100];
	cfg_get(manifest, "web", plugin);

	char plugin_path[256];
	strcpy(plugin_path, app_root);
	path_cat(plugin_path, plugin);

	char plugin_entry[100];
	cfg_get(manifest, "web_entry", plugin_entry);

	printf("static root: %s\n", static_root);

	if (!load_plugin(plugin_path, plugin_entry, static_root, port)) {
		return;
	}

	return;
}

void init_gui(char* app_root, int port) {
	char title[100];
	char manifest[100];
	char url[100], width[100], height[100];
	strcpy(manifest, app_root);
	path_cat(manifest, "manifest.cfg");
	if (!cfg_get(manifest, "title", title)) {
		return;
	}

	cfg_get(manifest, "width", width);
	cfg_get(manifest, "height", height);

	printf("extracted manifest path: %s\n\ttitle: %s\n", manifest, title);
	sprintf(url, "http://127.0.0.1:%d", port);
	
	open_window(url, title, atoi(width), atoi(height));
	return;
}

int main(int argc, char** argv) {

	if (argc != 2) {
		printf("Usage: ocean [app dir]\n");
		return 1;
	}

	int port = create_port();

	printf("selected port %d\n", port);

	if (!fork()) {
		init_web(argv[1], port);
		exit(0);
	} else {
		init_gui(argv[1], port);
		wait(NULL);
	}

	return 0;
}
