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
 * Profiles: "[name]" lines start a profile section, e.g. [socom] and
 * [hunsterverse]. Keys before the first section are global. A key is looked
 * up in the active profile first, then in the global part; if the active
 * profile sets a key at all, its occurrences replace the global ones (so a
 * profile's hotspot list doesn't get mixed with a global one).
 *
 * The active profile is stored in kd/atpro_profile.txt next to the kd modules
 * of the memory the ini was found on (written by the L+R+SELECT menu). If that
 * file is missing or names no existing section, the first section is used.
 * Without sections the ini works as a single flat configuration.
 *
 * The file is read line by line, so there is no size limit.
 *
 * Settings missing from atpro.ini fall back to the legacy single value files
 * (server.txt, port_offset.txt, hotspot.txt, nickname.txt, ...).
 */

#include <pspiofilemgr.h>
#include <string.h>

#define ATPRO_CONFIG_LINE_MAX 160
#define ATPRO_PROFILE_NAME_MAX 32
#define ATPRO_PROFILE_MAX 8

static const char *atpro_config_paths[] = {
	"ms0:/seplugins/atpro.ini",
	"ef0:/seplugins/atpro.ini",
	// PPSSPP style
	"ms0:/PSP/PLUGINS/atpro/atpro.ini",
};

// Active profile file for each entry of atpro_config_paths, same order
static const char *atpro_profile_paths[] = {
	"ms0:/kd/atpro_profile.txt",
	"ef0:/kd/atpro_profile.txt",
	// PPSSPP style
	"ms0:/PSP/PLUGINS/atpro/atpro_profile.txt",
};

#define ATPRO_CONFIG_PATH_COUNT (sizeof(atpro_config_paths) / sizeof(atpro_config_paths[0]))

typedef struct {
	int fd;
	int pos;
	int len;
	int first;
	char buf[128];
} atpro_config_reader;

static inline int atpro_config_is_space(char c){
	return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static inline char atpro_config_lower(char c){
	if (c >= 'A' && c <= 'Z'){
		return c - 'A' + 'a';
	}
	return c;
}

// Bounded string copy, always null terminated (the kd modules link against
// littlec, which has no strcpy/strncpy)
static inline void atpro_config_copy(char *dst, const char *src, int dst_len){
	int i = 0;
	for (;src[i] != '\0' && i < dst_len - 1;i++){
		dst[i] = src[i];
	}
	dst[i] = '\0';
}

/**
 * Open the first atpro.ini found
 * @param which Receives the index into atpro_config_paths, may be NULL
 * @return File descriptor, < 0 if there is no atpro.ini
 */
static __attribute__((unused)) int atpro_config_open(int *which)
{
	for (int i = 0;i < ATPRO_CONFIG_PATH_COUNT;i++){
		int fd = sceIoOpen(atpro_config_paths[i], PSP_O_RDONLY, 0777);
		if (fd >= 0){
			if (which != NULL){
				*which = i;
			}
			return fd;
		}
	}
	return -1;
}

/**
 * Read the next line, without line break; overlong lines are cut off
 * @return 1 if a line was read, 0 at the end of the file
 */
static __attribute__((unused)) int atpro_config_read_line(atpro_config_reader *r, char *line, int line_len)
{
	int n = 0;
	int got = 0;
	while (1){
		if (r->pos >= r->len){
			r->len = sceIoRead(r->fd, r->buf, sizeof(r->buf));
			r->pos = 0;
			if (r->len <= 0){
				r->len = 0;
				break;
			}
		}
		got = 1;
		char c = r->buf[r->pos++];
		if (c == '\n'){
			break;
		}
		if (n < line_len - 1){
			line[n++] = c;
		}
	}
	line[n] = '\0';

	// Skip UTF-8 BOM
	if (r->first){
		r->first = 0;
		if ((unsigned char)line[0] == 0xEF && (unsigned char)line[1] == 0xBB && (unsigned char)line[2] == 0xBF){
			memmove(line, line + 3, n - 2);
		}
	}
	return got;
}

static __attribute__((unused)) char *atpro_config_trim(char *s)
{
	while (*s != '\0' && atpro_config_is_space(*s)){
		s++;
	}
	int n = strlen(s);
	while (n > 0 && atpro_config_is_space(s[n - 1])){
		s[--n] = '\0';
	}
	return s;
}

/**
 * Parse a "[name]" section line (already trimmed)
 * @param out Receives the lower case section name
 * @return 1 if the line is a section line
 */
static __attribute__((unused)) int atpro_config_section(char *line, char *out, int out_len)
{
	int n = strlen(line);
	if (n < 2 || line[0] != '[' || line[n - 1] != ']'){
		return 0;
	}
	line[n - 1] = '\0';
	char *name = atpro_config_trim(line + 1);
	int i = 0;
	for (;name[i] != '\0' && i < out_len - 1;i++){
		out[i] = atpro_config_lower(name[i]);
	}
	out[i] = '\0';
	return 1;
}

/**
 * Match "key = value" (line already trimmed, not a comment or section)
 * @return Pointer to the trimmed value, NULL if the line sets another key
 */
static __attribute__((unused)) char *atpro_config_match(char *line, const char *key)
{
	char *p = line;
	const char *k = key;
	while (*k != '\0' && atpro_config_lower(*p) == *k){
		p++;
		k++;
	}
	if (*k != '\0'){
		return NULL;
	}
	while (*p == ' ' || *p == '\t'){
		p++;
	}
	if (*p != '='){
		return NULL;
	}
	return atpro_config_trim(p + 1);
}

/**
 * Look up the index-th occurrence of key in one part of atpro.ini
 * @param section Lower case profile name, NULL or "" for the global part
 * @param count Receives the number of occurrences in that part, may be NULL
 * @return 1 if found (value may be empty), 0 if not
 */
static __attribute__((unused)) int atpro_config_scan(const char *key, int index, const char *section, char *out, int out_len, int *count)
{
	if (count != NULL){
		*count = 0;
	}
	atpro_config_reader r = {0};
	r.first = 1;
	r.fd = atpro_config_open(NULL);
	if (r.fd < 0){
		return 0;
	}

	int global = (section == NULL || section[0] == '\0');
	int in_scope = global;
	int found = 0;
	int result = 0;
	char raw[ATPRO_CONFIG_LINE_MAX];
	char name[ATPRO_PROFILE_NAME_MAX];
	while (atpro_config_read_line(&r, raw, sizeof(raw))){
		char *line = atpro_config_trim(raw);
		if (line[0] == '\0' || line[0] == '#' || line[0] == ';'){
			continue;
		}
		if (atpro_config_section(line, name, sizeof(name))){
			if (global){
				break; // the global part ends at the first section
			}
			in_scope = strcmp(name, section) == 0;
			continue;
		}
		if (!in_scope){
			continue;
		}
		char *value = atpro_config_match(line, key);
		if (value == NULL){
			continue;
		}
		if (found == index && !result){
			atpro_config_copy(out, value, out_len);
			result = 1;
			if (count == NULL){
				break;
			}
		}
		found++;
	}
	sceIoClose(r.fd);
	if (count != NULL){
		*count = found;
	}
	return result;
}

/**
 * List the profile sections of atpro.ini, in file order
 * @param names Receives up to max lower case profile names
 * @param which Receives the index into atpro_config_paths, may be NULL
 * @return Number of profiles, 0 if there are none or no atpro.ini
 */
static __attribute__((unused)) int atpro_config_profiles(char names[][ATPRO_PROFILE_NAME_MAX], int max, int *which)
{
	atpro_config_reader r = {0};
	r.first = 1;
	r.fd = atpro_config_open(which);
	if (r.fd < 0){
		return 0;
	}
	int count = 0;
	char raw[ATPRO_CONFIG_LINE_MAX];
	char name[ATPRO_PROFILE_NAME_MAX];
	while (count < max && atpro_config_read_line(&r, raw, sizeof(raw))){
		char *line = atpro_config_trim(raw);
		if (atpro_config_section(line, name, sizeof(name)) && name[0] != '\0'){
			int exists = 0;
			for (int i = 0;i < count;i++){
				if (strcmp(names[i], name) == 0){
					exists = 1;
				}
			}
			if (!exists){
				atpro_config_copy(names[count++], name, ATPRO_PROFILE_NAME_MAX);
			}
		}
	}
	sceIoClose(r.fd);
	return count;
}

/**
 * Name of the active profile: the one saved in kd/atpro_profile.txt if it
 * exists in atpro.ini, otherwise the first profile
 * @param out Receives the lower case name, "" without profiles
 * @return 1 if there is an active profile, 0 if atpro.ini has no profiles
 */
static __attribute__((unused)) int atpro_config_active_profile(char *out, int out_len)
{
	char names[ATPRO_PROFILE_MAX][ATPRO_PROFILE_NAME_MAX];
	int which = 0;
	out[0] = '\0';
	int count = atpro_config_profiles(names, ATPRO_PROFILE_MAX, &which);
	if (count == 0){
		return 0;
	}

	char saved[ATPRO_PROFILE_NAME_MAX] = {0};
	int fd = sceIoOpen(atpro_profile_paths[which], PSP_O_RDONLY, 0777);
	if (fd >= 0){
		atpro_config_reader r = {0};
		r.fd = fd;
		r.first = 1;
		char raw[ATPRO_CONFIG_LINE_MAX];
		if (atpro_config_read_line(&r, raw, sizeof(raw))){
			char *name = atpro_config_trim(raw);
			int i = 0;
			for (;name[i] != '\0' && i < sizeof(saved) - 1;i++){
				saved[i] = atpro_config_lower(name[i]);
			}
			saved[i] = '\0';
		}
		sceIoClose(fd);
	}

	int pick = 0;
	for (int i = 0;i < count;i++){
		if (strcmp(names[i], saved) == 0){
			pick = i;
			break;
		}
	}
	atpro_config_copy(out, names[pick], out_len);
	return 1;
}

/**
 * Save the active profile to kd/atpro_profile.txt
 * @return 0 on success, < 0 on failure
 */
static __attribute__((unused)) int atpro_config_save_profile(const char *name)
{
	int which = 0;
	int fd = atpro_config_open(&which);
	if (fd < 0){
		return -1;
	}
	sceIoClose(fd);

	fd = sceIoOpen(atpro_profile_paths[which], PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
	if (fd < 0){
		return fd;
	}
	int len = strlen(name);
	int written = sceIoWrite(fd, name, len);
	sceIoWrite(fd, "\n", 1);
	sceIoClose(fd);
	return written == len ? 0 : -1;
}

/**
 * Look up the index-th occurrence of key in atpro.ini, active profile first
 * @param key Lower case key name
 * @param index 0 for the first occurrence, 1 for the second, ...
 * @param out Value buffer, always null terminated on success
 * @param out_len Size of the value buffer
 * @return 1 if found (value may be empty), 0 if not found or no atpro.ini
 */
static __attribute__((unused)) int atpro_config_get(const char *key, int index, char *out, int out_len)
{
	char profile[ATPRO_PROFILE_NAME_MAX];
	if (atpro_config_active_profile(profile, sizeof(profile))){
		int count = 0;
		if (atpro_config_scan(key, index, profile, out, out_len, &count)){
			return 1;
		}
		if (count > 0){
			return 0; // the profile sets this key, don't mix in global values
		}
	}
	return atpro_config_scan(key, index, NULL, out, out_len, NULL);
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
