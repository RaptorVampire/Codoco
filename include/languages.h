#ifndef CODOCO_LANGUAGES_H
#define CODOCO_LANGUAGES_H

#define LANG_MAX_EXT   24
#define LANG_MAX_LINE  8

typedef struct {
    const char *name;
    const char *exts[LANG_MAX_EXT];
    const char *line[LANG_MAX_LINE];
    const char *block_start;
    const char *block_end;
    int         ascii_only;
} Language;

extern const Language CODOCO_LANGS[];
extern const int      CODOCO_LANG_COUNT;

const Language *lang_by_extension(const char *ext);
const Language *lang_by_name(const char *name);
const Language *lang_by_filename(const char *filename);
int             lang_is_known_filename(const char *name);
const Language *lang_by_shebang(const char *first_line);
int             lang_supports_ext(const char *ext);

void            lang_list_all(void);

#endif /* CODOCO_LANGUAGES_H */
