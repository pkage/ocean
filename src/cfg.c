/**
 * Ocean cfg parser
 * @author Patrick Kage
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <ranlib.h>
#include "cfg.h"

bool cfg_get(char* file, char* key, char* fvalue) {
	FILE* fp = fopen(file, "r");
	if (!fp) {
		return NULL;
	}

	int ch, offset = 0;
	char line[256];

	while ((ch = fgetc(fp)) != EOF) {
		if (ch == '\n') {
			line[offset] = '\0';
			if (strncmp(line, key, strlen(key)) == 0) {
				strcpy(fvalue, line + strlen(key) + 2);
				fclose(fp);
				return true;
			}
			offset = 0;
			continue;
		}
		line[offset] = ch;
		offset += 1;
	}

	fclose(fp);
	return false;
}

char* path_cat(char* dest, char* src) {
	if (dest[strlen(dest) - 1] == '/') {
		dest[strlen(dest) - 1] = '\0';
	}
	if (src[0] == '/') {
		strcat(dest, src);
	} else {
		strcat(dest, "/");
		strcat(dest, src);
	}
	
	return dest;
}

int create_port() {
	srand(time(0));
	return 20000 + (rand() % 45536);
}
