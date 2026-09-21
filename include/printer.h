#ifndef CODOCO_PRINTER_H
#define CODOCO_PRINTER_H

#include <stddef.h>
#include <stdio.h>

typedef enum { COLOR_AUTO = 0, COLOR_ALWAYS, COLOR_NEVER } ColorMode;

/* ANSI colour shortcuts */
#define C_RESET   "\033[0m"
#define C_BOLD    "\033[1m"
#define C_DIM     "\033[2m"
#define C_RED     "\033[31m"
#define C_GREEN   "\033[32m"
#define C_YELLOW  "\033[33m"
#define C_BLUE    "\033[34m"
#define C_MAGENTA "\033[35m"
#define C_CYAN    "\033[36m"
#define C_WHITE   "\033[37m"
#define C_BRED    "\033[91m"
#define C_BGREEN  "\033[92m"
#define C_BYELLOW "\033[93m"
#define C_BBLUE   "\033[94m"
#define C_BMAG    "\033[95m"
#define C_BCYAN   "\033[96m"
#define C_GRAY    "\033[90m"

/* Alignment */
#define AL_LEFT   0
#define AL_RIGHT  1
#define AL_CENTER 2

void pr_init(ColorMode mode, int ascii_only);
int  pr_width(void);
int  pr_color(void);

/* Box drawing (Unicode or ASCII fallback) */
void pr_box_top(void);
void pr_box_bottom(void);
void pr_box_sep(void);
void pr_box_blank(void);
void pr_box_center(const char *s);
void pr_box_left(const char *s);
void pr_box_kv(const char *k, const char *v);
void pr_box_wrapped(const char *text, int indent);

/* Visible-width aware printing */
int  pr_vislen(const char *s);
void pr_print_trimmed(const char *s, int maxw);

/* Table */
void pr_table_header(const char **cols, const int *widths, const int *aligns, int n);
void pr_table_row(const char **cells, const int *widths, const int *aligns, int n);
void pr_table_sep(const int *widths, int n);

/* Bars */
void pr_bar(double pct, int width, const char *color);

/* Messages */
void pr_msg(const char *fmt, ...);
void pr_err(const char *fmt, ...);

#endif /* CODOCO_PRINTER_H */
