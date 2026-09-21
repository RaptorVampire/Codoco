#ifndef CODOCO_OPTIONS_H
#define CODOCO_OPTIONS_H

#include <stdint.h>

#define OPT_MAX_FILTER 64

typedef enum {
    OUT_TABLE = 0,
    OUT_JSON,
    OUT_CSV,
    OUT_XML,
    OUT_HTML
} OutputFormat;

typedef enum {
    SORT_NAME = 0,
    SORT_FILES,
    SORT_LINES,
    SORT_CODE,
    SORT_COMMENTS,
    SORT_BLANKS,
    SORT_BYTES
} SortKey;

typedef enum {
    ORDER_ASC = 0,
    ORDER_DESC
} SortOrder;

typedef struct {
    /* targets */
    char *paths[256];
    int   npaths;

    /* filters */
    char *include_lang[OPT_MAX_FILTER];
    int   n_include_lang;
    char *exclude_lang[OPT_MAX_FILTER];
    int   n_exclude_lang;
    char *include_ext[OPT_MAX_FILTER];
    int   n_include_ext;
    char *exclude_ext[OPT_MAX_FILTER];
    int   n_exclude_ext;
    char *exclude_dir[OPT_MAX_FILTER];
    int   n_exclude_dir;

    /* behavior */
    int   recursive;
    int   depth;
    int   follow_symlinks;
    int   include_hidden;
    int   use_ignore_file;
    int   quiet;
    int   verbose;
    int   by_file;
    int   show_bar;
    int   show_percent;
    int   show_total;
    int   show_header;
    int   ascii_only;
    int   no_color;
    int   force_stdin;

    uint64_t max_file_size;
    uint64_t min_file_size;

    OutputFormat format;
    SortKey      sort_key;
    SortOrder    sort_order;
    int          top_n;

    const char  *force_lang;
    const char  *ignore_file;
    const char  *explicit_out;
    int          html_theme;   /* 0=both, 1=dark, 2=light */

    /* misc */
    int  want_help;
    int  want_version;
    int  want_license;
    int  want_list_langs;
} Options;

void options_default(Options *o);
void options_free(Options *o);
int  options_parse(Options *o, int argc, char **argv);
void codoco_print_usage(void);

#endif /* CODOCO_OPTIONS_H */
