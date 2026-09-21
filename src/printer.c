/* Codoco - printer
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 * https://github.com/RaptorVampire/Codoco
 *
 * All terminal output MUST go through this module so that nothing ever
 * exceeds the terminal width and no line ever wraps unexpectedly.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#if defined(_WIN32) || defined(__MINGW32__)
#  include <windows.h>
#else
#  include <sys/ioctl.h>
#endif

#include "printer.h"

static int  g_color = 0;
static int  g_ascii = 0;
static int  g_width = 80;

/* ---- Unicode box-drawing (or ASCII fallback) ---- */
static const char *BOX_H, *BOX_V;
static const char *BOX_TL, *BOX_TR, *BOX_BL, *BOX_BR;
static const char *BOX_LT, *BOX_RT, *BOX_TT, *BOX_BT, *BOX_X;

static void set_box_chars(void) {
    if (g_ascii) {
        BOX_H = "-"; BOX_V = "|";
        BOX_TL = "+"; BOX_TR = "+"; BOX_BL = "+"; BOX_BR = "+";
        BOX_LT = "+"; BOX_RT = "+"; BOX_TT = "+"; BOX_BT = "+"; BOX_X = "+";
    } else {
        BOX_H = "\xE2\x94\x80";  /* ─ */
        BOX_V = "\xE2\x94\x82";  /* │ */
        BOX_TL = "\xE2\x94\x8C"; /* ┌ */
        BOX_TR = "\xE2\x94\x90"; /* ┐ */
        BOX_BL = "\xE2\x94\x94"; /* └ */
        BOX_BR = "\xE2\x94\x98"; /* ┘ */
        BOX_LT = "\xE2\x94\x9C"; /* ├ */
        BOX_RT = "\xE2\x94\xA4"; /* ┤ */
        BOX_TT = "\xE2\x94\xAC"; /* ┬ */
        BOX_BT = "\xE2\x94\xB4"; /* ┴ */
        BOX_X  = "\xE2\x94\xBC"; /* ┼ */
    }
}

void pr_init(ColorMode mode, int ascii_only) {
    g_ascii = ascii_only ? 1 : 0;

    int want = 0;
    if (mode == COLOR_ALWAYS) want = 1;
    else if (mode == COLOR_NEVER) want = 0;
    else want = isatty(STDOUT_FILENO) ? 1 : 0;

    const char *term = getenv("TERM");
    if (term && !strcmp(term, "dumb")) want = 0;

    g_color = want;    /* terminal width detection (portable) */
    {
#if defined(_WIN32) || defined(__MINGW32__)
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
        if (h != INVALID_HANDLE_VALUE &&
            GetConsoleScreenBufferInfo(h, &csbi)) {
            g_width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        }
#else
        struct winsize ws;
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 &&
            ws.ws_col > 0) {
            g_width = ws.ws_col;
        }
#endif
        if (g_width <= 0) {
        const char *cols = getenv("COLUMNS");
        if (cols && atoi(cols) > 0) g_width = atoi(cols);

        }
    }
    if (g_width < 20) g_width = 20;
    if (g_width > 400) g_width = 400;

    set_box_chars();
}

int pr_width(void) { return g_width; }
int pr_color(void) { return g_color; }

/* Visible length: number of printable columns (UTF-8 aware,
 * ignores ANSI SGR sequences). */
int pr_vislen(const char *s) {
    int n = 0;
    const unsigned char *p = (const unsigned char *)s;
    while (*p) {
        if (p[0] == 0x1B && p[1] == '[') {
            p += 2;
            while (*p && !(*p >= 0x40 && *p <= 0x7E)) p++;
            if (*p) p++;
            continue;
        }
        if (p[0] < 0x80) { p++; n++; }
        else if ((p[0] & 0xE0) == 0xC0) { p += 2; n++; }
        else if ((p[0] & 0xF0) == 0xE0) { p += 3; n++; }
        else if ((p[0] & 0xF8) == 0xF0) { p += 4; n++; }
        else { p++; n++; }
    }
    return n;
}

static void emit_pad(int n) {
    for (int i = 0; i < n; i++) putchar(' ');
}

/* Print string truncated to maxw visible columns (never wraps). */
void pr_print_trimmed(const char *s, int maxw) {
    if (maxw <= 0) return;
    int w = 0;
    const unsigned char *p = (const unsigned char *)s;
    const char *ellipsis = g_ascii ? ".." : "\xE2\x80\xA6";
    int ell_w = g_ascii ? 2 : 1;

    while (*p && w < maxw) {
        if (p[0] == 0x1B && p[1] == '[') {
            const unsigned char *q = p + 2;
            while (*q && !(*q >= 0x40 && *q <= 0x7E)) q++;
            if (*q) q++;
            fwrite(p, 1, (size_t)(q - p), stdout);
            p = q;
            continue;
        }
        int cw;
        if (p[0] < 0x80) cw = 1;
        else if ((p[0] & 0xE0) == 0xC0) cw = 2;
        else if ((p[0] & 0xF0) == 0xE0) cw = 3;
        else if ((p[0] & 0xF8) == 0xF0) cw = 4;
        else cw = 1;

        if (w + 1 > maxw) break;
        if (p[cw] == 0 && w + 1 == maxw) {
            /* perfect fit, print as-is */
        }
        if (w + 1 > maxw - ell_w && p[cw] != 0) {
            /* will need ellipsis later - stop */
            break;
        }
        fwrite(p, 1, cw, stdout);
        p += cw;
        w++;
    }
    if (*p) {
        int rem = maxw - w;
        if (rem >= ell_w) fputs(ellipsis, stdout);
    }
}

/* ---------- Box drawing ---------- */
static void hline(const char *left, const char *mid, const char *right) {
    (void)mid;  /* reserved for future box-drawing */
    int inner = g_width - 2;
    if (inner < 1) inner = 1;
    fputs(left, stdout);
    for (int i = 0; i < inner; i++) fputs(BOX_H, stdout);
    fputs(right, stdout);
    putchar('\n');
}

void pr_box_top(void)    { if (g_color) fputs(C_GRAY, stdout); hline(BOX_TL, NULL, BOX_TR); if (g_color) fputs(C_RESET, stdout); }
void pr_box_bottom(void) { if (g_color) fputs(C_GRAY, stdout); hline(BOX_BL, NULL, BOX_BR); if (g_color) fputs(C_RESET, stdout); }
void pr_box_sep(void)    { if (g_color) fputs(C_GRAY, stdout); hline(BOX_LT, NULL, BOX_RT); if (g_color) fputs(C_RESET, stdout); }

void pr_box_blank(void) {
    if (g_color) fputs(C_GRAY, stdout);
    fputs(BOX_V, stdout);
    if (g_color) fputs(C_RESET, stdout);
    emit_pad(g_width - 2);
    if (g_color) fputs(C_GRAY, stdout);
    fputs(BOX_V, stdout);
    if (g_color) fputs(C_RESET, stdout);
    putchar('\n');
}

/* Print content padded to fill the box interior (no wrap). */
static void box_content(const char *prefix, int content_w, const char *s) {
    (void)content_w;  /* reserved for future use */
    int inner = g_width - 2;
    if (g_color) fputs(C_GRAY, stdout);
    fputs(BOX_V, stdout);
    if (g_color) fputs(C_RESET, stdout);

    int used = 0;
    if (prefix && *prefix) {
        fputs(prefix, stdout);
        used += pr_vislen(prefix);
    }
    putchar(' ');
    used += 1;

    int avail = inner - used - 1;
    if (avail < 1) avail = 1;
    pr_print_trimmed(s ? s : "", avail);
    int w = s ? pr_vislen(s) : 0;
    if (w > avail) w = avail;
    emit_pad(avail - w);
    putchar(' ');

    if (g_color) fputs(C_GRAY, stdout);
    fputs(BOX_V, stdout);
    if (g_color) fputs(C_RESET, stdout);
    putchar('\n');
}

void pr_box_left(const char *s) {
    box_content(NULL, 0, s);
}

void pr_box_center(const char *s) {
    int inner = g_width - 2;
    if (g_color) fputs(C_GRAY, stdout);
    fputs(BOX_V, stdout);
    if (g_color) fputs(C_RESET, stdout);
    int len = s ? pr_vislen(s) : 0;
    int left = (inner - len) / 2;
    if (left < 0) left = 0;
    int right = inner - len - left;
    if (right < 0) right = 0;
    emit_pad(left);
    if (s) pr_print_trimmed(s, inner);
    emit_pad(right);
    if (g_color) fputs(C_GRAY, stdout);
    fputs(BOX_V, stdout);
    if (g_color) fputs(C_RESET, stdout);
    putchar('\n');
}

void pr_box_kv(const char *k, const char *v) {
    int inner = g_width - 2;
    if (g_color) fputs(C_GRAY, stdout);
    fputs(BOX_V, stdout);
    if (g_color) fputs(C_RESET, stdout);
    putchar(' ');

    int klen = k ? pr_vislen(k) : 0;
    int maxk = inner / 2 - 2;
    if (maxk < 8) maxk = 8;
    if (klen > maxk) klen = maxk;
    if (k) {
        if (g_color) fputs(C_BOLD, stdout);
        pr_print_trimmed(k, maxk);
        if (g_color) fputs(C_RESET, stdout);
    }
    emit_pad(maxk - klen);
    fputs(": ", stdout);

    int used = 1 + maxk + 2 + 1;
    int avail = inner - used;
    if (avail < 1) avail = 1;
    if (v) pr_print_trimmed(v, avail);
    int vw = v ? pr_vislen(v) : 0;
    if (vw > avail) vw = avail;
    emit_pad(avail - vw);
    putchar(' ');
    if (g_color) fputs(C_GRAY, stdout);
    fputs(BOX_V, stdout);
    if (g_color) fputs(C_RESET, stdout);
    putchar('\n');
}

void pr_box_wrapped(const char *text, int indent) {
    if (!text) return;
    int inner = g_width - 2 - indent - 2;
    if (inner < 10) inner = 10;

    const char *p = text;
    while (*p) {
        while (*p == ' ') p++;
        const char *end = p;
        int w = 0;
        const char *last_space = NULL;
        while (*end && w < inner) {
            if (*end == ' ') last_space = end;
            unsigned char c = (unsigned char)*end;
            int cw = 1;
            if (c >= 0x80) {
                if ((c & 0xE0) == 0xC0) cw = 2;
                else if ((c & 0xF0) == 0xE0) cw = 3;
                else if ((c & 0xF8) == 0xF0) cw = 4;
            }
            end += cw;
            w++;
        }
        if (*end && last_space) end = last_space;
        char buf[1024];
        size_t len = (size_t)(end - p);
        if (len >= sizeof buf) len = sizeof buf - 1;
        memcpy(buf, p, len);
        buf[len] = 0;

        char pad[256] = {0};
        char line[1600];
        {
            int rv = snprintf(line, sizeof line, "%s%s", pad, buf);
            if (rv < 0) line[0] = 0;
        }
        int n = indent < 255 ? indent : 255;
        memset(pad, ' ', n);
        pad[n] = 0;

        pr_box_left(line);

        p = end;
        while (*p == ' ') p++;
    }
}

/* ---------- Tables ---------- */
void pr_table_header(const char **cols, const int *widths, const int *aligns, int n) {
    int total = 1;
    for (int i = 0; i < n; i++) total += widths[i] + 1;
    (void)total; /* caller must shrink widths to fit */
    putchar(' ');
    for (int i = 0; i < n; i++) {
        if (i) putchar(' ');
        if (g_color) fputs(C_BOLD, stdout);
        int w = widths[i];
        int vl = cols[i] ? pr_vislen(cols[i]) : 0;
        int pad = w - vl;
        if (pad < 0) pad = 0;
        if (aligns[i] == AL_RIGHT) emit_pad(pad);
        if (cols[i]) pr_print_trimmed(cols[i], w);
        if (aligns[i] != AL_RIGHT) emit_pad(pad);
        if (g_color) fputs(C_RESET, stdout);
    }
    putchar('\n');
}

void pr_table_sep(const int *widths, int n) {
    if (g_color) fputs(C_GRAY, stdout);
    putchar(' ');
    for (int i = 0; i < n; i++) {
        if (i) putchar(' ');
        for (int j = 0; j < widths[i]; j++) fputs(BOX_H, stdout);
    }
    if (g_color) fputs(C_RESET, stdout);
    putchar('\n');
}

void pr_table_row(const char **cells, const int *widths, const int *aligns, int n) {
    putchar(' ');
    for (int i = 0; i < n; i++) {
        if (i) putchar(' ');
        int w = widths[i];
        int vl = cells[i] ? pr_vislen(cells[i]) : 0;
        int pad = w - vl;
        if (pad < 0) pad = 0;
        if (aligns[i] == AL_RIGHT) emit_pad(pad);
        if (cells[i]) pr_print_trimmed(cells[i], w);
        if (aligns[i] != AL_RIGHT) emit_pad(pad);
    }
    putchar('\n');
}

/* ---------- Bars ---------- */
void pr_bar(double pct, int width, const char *color) {
    if (width < 4) width = 4;
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    int filled = (int)((pct / 100.0) * width + 0.5);
    if (filled > width) filled = width;
    if (g_color && color) fputs(color, stdout);
    for (int i = 0; i < filled; i++) {
        if (g_ascii) putchar('#');
        else fputs("\xE2\x96\x88", stdout); /* █ */
    }
    if (g_color) fputs(C_GRAY, stdout);
    for (int i = filled; i < width; i++) {
        if (g_ascii) putchar('.');
        else fputs("\xE2\x96\x91", stdout); /* ░ */
    }
    if (g_color) fputs(C_RESET, stdout);
}

/* ---------- Messages ---------- */
void pr_msg(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
}

void pr_err(const char *fmt, ...) {
    if (g_color) fputs(C_BRED, stderr);
    fputs("codoco: ", stderr);
    if (g_color) fputs(C_RESET, stderr);
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
}
