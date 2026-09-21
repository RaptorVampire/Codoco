#ifndef CODOCO_UTILS_H
#define CODOCO_UTILS_H

#include <stddef.h>
#include <stdint.h>
#include <inttypes.h>

char  *xstrdup(const char *s);
void  *xmalloc(size_t n);
void  *xrealloc(void *p, size_t n);
void  *xcalloc(size_t n, size_t sz);

char  *path_join(const char *a, const char *b);
const char *path_basename(const char *p);
const char *path_extension(const char *p);
int    path_is_hidden(const char *p);

void   fmt_num(uint64_t n, char *buf, size_t bufsz);
void   fmt_bytes(uint64_t n, char *buf, size_t bufsz);
void   fmt_pct(double pct, char *buf, size_t bufsz);

int    match_any_ext(const char *ext, const char *const *patterns);
int    glob_match(const char *pattern, const char *text);

uint64_t hash_fnv1a(const void *data, size_t len);

char  *str_trim(char *s);
int    str_icmp(const char *a, const char *b);
int    str_starts_with(const char *s, const char *prefix);
int    str_ends_with(const char *s, const char *suffix);


/* ------------------------------------------------------------------ *
 *  Windows (MinGW / MSYS2) does not provide strsep(), which is a     *
 *  BSD/POSIX extension. Provide a portable fallback so src/options.c *
 *  compiles unchanged.                                               *
 * ------------------------------------------------------------------ */
#if defined(_WIN32) || defined(__MINGW32__)
#  ifndef CODOCO_HAVE_STRSEP
#    define CODOCO_HAVE_STRSEP
#    ifndef strsep
#      define strsep codoco_strsep
#    endif
char *codoco_strsep(char **stringp, const char *delim);
#  endif
#endif


/* ------------------------------------------------------------------ *
 *  Windows (MinGW / MSYS2) does not declare lstat() unless a          *
 *  specific _WIN32_WINNT is set. Symlinks are followed by default on  *
 *  Windows, so mapping lstat -> stat is safe.                         *
 * ------------------------------------------------------------------ */
#if defined(_WIN32) || defined(__MINGW32__)
#  include <sys/stat.h>
#  ifndef lstat
#    define lstat stat
#  endif
#endif

#endif /* CODOCO_UTILS_H */
