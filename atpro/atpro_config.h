#ifndef ATPRO_CONFIG_H
#define ATPRO_CONFIG_H

/*
 * Shared reader for seplugins/atpro.ini, the consolidated configuration file.
 *
 * Format: one "key = value" pair per line, keys are case insensitive,
 * whitespace around keys and values is ignored, lines starting with '#' or ';'
 * are comments. A key may appear several times (e.g. hotspot), the index
 * parameter selects the n-th occurrence.
 *
 * Settings missing from atpro.ini fall back to the legacy single value files
 * (server.txt, port_offset.txt, hotspot.txt, nickname.txt, ...).
 */

#include <pspiofilemgr.h>

#define ATPRO_CONFIG_MAX_SIZE 1024 // static per module, kd modules live in game memory

static const char *atpro_config_paths[] = {
	"ms0:/seplugins/atpro.ini",
	"ef0:/seplugins/atpro.ini",
	// PPSSPP style
	"ms0:/PSP/PLUGINS/atpro/atpro.ini",
};

static inline int atpro_config_is_space(char c){
	return c == ' ' || c == '\t' || c == '\r';
}

static inline char atpro_config_lower(char c){
	if (c >= 'A' && c <= 'Z'){
		return c - 'A' + 'a';
	}
	return c;
}

/**
 * Look up the index-th occurrence of key in atpro.ini
 * @param key Lower case key name
 * @param index 0 for the first occurrence, 1 for the second, ...
 * @param out Value buffer, always null terminated on success
 * @param out_len Size of the value buffer
 * @return 1 if found (value may be empty), 0 if not found or no atpro.ini
 */
static __attribute__((unused)) int atpro_config_get(const char *key, int index, char *out, int out_len)
{
	static char buf[ATPRO_CONFIG_MAX_SIZE];

	int len = -1;
	for (int i = 0;i < sizeof(atpro_config_paths) / sizeof(atpro_config_paths[0]);i++){
		int fd = sceIoOpen(atpro_config_paths[i], PSP_O_RDONLY, 0777);
		if (fd >= 0){
			len = sceIoRead(fd, buf, sizeof(buf) - 1);
			sceIoClose(fd);
			break;
		}
	}
	if (len <= 0){
		return 0;
	}
	buf[len] = '\0';

	char *line = buf;

	// Skip UTF-8 BOM
	if ((unsigned char)line[0] == 0xEF && (unsigned char)line[1] == 0xBB && (unsigned char)line[2] == 0xBF){
		line += 3;
	}

	int found = 0;
	while (*line != '\0'){
		char *end = line;
		while (*end != '\0' && *end != '\n'){
			end++;
		}
		char *next = (*end == '\n') ? end + 1 : end;

		char *p = line;
		while (p < end && atpro_config_is_space(*p)){
			p++;
		}
		while (end > p && atpro_config_is_space(end[-1])){
			end--;
		}

		if (p < end && *p != '#' && *p != ';'){
			const char *k = key;
			while (*k != '\0' && p < end && atpro_config_lower(*p) == *k){
				p++;
				k++;
			}
			if (*k == '\0'){
				while (p < end && atpro_config_is_space(*p)){
					p++;
				}
				if (p < end && *p == '='){
					p++;
					while (p < end && atpro_config_is_space(*p)){
						p++;
					}
					if (found == index){
						int n = 0;
						while (p < end && n < out_len - 1){
							out[n++] = *p++;
						}
						out[n] = '\0';
						return 1;
					}
					found++;
				}
			}
		}

		line = next;
	}

	return 0;
}

/**
 * Read a boolean setting from atpro.ini (1/0, yes/no, true/false, on/off)
 * @return 1 or 0 if set, -1 if not set
 */
static __attribute__((unused)) int atpro_config_get_bool(const char *key)
{
	char value[8] = {0};
	if (!atpro_config_get(key, 0, value, sizeof(value)) || value[0] == '\0'){
		return -1;
	}
	char c = atpro_config_lower(value[0]);
	if (c == '1' || c == 'y' || c == 't' || (c == 'o' && atpro_config_lower(value[1]) == 'n')){
		return 1;
	}
	return 0;
}

#endif
