/* Codoco - utilities
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 * https://github.com/RaptorVampire/Codoco
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "utils.h"
#include <inttypes.h>

void *xmalloc(size_t n) {
    void *p = malloc(n ? n : 1);
    if (!p) { fprintf(stderr, "codoco: out of memory\n"); exit(2); }
    return p;
}
void *xcalloc(size_t n, size_t sz) {
    void *p = calloc(n ? n : 1, sz ? sz : 1);
    if (!p) { fprintf(stderr, "codoco: out of memory\n"); exit(2); }
    return p;
}
void *xrealloc(void *p, size_t n) {
    void *q = realloc(p, n ? n : 1);
    if (!q) { fprintf(stderr, "codoco: out of memory\n"); exit(2); }
    return q;
}
char *xstrdup(const char *s) {
    if (!s) return NULL;
    size_t n = strlen(s) + 1;
    char *p = xmalloc(n);
    memcpy(p, s, n);
    return p;
}

char *path_join(const char *a, const char *b) {
    size_t la = strlen(a);
    size_t lb = strlen(b);
    int need_sep = (la > 0 && a[la-1] != '/');
    char *out = xmalloc(la + lb + 2);
    memcpy(out, a, la);
    size_t k = la;
    if (need_sep) out[k++] = '/';
    memcpy(out + k, b, lb);
    out[k + lb] = 0;
    return out;
}

const char *path_basename(const char *p) {
    const char *s = strrchr(p, '/');
    return s ? s + 1 : p;
}

const char *path_extension(const char *p) {
    const char *base = path_basename(p);
    const char *dot  = strrchr(base, '.');
    if (!dot || dot == base) return "";
    return dot + 1;
}

int path_is_hidden(const char *p) {
    const char *b = path_basename(p);
    return b[0] == '.' && b[1] != 0 && strcmp(b, ".") && strcmp(b, "..");
}

void fmt_num(uint64_t n, char *buf, size_t bufsz) {
    char tmp[32];
    snprintf(tmp, sizeof tmp, "%" PRIu64, n);
    size_t len = strlen(tmp);
    size_t commas = (len > 0) ? (len - 1) / 3 : 0;
    if (len + commas + 1 > bufsz) {
        snprintf(buf, bufsz, "%s", tmp);
        return;
    }
    size_t j = len + commas;
    buf[j] = 0;
    int cnt = 0;
    for (size_t i = len; i-- > 0; ) {
        if (cnt == 3) { buf[--j] = ','; cnt = 0; }
        buf[--j] = tmp[i];
        cnt++;
    }
}

void fmt_bytes(uint64_t n, char *buf, size_t bufsz) {
    static const char *units[] = {"B","KiB","MiB","GiB","TiB","PiB"};
    double v = (double)n;
    int u = 0;
    while (v >= 1024.0 && u < 5) { v /= 1024.0; u++; }
    if (u == 0) snprintf(buf, bufsz, "%" PRIu64 " %s", n, units[u]);
    else        snprintf(buf, bufsz, "%.2f %s", v, units[u]);
}

void fmt_pct(double pct, char *buf, size_t bufsz) {
    snprintf(buf, bufsz, "%.2f%%", pct);
}

int glob_match(const char *pat, const char *txt) {
    while (*pat) {
        if (*pat == '*') {
            pat++;
            if (!*pat) return 1;
            while (*txt) {
                if (glob_match(pat, txt)) return 1;
                txt++;
            }
            return glob_match(pat, txt);
        } else if (*pat == '?') {
            if (!*txt) return 0;
            pat++; txt++;
        } else {
            if (*pat != *txt) return 0;
            pat++; txt++;
        }
    }
    return *txt == 0;
}

int match_any_ext(const char *ext, const char *const *patterns) {
    for (int i = 0; patterns[i]; i++) {
        if (glob_match(patterns[i], ext)) return 1;
    }
    return 0;
}

uint64_t hash_fnv1a(const void *data, size_t len) {
    const unsigned char *p = data;
    uint64_t h = 1469598103934665603ULL;
    for (size_t i = 0; i < len; i++) {
        h ^= p[i];
        h *= 1099511628211ULL;
    }
    return h;
}

char *str_trim(char *s) {
    while (*s && isspace((unsigned char)*s)) s++;
    char *e = s + strlen(s);
    while (e > s && isspace((unsigned char)e[-1])) e--;
    *e = 0;
    return s;
}

int str_icmp(const char *a, const char *b) {
    while (*a && *b) {
        int ca = tolower((unsigned char)*a);
        int cb = tolower((unsigned char)*b);
        if (ca != cb) return ca - cb;
        a++; b++;
    }
    return tolower((unsigned char)*a) - tolower((unsigned char)*b);
}

int str_starts_with(const char *s, const char *p) {
    return strncmp(s, p, strlen(p)) == 0;
}

int str_ends_with(const char *s, const char *p) {
    size_t ls = strlen(s), lp = strlen(p);
    if (lp > ls) return 0;
    return memcmp(s + ls - lp, p, lp) == 0;
}

/* Portable replacement for strsep() on Windows. */
#if defined(_WIN32) || defined(__MINGW32__)
char *codoco_strsep(char **stringp, const char *delim) {
    if (!stringp || !*stringp) return NULL;
    char *start = *stringp;
    char *end = strpbrk(start, delim);
    if (end) {
        *end = '\0';
        *stringp = end + 1;
    } else {
        *stringp = NULL;
    }
    return start;
}
#endif
