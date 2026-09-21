/* Codoco - statistics accumulation and sorting
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 * https://github.com/RaptorVampire/Codoco
 */
#include <stdlib.h>
#include <string.h>

#include "stats.h"
#include "utils.h"
#include "options.h"

/* --------- LangArray --------- */
void stats_lang_init(LangArray *a) {
    memset(a, 0, sizeof *a);
    a->cap = 64;
    a->items = xcalloc((size_t)a->cap, sizeof(LangEntry));
}

void stats_lang_free(LangArray *a) {
    for (int i = 0; i < a->count; i++) free(a->items[i].name);
    free(a->items);
    memset(a, 0, sizeof *a);
}

static void lang_grow(LangArray *a) {
    if (a->count < a->cap) return;
    a->cap *= 2;
    a->items = xrealloc(a->items, (size_t)a->cap * sizeof(LangEntry));
}

LangEntry *stats_lang_get(LangArray *a, const char *name) {
    for (int i = 0; i < a->count; i++) {
        if (strcmp(a->items[i].name, name) == 0) return &a->items[i];
    }
    lang_grow(a);
    LangEntry *e = &a->items[a->count++];
    memset(e, 0, sizeof *e);
    e->name = xstrdup(name);
    return e;
}

void stats_lang_add(LangArray *a, const char *name,
                    uint64_t lines, uint64_t code, uint64_t comments,
                    uint64_t blanks, uint64_t bytes, uint64_t chars)
{
    LangEntry *e = stats_lang_get(a, name);
    e->files++;
    e->lines    += lines;
    e->code     += code;
    e->comments += comments;
    e->blanks   += blanks;
    e->bytes    += bytes;
    e->chars    += chars;

    a->total_files++;
    a->total_lines    += lines;
    a->total_code     += code;
    a->total_comments += comments;
    a->total_blanks   += blanks;
    a->total_bytes    += bytes;
    a->total_chars    += chars;
}

/* --------- FileArray --------- */
void stats_file_init(FileArray *a) {
    memset(a, 0, sizeof *a);
    a->cap = 128;
    a->items = xcalloc((size_t)a->cap, sizeof(FileEntry));
}

void stats_file_free(FileArray *a) {
    for (int i = 0; i < a->count; i++) { free(a->items[i].path); free(a->items[i].lang); }
    free(a->items);
    memset(a, 0, sizeof *a);
}

static void file_grow(FileArray *a) {
    if (a->count < a->cap) return;
    a->cap *= 2;
    a->items = xrealloc(a->items, (size_t)a->cap * sizeof(FileEntry));
}

void stats_file_add(FileArray *a, const char *path, const char *lang,
                    uint64_t lines, uint64_t code, uint64_t comments,
                    uint64_t blanks, uint64_t bytes)
{
    file_grow(a);
    FileEntry *e = &a->items[a->count++];
    e->path = xstrdup(path);
    e->lang = xstrdup(lang ? lang : "Unknown");
    e->lines = lines;
    e->code = code;
    e->comments = comments;
    e->blanks = blanks;
    e->bytes = bytes;
}

/* --------- sorting --------- */
static SortKey g_sort_key;
static SortOrder g_sort_order;

static int cmp_lang(const void *pa, const void *pb) {
    const LangEntry *a = pa, *b = pb;
    int r = 0;
    switch (g_sort_key) {
        case SORT_NAME:     r = strcmp(a->name, b->name); break;
        case SORT_FILES:    r = (a->files    < b->files   ) ? -1 : (a->files    > b->files   ); break;
        case SORT_LINES:    r = (a->lines    < b->lines   ) ? -1 : (a->lines    > b->lines   ); break;
        case SORT_CODE:     r = (a->code     < b->code    ) ? -1 : (a->code     > b->code    ); break;
        case SORT_COMMENTS: r = (a->comments < b->comments) ? -1 : (a->comments > b->comments); break;
        case SORT_BLANKS:   r = (a->blanks   < b->blanks  ) ? -1 : (a->blanks   > b->blanks  ); break;
        case SORT_BYTES:    r = (a->bytes    < b->bytes   ) ? -1 : (a->bytes    > b->bytes   ); break;
    }
    if (g_sort_order == ORDER_DESC) r = -r;
    if (r == 0) r = strcmp(a->name, b->name);
    return r;
}

void stats_lang_sort(LangArray *a, int key, int order) {
    g_sort_key = (SortKey)key;
    g_sort_order = (SortOrder)order;
    qsort(a->items, (size_t)a->count, sizeof(LangEntry), cmp_lang);
}

static int cmp_file(const void *pa, const void *pb) {
    const FileEntry *a = pa, *b = pb;
    int r = 0;
    switch (g_sort_key) {
        case SORT_NAME:     r = strcmp(a->path, b->path); break;
        case SORT_LINES:    r = (a->lines    < b->lines   ) ? -1 : (a->lines    > b->lines   ); break;
        case SORT_CODE:     r = (a->code     < b->code    ) ? -1 : (a->code     > b->code    ); break;
        case SORT_COMMENTS: r = (a->comments < b->comments) ? -1 : (a->comments > b->comments); break;
        case SORT_BLANKS:   r = (a->blanks   < b->blanks  ) ? -1 : (a->blanks   > b->blanks  ); break;
        case SORT_BYTES:    r = (a->bytes    < b->bytes   ) ? -1 : (a->bytes    > b->bytes   ); break;
        default:            r = strcmp(a->path, b->path); break;
    }
    if (g_sort_order == ORDER_DESC) r = -r;
    if (r == 0) r = strcmp(a->path, b->path);
    return r;
}

void stats_file_sort(FileArray *a, int key, int order) {
    g_sort_key = (SortKey)key;
    g_sort_order = (SortOrder)order;
    qsort(a->items, (size_t)a->count, sizeof(FileEntry), cmp_file);
}
