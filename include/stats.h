#ifndef CODOCO_STATS_H
#define CODOCO_STATS_H

#include <stdint.h>
#include <inttypes.h>

typedef struct {
    char     *name;
    uint64_t  files;
    uint64_t  lines;
    uint64_t  code;
    uint64_t  comments;
    uint64_t  blanks;
    uint64_t  bytes;
    uint64_t  chars;
} LangEntry;

typedef struct {
    LangEntry *items;
    int        count;
    int        cap;
    uint64_t   total_files;
    uint64_t   total_lines;
    uint64_t   total_code;
    uint64_t   total_comments;
    uint64_t   total_blanks;
    uint64_t   total_bytes;
    uint64_t   total_chars;
} LangArray;

typedef struct {
    char     *path;
    char     *lang;
    uint64_t  lines;
    uint64_t  code;
    uint64_t  comments;
    uint64_t  blanks;
    uint64_t  bytes;
} FileEntry;

typedef struct {
    FileEntry *items;
    int        count;
    int        cap;
} FileArray;

void stats_lang_init(LangArray *a);
void stats_lang_free(LangArray *a);
LangEntry *stats_lang_get(LangArray *a, const char *name);
void stats_lang_add(LangArray *a, const char *name,
                    uint64_t lines, uint64_t code, uint64_t comments,
                    uint64_t blanks, uint64_t bytes, uint64_t chars);

void stats_file_init(FileArray *a);
void stats_file_free(FileArray *a);
void stats_file_add(FileArray *a, const char *path, const char *lang,
                    uint64_t lines, uint64_t code, uint64_t comments,
                    uint64_t blanks, uint64_t bytes);

void stats_lang_sort(LangArray *a, int key, int order);
void stats_file_sort(FileArray *a, int key, int order);

#endif /* CODOCO_STATS_H */
