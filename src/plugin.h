#ifndef OCEAN_PLUGIN
#define OCEAN_PLUGIN

#include "web.h"

typedef void (*PluginEntry)(char* sroot, int port);
bool load_plugin(char* path, char* entry, char* sroot, int port);

#endif
