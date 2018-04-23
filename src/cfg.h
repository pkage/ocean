#ifndef OCEAN_CFG_PARSER
#define OCEAN_CFG_PARSER

#include <stdbool.h>

bool cfg_get(char* file, char* key, char* value);

char* path_cat(char* dest, char* src);

int create_port();

#endif
