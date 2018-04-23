/**
 * Ocean plugin system
 */

#include <dlfcn.h>
#include <stdbool.h>
#include "plugin.h"

bool load_plugin(char* path, char* entry, char* sroot, int port) {
	void* handle = dlopen(path, RTLD_LAZY);

	if (!handle) {
		printf("plugin load failure: %s\n", dlerror());
		return false;
	}

	PluginEntry plugin_setup = dlsym(handle, entry);

	char* dlsym_error = dlerror();

	if (dlsym_error) {
		printf("plugin load failure: %s\n", dlsym_error);
		return false;
	}

	plugin_setup(sroot, port);

	dlclose(handle);
	return true;
}


