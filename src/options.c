/* Codoco - command line parsing
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 * https://github.com/RaptorVampire/Codoco
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "options.h"
#include "utils.h"
#include "version.h"

void options_default(Options *o) {
    memset(o, 0, sizeof *o);
    o->recursive        = 1;
    o->depth            = 0;
    o->follow_symlinks  = 0;
    o->include_hidden   = 0;
    o->use_ignore_file  = 1;
    o->quiet            = 0;
    o->verbose          = 0;
    o->by_file          = 0;
    o->show_bar         = 1;
    o->show_percent     = 1;
    o->show_total       = 1;
    o->show_header      = 1;
    o->ascii_only       = 0;
    o->no_color         = 0;
    o->max_file_size    = (uint64_t)100 * 1024 * 1024;   /* 100 MiB */
    o->min_file_size    = 0;
    o->format           = OUT_TABLE;
    o->sort_key         = SORT_LINES;
    o->sort_order       = ORDER_DESC;
    o->top_n            = 0;
    o->force_lang       = NULL;
    o->ignore_file      = NULL;
}

static void add_path(Options *o, const char *p) {
    if (o->npaths >= (int)(sizeof o->paths / sizeof o->paths[0])) return;
    o->paths[o->npaths++] = xstrdup(p);
}

static void add_filter(char *arr[], int *n, const char *v) {
    if (*n >= OPT_MAX_FILTER) return;
    /* support comma-separated lists */
    char *copy = xstrdup(v);
    char *p = copy, *tok;
    while ((tok = strsep(&p, ",")) != NULL) {
        tok = str_trim(tok);
        if (*tok && *n < OPT_MAX_FILTER) arr[(*n)++] = xstrdup(tok);
    }
    free(copy);
}

static void usage(void) {
    printf("Usage: codoco [OPTIONS] [PATH...]\n");
    printf("\n");
    printf("  %s v%s - %s\n", CODOCO_NAME, CODOCO_VERSION, CODOCO_TAGLINE);
    printf("  Home: %s\n", CODOCO_URL);
    printf("\n");
    printf("Output:\n");
    printf("  --format=FMT       table (default), json, csv, xml, html\n");
    printf("  --json             shorthand for --format=json\n");
    printf("  --csv              shorthand for --format=csv\n");
    printf("  --xml              shorthand for --format=xml\n");
    printf("  --html             shorthand for --format=html (dark, print-ready)\n");
    printf("  -o, --output=FILE  write report(s) to FILE (default: codoco-report)\n");
    printf("  --theme=MODE       html theme: dark|light|both (default: both)\n");
    printf("  --by-file          per-file report\n");
    printf("  --no-bar           hide bar charts\n");
    printf("  --no-percent       hide percentages\n");
    printf("  --no-total         hide total row\n");
    printf("  --no-header        hide column headers\n");
    printf("  --ascii            ASCII-only output (no Unicode)\n");
    printf("  --no-color         disable ANSI colours\n");
    printf("  --color            force ANSI colours even if not a tty\n");
    printf("  --top=N            show only top N results\n");
    printf("\n");
    printf("Sorting:\n");
    printf("  --sort=KEY         name|files|lines|code|comments|blanks|bytes (default: lines)\n");
    printf("  --order=ORD        asc|desc (default: desc)\n");
    printf("\n");
    printf("Filtering:\n");
    printf("  --include-lang=L   only languages in comma list\n");
    printf("  --exclude-lang=L   skip languages in comma list\n");
    printf("  --include-ext=L    only file extensions in comma list\n");
    printf("  --exclude-ext=L    skip file extensions in comma list\n");
    printf("  --exclude-dir=L    skip directory names (glob) in comma list\n");
    printf("  --max-file-size=N  skip files larger than N bytes\n");
    printf("  --min-file-size=N  skip files smaller than N bytes\n");
    printf("  --force-lang=LANG  treat every file as this language\n");
    printf("\n");
    printf("Traversal:\n");
    printf("  -r, --recursive    recurse (default on)\n");
    printf("      --no-recursive do not recurse\n");
    printf("  -d, --depth=N      maximum recursion depth\n");
    printf("      --hidden       include hidden files and dirs\n");
    printf("      --follow       follow symbolic links\n");
    printf("\n");
    printf("Info:\n");
    printf("  -h, -?, -help, --help   this help\n");
    printf("  -v, --version           version\n");
    printf("  --license               show licence\n");
    printf("  --list-langs, --langs   list supported languages\n");
}

static int parse_format(Options *o, const char *v) {
    if (!strcmp(v, "table")) o->format = OUT_TABLE;
    else if (!strcmp(v, "json")) o->format = OUT_JSON;
    else if (!strcmp(v, "csv"))  o->format = OUT_CSV;
    else if (!strcmp(v, "xml"))  o->format = OUT_XML;
    else if (!strcmp(v, "html")) o->format = OUT_HTML;
    else return -1;
    return 0;
}

static int parse_sort(Options *o, const char *v) {
    if      (!strcmp(v, "name"))     o->sort_key = SORT_NAME;
    else if (!strcmp(v, "files"))    o->sort_key = SORT_FILES;
    else if (!strcmp(v, "lines"))    o->sort_key = SORT_LINES;
    else if (!strcmp(v, "code"))     o->sort_key = SORT_CODE;
    else if (!strcmp(v, "comments")) o->sort_key = SORT_COMMENTS;
    else if (!strcmp(v, "blanks"))   o->sort_key = SORT_BLANKS;
    else if (!strcmp(v, "bytes"))    o->sort_key = SORT_BYTES;
    else return -1;
    return 0;
}

static int parse_order(Options *o, const char *v) {
    if      (!strcmp(v, "asc"))  o->sort_order = ORDER_ASC;
    else if (!strcmp(v, "desc")) o->sort_order = ORDER_DESC;
    else return -1;
    return 0;
}

int options_parse(Options *o, int argc, char **argv) {
    int i = 1;
    for (; i < argc; i++) {
        const char *a = argv[i];

        if (!strcmp(a, "--")) { i++; break; }

        if (a[0] != '-') { add_path(o, a); continue; }

        if (!strcmp(a, "-h") || !strcmp(a, "--help") || !strcmp(a, "-help") || !strcmp(a, "-?")) { o->want_help = 1; continue; }
        if (!strcmp(a, "-v") || !strcmp(a, "--version")) { o->want_version = 1; continue; }
        if (!strcmp(a, "--license")) { o->want_license = 1; continue; }
        if (!strcmp(a, "--list-langs") || !strcmp(a, "--langs")) { o->want_list_langs = 1; continue; }

        if (!strcmp(a, "-r") || !strcmp(a, "--recursive")) { o->recursive = 1; continue; }
        if (!strcmp(a, "--no-recursive")) { o->recursive = 0; continue; }
        if (!strcmp(a, "--hidden")) { o->include_hidden = 1; continue; }
        if (!strcmp(a, "--follow")) { o->follow_symlinks = 1; continue; }
        if (!strcmp(a, "--by-file")) { o->by_file = 1; continue; }
        if (!strcmp(a, "--no-bar")) { o->show_bar = 0; continue; }
        if (!strcmp(a, "--no-percent")) { o->show_percent = 0; continue; }
        if (!strcmp(a, "--no-total")) { o->show_total = 0; continue; }
        if (!strcmp(a, "--no-header")) { o->show_header = 0; continue; }
        if (!strcmp(a, "--ascii")) { o->ascii_only = 1; continue; }
        if (!strcmp(a, "--no-color")) { o->no_color = 1; continue; }
        if (!strcmp(a, "--color")) { o->no_color = -1; continue; }
        if (!strcmp(a, "--json")) { o->format = OUT_JSON; continue; }
        if (!strcmp(a, "--csv")) { o->format = OUT_CSV; continue; }
        if (!strcmp(a, "--xml")) { o->format = OUT_XML; continue; }
        if (!strcmp(a, "--html")) { o->format = OUT_HTML; continue; }
        if (!strcmp(a, "--quiet") || !strcmp(a, "-q")) { o->quiet = 1; continue; }
        if (!strcmp(a, "--verbose")) { o->verbose = 1; continue; }

#define ARG_EQ(name) (!strncmp(a, name "=", strlen(name) + 1))
#define ARG_NEXT(name) (!strcmp(a, name))

        if (ARG_EQ("--format")) { if (parse_format(o, a + 9) < 0) return -1; continue; }
        if (ARG_EQ("--sort"))   { if (parse_sort(o, a + 7) < 0) return -1; continue; }
        if (ARG_EQ("--order"))  { if (parse_order(o, a + 8) < 0) return -1; continue; }
        if (ARG_EQ("--top"))    { o->top_n = atoi(a + 6); continue; }
        if (ARG_EQ("--depth"))  { o->depth = atoi(a + 8); continue; }
        if (ARG_EQ("--max-file-size")) { o->max_file_size = strtoull(a + 16, NULL, 10); continue; }
        if (ARG_EQ("--min-file-size")) { o->min_file_size = strtoull(a + 16, NULL, 10); continue; }
        if (ARG_EQ("--force-lang"))  { o->force_lang = a + 13; continue; }
        if (ARG_EQ("--output")) { o->explicit_out = a + 9; continue; }
        if (ARG_EQ("--theme")) {
            const char *v = a + 8;
            if      (!strcmp(v,"dark"))  o->html_theme = 1;
            else if (!strcmp(v,"light")) o->html_theme = 2;
            else                          o->html_theme = 0;
            continue;
        }
        if (ARG_EQ("-o"))       { o->explicit_out = a + 3; continue; }
        if (ARG_EQ("--include-lang")){ add_filter(o->include_lang, &o->n_include_lang, a + 15); continue; }
        if (ARG_EQ("--exclude-lang")){ add_filter(o->exclude_lang, &o->n_exclude_lang, a + 15); continue; }
        if (ARG_EQ("--include-ext")) { add_filter(o->include_ext,  &o->n_include_ext,  a + 14); continue; }
        if (ARG_EQ("--exclude-ext")) { add_filter(o->exclude_ext,  &o->n_exclude_ext,  a + 14); continue; }
        if (ARG_EQ("--exclude-dir")) { add_filter(o->exclude_dir,  &o->n_exclude_dir,  a + 14); continue; }

        if (ARG_NEXT("-d") && i + 1 < argc) { o->depth = atoi(argv[++i]); continue; }
        if (ARG_NEXT("-o") && i + 1 < argc) { o->explicit_out = argv[++i]; continue; }
        if (ARG_NEXT("--output") && i + 1 < argc) { o->explicit_out = argv[++i]; continue; }
        if (ARG_NEXT("--exclude-dir") && i + 1 < argc) { add_filter(o->exclude_dir, &o->n_exclude_dir, argv[++i]); continue; }
        if (ARG_NEXT("--include-lang") && i + 1 < argc) { add_filter(o->include_lang, &o->n_include_lang, argv[++i]); continue; }
        if (ARG_NEXT("--exclude-lang") && i + 1 < argc) { add_filter(o->exclude_lang, &o->n_exclude_lang, argv[++i]); continue; }

        fprintf(stderr, "codoco: unknown option '%s'\n", a);
        return -1;
#undef ARG_EQ
#undef ARG_NEXT
    }

    for (; i < argc; i++) add_path(o, argv[i]);

    if (o->npaths == 0) add_path(o, ".");
    return 0;
}

/* keep the compiler quiet about usage() when not referenced */
void codoco_print_usage(void) { usage(); }

/* Free all heap-allocated pieces of `Options`. */
void options_free(Options *o) {
    if (!o) return;
    for (int i = 0; i < o->npaths; i++) {
        free(o->paths[i]);
        o->paths[i] = NULL;
    }
    o->npaths = 0;

    for (int i = 0; i < o->n_include_lang; i++) free(o->include_lang[i]);
    for (int i = 0; i < o->n_exclude_lang; i++) free(o->exclude_lang[i]);
    for (int i = 0; i < o->n_include_ext;  i++) free(o->include_ext[i]);
    for (int i = 0; i < o->n_exclude_ext;  i++) free(o->exclude_ext[i]);
    for (int i = 0; i < o->n_exclude_dir;  i++) free(o->exclude_dir[i]);
    o->n_include_lang = o->n_exclude_lang = 0;
    o->n_include_ext  = o->n_exclude_ext  = 0;
    o->n_exclude_dir  = 0;
}
