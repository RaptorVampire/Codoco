/* Codoco - filesystem scanner & line classifier
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 * https://github.com/RaptorVampire/Codoco
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include "scanner.h"
#include "languages.h"
#include "utils.h"

/* Common binary / non-text extensions skipped by default. */
static const char *BINARY_EXT[] = {
    /* object / executable */
    "o","obj","a","lib","so","dylib","dll","exe","bin","out","elf","ko",
    "class","jar","war","ear","apk","dex","pyc","pyo","pyd","wasm",
    /* archives */
    "zip","tar","gz","tgz","bz2","tbz","xz","txz","7z","rar","lz","lzma","zst",
    /* media / binary */
    "png","jpg","jpeg","gif","bmp","ico","tif","tiff","webp","heic",
    "mp3","mp4","avi","mov","mkv","flv","wmv","wav","ogg","flac","opus","webm",
    "pdf","ps","eps","doc","docx","xls","xlsx","ppt","pptx",
    "ttf","otf","woff","woff2","eot",
    "dylib","pack","idx","dat","db","sqlite","sqlite3","mdb",
    "iso","img","vmdk","qcow2","vdi",
    /* compiled artefacts */
    "gch","pch","d","su","lo","la","lai","rlib","rmeta",
    "min.js","min.css","map",
    NULL
};

static const char *DEFAULT_IGNORE_DIRS[] = {
    ".git", ".hg", ".svn", ".bzr",
    "node_modules", "bower_components", "vendor",
    "__pycache__", ".pytest_cache", ".mypy_cache", ".tox", ".nox", ".ruff_cache",
    "target", "build", "dist", "out", "bin", "obj", "output",
    ".venv", "venv", "env", ".env",
    ".idea", ".vscode", ".vs", ".cache", ".gradle", ".m2", ".cargo",
    "cmake-build-debug", "cmake-build-release",
    "Pods", "DerivedData", ".dart_tool", ".next", ".nuxt",
    ".terraform", ".serverless",
    "coverage", ".nyc_output", "htmlcov",
    NULL
};

static int is_binary_ext(const char *name) {
    for (int i = 0; BINARY_EXT[i]; i++) {
        if (str_ends_with(name, BINARY_EXT[i])) {
            /* ensure it's preceded by '.' or '/', or the whole name matches */
            size_t ln = strlen(name), le = strlen(BINARY_EXT[i]);
            if (ln >= le + 1 && name[ln - le - 1] == '.') return 1;
            if (ln == le) return 1;
        }
    }
    return 0;
}

static int in_list(const char *name, const char *const *list) {
    for (int i = 0; list[i]; i++)
        if (strcmp(name, list[i]) == 0) return 1;
    return 0;
}

static int is_default_ignore_dir(const char *name) {
    return in_list(name, DEFAULT_IGNORE_DIRS);
}

static int is_ignored_by_user(const Options *o, const char *name) {
    for (int i = 0; i < o->n_exclude_dir; i++)
        if (glob_match(o->exclude_dir[i], name)) return 1;
    return 0;
}

/* ---------- line classification ---------- */

typedef struct {
    int  in_block;
    char quote;   /* 0, '\'', '"', '`' */
    int  escape;
} LexState;

static int starts_with(const char *p, const char *end, const char *tok, size_t tlen) {
    if (tlen == 0) return 0;
    if ((size_t)(end - p) < tlen) return 0;
    return memcmp(p, tok, tlen) == 0;
}

/* Classify a buffer into code/comment/blank line counts.
 * Returns 0 on success. */
static int classify(const char *data, size_t size, const Language *lang,
                    uint64_t *out_lines, uint64_t *out_code,
                    uint64_t *out_comments, uint64_t *out_blanks)
{
    uint64_t lines = 0, code = 0, comments = 0, blanks = 0;
    if (size == 0) {
        *out_lines = *out_code = *out_comments = *out_blanks = 0;
        return 0;
    }

    /* Derive line-comment token list (skip empty). */
    const char *lc[LANG_MAX_LINE] = {0};
    int nlc = 0;
    if (lang) {
        for (int i = 0; i < LANG_MAX_LINE && lang->line[i]; i++) {
            if (lang->line[i][0]) lc[nlc++] = lang->line[i];
        }
        /* sort by length descending (bubble, small n) */
        for (int i = 0; i < nlc; i++)
            for (int j = i + 1; j < nlc; j++)
                if (strlen(lc[j]) > strlen(lc[i])) {
                    const char *t = lc[i]; lc[i] = lc[j]; lc[j] = t;
                }
    }

    const char *bs = lang ? lang->block_start : NULL;
    const char *be = lang ? lang->block_end   : NULL;
    size_t bslen = bs ? strlen(bs) : 0;
    size_t belen = be ? strlen(be) : 0;

    LexState st = {0, 0, 0};
    const char *p = data;
    const char *end = data + size;

    while (p < end) {
        /* determine line end (exclusive) */
        const char *le = memchr(p, '\n', (size_t)(end - p));
        const char *line_end = le ? le : end;

        int has_nonws   = 0;
        int has_code    = 0;
        int has_comment = 0;
        int entry_block = st.in_block;
        int entry_quote = st.quote;

        const char *q = p;
        while (q < line_end) {
            unsigned char c = (unsigned char)*q;

            if (st.in_block) {
                has_nonws = 1;
                if (belen && starts_with(q, line_end, be, belen)) {
                    q += belen;
                    st.in_block = 0;
                } else {
                    q++;
                }
                continue;
            }

            if (st.quote) {
                has_nonws = 1;
                has_code  = 1;
                if (st.escape) { st.escape = 0; q++; continue; }
                if (c == '\\') { st.escape = 1; q++; continue; }
                if (c == (unsigned char)st.quote) { st.quote = 0; }
                q++;
                continue;
            }

            if (isspace(c)) { q++; continue; }

            has_nonws = 1;

            /* block comment start */
            if (bslen && starts_with(q, line_end, bs, bslen)) {
                st.in_block = 1;
                q += bslen;
                has_comment = 1;
                continue;
            }

            /* line comment */
            int matched = 0;
            for (int i = 0; i < nlc; i++) {
                size_t L = strlen(lc[i]);
                if (starts_with(q, line_end, lc[i], L)) {
                    matched = 1;
                    has_comment = 1;
                    q = line_end;
                    break;
                }
            }
            if (matched) break;

            /* string start */
            if (c == '"' || c == '\'' || c == '`') {
                st.quote = (char)c;
                st.escape = 0;
                has_code = 1;
                q++;
                continue;
            }

            has_code = 1;
            q++;
        }

        /* classify line */
        if (!has_nonws) blanks++;
        else if (has_code) code++;
        else if (has_comment) comments++;
        else if (entry_block || entry_quote) comments++;
        else code++; /* fallback: treat as code */

        lines++;

        p = le ? le + 1 : end;
        /* line comments do not cross the newline */
    }

    *out_lines = lines;
    *out_code = code;
    *out_comments = comments;
    *out_blanks = blanks;
    return 0;
}

/* ---------- file reading ---------- */

static int looks_binary(const char *buf, size_t n) {
    if (n == 0) return 0;
    size_t check = n < 8192 ? n : 8192;
    int nul = 0;
    for (size_t i = 0; i < check; i++) {
        if (buf[i] == 0) nul++;
    }
    return nul > 0;
}


/* Look for a sibling C++ source with the same basename as `path`
 * (which ends in .h/.H). If found, the header belongs to C++. */
static int has_cpp_sibling(const char *path) {
    size_t len = strlen(path);
    if (len < 2) return 0;
    if (path[len-2] != '.') return 0;
    if (path[len-1] != 'h' && path[len-1] != 'H') return 0;

    static const char *cands[] = {
        "cpp","cc","cxx","c++","hpp","hxx","hh","ipp","tcc",NULL
    };
    char buf[4096];
    if (len - 2 >= sizeof buf - 8) return 0;
    memcpy(buf, path, len - 2);
    buf[len - 2] = 0;

    for (int i = 0; cands[i]; i++) {
        snprintf(buf + len - 2, sizeof buf - (len - 2), ".%s", cands[i]);
        if (access(buf, F_OK) == 0) return 1;
    }
    return 0;
}


/* Heuristic: does this text look like a C++ header?
 *
 * Strong signals that only appear in C++-oriented headers:
 *    - "__cplusplus"           (#ifdef __cplusplus guards)
 *    - `extern "C"`            (C++ linkage specifier)
 *    - `#include <c...>`       (C++ stdlib headers)
 *    - `namespace `            (C++ only)
 *    - `template <`            (C++ only)
 *    - `class X {`             (C++ class definition)
 *
 * A pure C file will not contain any of these markers.
 */
static int looks_like_cpp_header(const char *buf)
{
    if (!buf) return 0;

    static const char *markers[] = {
        "__cplusplus",
        "extern \"C\"",
        "extern \"C\" {",
        "#include <cstddef>",
        "#include <cstdint>",
        "#include <cstdio>",
        "#include <cstdlib>",
        "#include <cstring>",
        "#include <iostream>",
        "#include <vector>",
        "#include <string>",
        "#include <memory>",
        "#include <algorithm>",
        "#include <utility>",
        "#include <type_traits>",
        "namespace ",
        "template <",
        "template<",
        NULL
    };

    for (int i = 0; markers[i]; i++) {
        if (strstr(buf, markers[i]) != NULL) {
            return 1;
        }
    }
    return 0;
}

int scan_file(const char *path, const Options *opt,
              LangArray *langs, FileArray *files,
              uint64_t *bytes_read, int *was_binary)
{
    if (was_binary) *was_binary = 0;

    struct stat st;
    if (stat(path, &st) != 0) return -1;
    if (!S_ISREG(st.st_mode)) return -1;
    if (opt->max_file_size && (uint64_t)st.st_size > opt->max_file_size) return -2;
    if ((uint64_t)st.st_size < opt->min_file_size) return -2;

    FILE *fp = fopen(path, "rb");
    if (!fp) return -1;
    size_t sz = (size_t)st.st_size;
    char *buf = xmalloc(sz + 1);
    size_t got = fread(buf, 1, sz, fp);
    fclose(fp);
    buf[got] = 0;

    if (bytes_read) *bytes_read += got;

    /* Detect language */
    const Language *lang = NULL;
    if (opt->force_lang) {
        lang = lang_by_name(opt->force_lang);
    }
    if (!lang) {
        /* Filename match first (Makefile, Dockerfile, CMakeLists.txt, ...) */
        lang = lang_by_filename(path_basename(path));
    }
    if (!lang) {
        const char *ext = path_extension(path);
        if (*ext) lang = lang_by_extension(ext);
    }
    /* Upgrade a C header to C++ when there is evidence in the
     * same directory OR inside the file itself. */
    if (lang && !opt->force_lang && strcmp(lang->name, "C") == 0) {
        const char *__ext = path_extension(path);
        int is_h = (str_icmp(__ext, "h") == 0 ||
                    str_icmp(__ext, "H") == 0);
        if (is_h) {
            if (has_cpp_sibling(path) || looks_like_cpp_header(buf)) {
                lang = lang_by_name("C++");
            }
        }
    }
    if (!lang && got > 2 && buf[0] == '#' && buf[1] == '!') {
        /* shebang on first line */
        const char *nl = memchr(buf, '\n', got);
        size_t flen = nl ? (size_t)(nl - buf) : got;
        char first[512];
        size_t cp = flen < sizeof first - 1 ? flen : sizeof first - 1;
        memcpy(first, buf, cp);
        first[cp] = 0;
        lang = lang_by_shebang(first);
    }
    if (!lang) {
        /* Fall back: if text-like and no extension, treat as Text */
        if (looks_binary(buf, got)) {
            if (was_binary) *was_binary = 1;
            free(buf);
            return -3;
        }
        lang = lang_by_name("Text");
    }

    if (looks_binary(buf, got)) {
        if (was_binary) *was_binary = 1;
        free(buf);
        return -3;
    }

    /* filters */
    if (opt->n_include_lang) {
        int ok = 0;
        for (int i = 0; i < opt->n_include_lang; i++)
            if (str_icmp(opt->include_lang[i], lang->name) == 0) { ok = 1; break; }
        if (!ok) { free(buf); return -4; }
    }
    for (int i = 0; i < opt->n_exclude_lang; i++) {
        if (str_icmp(opt->exclude_lang[i], lang->name) == 0) { free(buf); return -4; }
    }
    if (opt->n_include_ext) {
        const char *ext = path_extension(path);
        int ok = 0;
        for (int i = 0; i < opt->n_include_ext; i++)
            if (str_icmp(opt->include_ext[i], ext) == 0) { ok = 1; break; }
        if (!ok) { free(buf); return -4; }
    }
    if (opt->n_exclude_ext) {
        const char *ext = path_extension(path);
        for (int i = 0; i < opt->n_exclude_ext; i++)
            if (str_icmp(opt->exclude_ext[i], ext) == 0) { free(buf); return -4; }
    }

    uint64_t L, C, M, B;
    classify(buf, got, lang, &L, &C, &M, &B);

    stats_lang_add(langs, lang->name, L, C, M, B, got, got);
    if (files) {
        stats_file_add(files, path, lang->name, L, C, M, B, got);
    }
    free(buf);
    return 0;
}

/* ---------- recursive scan ---------- */

static int scan_path(ScanResult *r, const Options *opt, const char *path, int depth);

static int scan_dir(ScanResult *r, const Options *opt, const char *path, int depth) {
    DIR *d = opendir(path);
    if (!d) { r->errors++; return -1; }
    struct dirent *de;
    while ((de = readdir(d)) != NULL) {
        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")) continue;
        if (!opt->include_hidden && de->d_name[0] == '.') continue;
        if (is_default_ignore_dir(de->d_name)) continue;
        if (is_ignored_by_user(opt, de->d_name)) continue;
        if (is_binary_ext(de->d_name)) { r->files_skipped++; continue; }

        char *full = path_join(path, de->d_name);
        scan_path(r, opt, full, depth + 1);
        free(full);
    }
    closedir(d);
    return 0;
}

static int scan_path(ScanResult *r, const Options *opt, const char *path, int depth) {
    struct stat st;
    int rc;
    if (opt->follow_symlinks) rc = stat(path, &st);
    else                     rc = lstat(path, &st);
    if (rc != 0) { r->errors++; return -1; }

    if (S_ISDIR(st.st_mode)) {
        if (!opt->recursive && depth > 0) return 0;
        if (opt->depth > 0 && depth > opt->depth) return 0;
        return scan_dir(r, opt, path, depth);
    }

    if (!S_ISREG(st.st_mode)) return 0;
    if (is_binary_ext(path_basename(path))) {
        r->files_skipped++;
        return 0;
    }

    uint64_t bytes_before = r->total_bytes_read;
    int was_bin = 0;
    int s = scan_file(path, opt, &r->langs, opt->by_file ? &r->files : NULL,
                      &r->total_bytes_read, &was_bin);
    if (s == 0) {
        r->files_scanned++;
    } else if (s == -3 || was_bin) {
        r->files_binary++;
    } else {
        r->files_skipped++;
        /* roll back byte count for skipped files */
        r->total_bytes_read = bytes_before;
    }
    return 0;
}

void scanner_init(ScanResult *r) {
    memset(r, 0, sizeof *r);
    stats_lang_init(&r->langs);
    stats_file_init(&r->files);
}

void scanner_free(ScanResult *r) {
    stats_lang_free(&r->langs);
    stats_file_free(&r->files);
}

int scanner_run(ScanResult *r, const Options *opt) {
    for (int i = 0; i < opt->npaths; i++) {
        scan_path(r, opt, opt->paths[i], 0);
    }
    return 0;
}
