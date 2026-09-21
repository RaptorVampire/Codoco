/* Codoco - main
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 * https://github.com/RaptorVampire/Codoco
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "codoco.h"

void codoco_print_usage(void); /* from options.c */
#include <inttypes.h>

static double g_elapsed_seconds = 0.0;

/* ---- forward decls ---- */
static void print_banner(void);
static void print_help(void);
static void print_license(void);

static void print_banner(void) {
    char line[256];
    snprintf(line, sizeof line, "%s  v%s", CODOCO_NAME, CODOCO_VERSION);
    pr_box_top();
    pr_box_center(line);
    pr_box_center(CODOCO_TAGLINE);
    pr_box_sep();
    pr_box_kv("Home",    CODOCO_URL);
    pr_box_kv("License", CODOCO_LICENSE);
    pr_box_kv("Author",  CODOCO_AUTHOR);
    pr_box_bottom();
}

static void print_help(void) {
    print_banner();
    pr_box_blank();
    codoco_print_usage();
}

static void print_license(void) {
    printf("%s is dual-licensed. You may use it under either:\n\n", CODOCO_NAME);
    printf("  1. Apache License, Version 2.0\n");
    printf("  2. MIT License\n\n");
    printf("SPDX-License-Identifier: Apache-2.0 OR MIT\n");
    printf("Full texts are in LICENSE-APACHE and LICENSE-MIT.\n");
}

/* ---- formatting helpers ---- */
static void csv_escape(const char *s, char *out, size_t n) {
    size_t j = 0;
    out[j++] = '"';
    for (size_t i = 0; s[i] && j + 2 < n; i++) {
        if (s[i] == '"') { if (j + 2 < n) out[j++] = '"'; }
        out[j++] = s[i];
    }
    out[j++] = '"';
    out[j] = 0;
}

static void json_escape(const char *s, char *out, size_t n) {
    size_t j = 0;
    for (size_t i = 0; s[i] && j + 2 < n; i++) {
        unsigned char c = (unsigned char)s[i];
        if (c == '"' || c == '\\') { out[j++] = '\\'; out[j++] = (char)c; }
        else if (c == '\n') { out[j++] = '\\'; out[j++] = 'n'; }
        else if (c == '\r') { out[j++] = '\\'; out[j++] = 'r'; }
        else if (c == '\t') { out[j++] = '\\'; out[j++] = 't'; }
        else if (c < 0x20) { j += (size_t)snprintf(out + j, n - j, "\\u%04x", c); }
        else out[j++] = (char)c;
    }
    out[j] = 0;
}

static void xml_escape(const char *s, char *out, size_t n) {
    size_t j = 0;
    for (size_t i = 0; s[i] && j + 8 < n; i++) {
        switch (s[i]) {
            case '&': j += (size_t)snprintf(out + j, n - j, "&amp;"); break;
            case '<': j += (size_t)snprintf(out + j, n - j, "&lt;"); break;
            case '>': j += (size_t)snprintf(out + j, n - j, "&gt;"); break;
            case '"': j += (size_t)snprintf(out + j, n - j, "&quot;"); break;
            case '\'': j += (size_t)snprintf(out + j, n - j, "&apos;"); break;
            default: out[j++] = s[i];
        }
    }
    out[j] = 0;
}

/* ---- output formats ---- */

static void output_json(const ScanResult *r, const Options *opt) {
    printf("{\n");
    printf("  \"tool\": \"%s\",\n", CODOCO_NAME);
    printf("  \"version\": \"%s\",\n", CODOCO_VERSION);
    printf("  \"url\": \"%s\",\n", CODOCO_URL);
    printf("  \"license\": \"%s\",\n", CODOCO_LICENSE);
    printf("  \"summary\": {\n");
    printf("    \"files\":    %" PRIu64 ",\n", r->langs.total_files);
    printf("    \"lines\":    %" PRIu64 ",\n", r->langs.total_lines);
    printf("    \"code\":     %" PRIu64 ",\n", r->langs.total_code);
    printf("    \"comments\": %" PRIu64 ",\n", r->langs.total_comments);
    printf("    \"blanks\":   %" PRIu64 ",\n", r->langs.total_blanks);
    printf("    \"bytes\":    %" PRIu64 "\n",  r->langs.total_bytes);
    printf("  },\n");
    printf("  \"languages\": [\n");
    int n = r->langs.count;
    if (opt->top_n > 0 && opt->top_n < n) n = opt->top_n;
    for (int i = 0; i < n; i++) {
        const LangEntry *e = &r->langs.items[i];
        char esc[512];
        json_escape(e->name, esc, sizeof esc);
        printf("    {\"name\":\"%s\",\"files\":%" PRIu64 ",\"lines\":%" PRIu64
               ",\"code\":%" PRIu64 ",\"comments\":%" PRIu64
               ",\"blanks\":%" PRIu64 ",\"bytes\":%" PRIu64 "}%s\n",
               esc, e->files, e->lines, e->code, e->comments, e->blanks, e->bytes,
               (i + 1 < n) ? "," : "");
    }
    printf("  ]");
    if (opt->by_file) {
        printf(",\n  \"files\": [\n");
        for (int i = 0; i < r->files.count; i++) {
            const FileEntry *f = &r->files.items[i];
            char pe[1024], le[128];
            json_escape(f->path, pe, sizeof pe);
            json_escape(f->lang, le, sizeof le);
            printf("    {\"path\":\"%s\",\"lang\":\"%s\",\"lines\":%" PRIu64
                   ",\"code\":%" PRIu64 ",\"comments\":%" PRIu64
                   ",\"blanks\":%" PRIu64 ",\"bytes\":%" PRIu64 "}%s\n",
                   pe, le, f->lines, f->code, f->comments, f->blanks, f->bytes,
                   (i + 1 < r->files.count) ? "," : "");
        }
        printf("  ]");
    }
    printf("\n}\n");
}

static void output_csv(const ScanResult *r, const Options *opt) {
    printf("Language,Files,Lines,Code,Comments,Blanks,Bytes,Percent\n");
    int n = r->langs.count;
    if (opt->top_n > 0 && opt->top_n < n) n = opt->top_n;
    for (int i = 0; i < n; i++) {
        const LangEntry *e = &r->langs.items[i];
        char esc[256];
        csv_escape(e->name, esc, sizeof esc);
        double pct = r->langs.total_lines ?
            (100.0 * (double)e->lines / (double)r->langs.total_lines) : 0.0;
        printf("%s,%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%.2f\n",
               esc, e->files, e->lines, e->code, e->comments, e->blanks, e->bytes, pct);
    }
    if (opt->show_total) {
        double pct = r->langs.total_lines ? 100.0 : 0.0;
        printf("TOTAL,%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%.2f\n",
               r->langs.total_files, r->langs.total_lines, r->langs.total_code,
               r->langs.total_comments, r->langs.total_blanks, r->langs.total_bytes, pct);
    }
    if (opt->by_file) {
        printf("\nPath,Language,Lines,Code,Comments,Blanks,Bytes\n");
        for (int i = 0; i < r->files.count; i++) {
            const FileEntry *f = &r->files.items[i];
            char pe[1024], le[128];
            csv_escape(f->path, pe, sizeof pe);
            csv_escape(f->lang, le, sizeof le);
            printf("%s,%s,%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64 "\n",
                   pe, le, f->lines, f->code, f->comments, f->blanks, f->bytes);
        }
    }
}

static void output_xml(const ScanResult *r, const Options *opt) {
    printf("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    printf("<codoco tool=\"%s\" version=\"%s\" url=\"%s\" license=\"%s\">\n",
           CODOCO_NAME, CODOCO_VERSION, CODOCO_URL, CODOCO_LICENSE);
    printf("  <summary files=\"%" PRIu64 "\" lines=\"%" PRIu64
           "\" code=\"%" PRIu64 "\" comments=\"%" PRIu64
           "\" blanks=\"%" PRIu64 "\" bytes=\"%" PRIu64 "\"/>\n",
           r->langs.total_files, r->langs.total_lines, r->langs.total_code,
           r->langs.total_comments, r->langs.total_blanks, r->langs.total_bytes);
    printf("  <languages>\n");
    int n = r->langs.count;
    if (opt->top_n > 0 && opt->top_n < n) n = opt->top_n;
    for (int i = 0; i < n; i++) {
        const LangEntry *e = &r->langs.items[i];
        char esc[256];
        xml_escape(e->name, esc, sizeof esc);
        double pct = r->langs.total_lines ?
            (100.0 * (double)e->lines / (double)r->langs.total_lines) : 0.0;
        printf("    <language name=\"%s\" files=\"%" PRIu64 "\" lines=\"%" PRIu64
               "\" code=\"%" PRIu64 "\" comments=\"%" PRIu64
               "\" blanks=\"%" PRIu64 "\" bytes=\"%" PRIu64 "\" percent=\"%.2f\"/>\n",
               esc, e->files, e->lines, e->code, e->comments, e->blanks, e->bytes, pct);
    }
    printf("  </languages>\n");
    if (opt->by_file) {
        printf("  <files>\n");
        for (int i = 0; i < r->files.count; i++) {
            const FileEntry *f = &r->files.items[i];
            char pe[1024], le[128];
            xml_escape(f->path, pe, sizeof pe);
            xml_escape(f->lang, le, sizeof le);
            printf("    <file path=\"%s\" lang=\"%s\" lines=\"%" PRIu64
                   "\" code=\"%" PRIu64 "\" comments=\"%" PRIu64
                   "\" blanks=\"%" PRIu64 "\" bytes=\"%" PRIu64 "\"/>\n",
                   pe, le, f->lines, f->code, f->comments, f->blanks, f->bytes);
        }
        printf("  </files>\n");
    }
    printf("</codoco>\n");
}

static void output_table(const ScanResult *r, const Options *opt) {
    print_banner();

    if (!opt->quiet) {
        printf("\n");
        pr_box_top();
        pr_box_center("Scan Statistics");
        pr_box_sep();
        char buf[64];
        uint64_t seen = r->files_scanned + r->files_skipped + r->files_binary;
        fmt_num(seen, buf, sizeof buf);
        pr_box_kv("Files scanned", buf);
        fmt_num(r->files_scanned, buf, sizeof buf);
        pr_box_kv("  counted",     buf);
        fmt_num(r->files_skipped, buf, sizeof buf);
        pr_box_kv("  skipped",     buf);
        fmt_num(r->files_binary,  buf, sizeof buf);
        pr_box_kv("  binary",      buf);
        fmt_num(r->errors,        buf, sizeof buf);
        pr_box_kv("  errors",      buf);
        char b2[64];
        fmt_bytes(r->langs.total_bytes, b2, sizeof b2);
        pr_box_kv("Total bytes",   b2);
        {
            char tbuf[64];
            snprintf(tbuf, sizeof tbuf, "%.3f s", g_elapsed_seconds);
            pr_box_kv("Elapsed",       tbuf);
        }
        pr_box_bottom();
    }

    printf("\n");

    /* Table of languages */
    int name_w = 22;
    int cols[7] = { name_w, 8, 10, 10, 10, 10, 12 };
    const char *hdr[7] = { "Language", "Files", "Lines", "Code", "Comments", "Blanks", "Bytes" };
    int aligns[7] = { AL_LEFT, AL_RIGHT, AL_RIGHT, AL_RIGHT, AL_RIGHT, AL_RIGHT, AL_RIGHT };

    int total_w = 1;
    for (int i = 0; i < 7; i++) total_w += cols[i] + 1;
    /* Adaptive layout: drop trailing columns (Bytes, Blanks, ...)
       until the table fits, then shrink Language if still needed. */
    int n_cols = 7;
    while (n_cols > 3 && total_w > pr_width()) {
        n_cols--;
        total_w -= cols[n_cols] + 1;
    }
    while (total_w > pr_width() && cols[0] > 6) {
        cols[0]--;
        total_w--;
    }

    if (opt->show_header) {
        pr_table_header(hdr, cols, aligns, n_cols);
        pr_table_sep(cols, n_cols);
    }

    int n = r->langs.count;
    if (opt->top_n > 0 && opt->top_n < n) n = opt->top_n;

    for (int i = 0; i < n; i++) {
        const LangEntry *e = &r->langs.items[i];
        char nf[32], nl[32], nc[32], nm[32], nb[32], nby[64], pct[32];
        fmt_num(e->files, nf, sizeof nf);
        fmt_num(e->lines, nl, sizeof nl);
        fmt_num(e->code, nc, sizeof nc);
        fmt_num(e->comments, nm, sizeof nm);
        fmt_num(e->blanks, nb, sizeof nb);
        fmt_bytes(e->bytes, nby, sizeof nby);
        double p = r->langs.total_lines ? (100.0 * (double)e->lines / (double)r->langs.total_lines) : 0.0;
        fmt_pct(p, pct, sizeof pct);
        (void)pct;

        char name_col[128];
        snprintf(name_col, sizeof name_col, "%s%s%s",
                 pr_color() ? C_BCYAN : "", e->name, pr_color() ? C_RESET : "");

        const char *cells[7] = { name_col, nf, nl, nc, nm, nb, nby };
        pr_table_row(cells, cols, aligns, n_cols);
    }

    if (opt->show_total) {
        pr_table_sep(cols, 7);
        char nf[32], nl[32], nc[32], nm[32], nb[32], nby[64];
        fmt_num(r->langs.total_files, nf, sizeof nf);
        fmt_num(r->langs.total_lines, nl, sizeof nl);
        fmt_num(r->langs.total_code, nc, sizeof nc);
        fmt_num(r->langs.total_comments, nm, sizeof nm);
        fmt_num(r->langs.total_blanks, nb, sizeof nb);
        fmt_bytes(r->langs.total_bytes, nby, sizeof nby);
        const char *bold_tot = pr_color() ? C_BOLD "TOTAL" C_RESET : "TOTAL";
        const char *cells[7] = { bold_tot, nf, nl, nc, nm, nb, nby };
        pr_table_row(cells, cols, aligns, n_cols);
    }

    /* Percentages + bars */
    if (opt->show_percent && r->langs.total_lines && n > 0) {
        printf("\n");
        if (pr_color()) fputs(C_BOLD, stdout);
        printf("  Language distribution\n");
        if (pr_color()) fputs(C_RESET, stdout);

        int barw = pr_width() - 36;
        if (barw < 10) barw = 10;
        if (barw > 60) barw = 60;

        for (int i = 0; i < n; i++) {
            const LangEntry *e = &r->langs.items[i];
            double p = (100.0 * (double)e->lines / (double)r->langs.total_lines);
            char pct[32]; fmt_pct(p, pct, sizeof pct);
            char nm[64]; snprintf(nm, sizeof nm, "%-22.22s", e->name);
            printf("  %s%s%s ", pr_color() ? C_BCYAN : "", nm, pr_color() ? C_RESET : "");
            if (opt->show_bar) {
                pr_bar(p, barw, C_BGREEN);
                putchar(' ');
            }
            printf("%s\n", pct);
        }
    }

    /* By-file report */
    if (opt->by_file && r->files.count > 0) {
        printf("\n");
        if (pr_color()) fputs(C_BOLD, stdout);
        printf("  Per-file report\n");
        if (pr_color()) fputs(C_RESET, stdout);

        int w0 = 12, w1 = 22, w2 = 8, w3 = 8, w4 = 9, w5 = 8, w6 = 10;
        int widths[7] = { w0, w1, w2, w3, w4, w5, w6 };
        const char *pf_hdr[7] = { "Lines", "Path", "Code", "Comments", "Blanks", "Bytes", "Language" };
        int pf_aligns[7] = { AL_RIGHT, AL_LEFT, AL_RIGHT, AL_RIGHT, AL_RIGHT, AL_RIGHT, AL_LEFT };

        int total = 1;
        for (int i = 0; i < 7; i++) total += widths[i] + 1;
        int over = total - pr_width();
        if (over > 0) { widths[1] -= over; if (widths[1] < 12) widths[1] = 12; }

        if (opt->show_header) {
            pr_table_header(pf_hdr, widths, pf_aligns, 7);
            pr_table_sep(widths, 7);
        }

        int fn = r->files.count;
        if (opt->top_n > 0 && opt->top_n < fn) fn = opt->top_n;

        for (int i = 0; i < fn; i++) {
            const FileEntry *f = &r->files.items[i];
            char nl[32], nc[32], nm[32], nb[32], nby[64];
            fmt_num(f->lines, nl, sizeof nl);
            fmt_num(f->code, nc, sizeof nc);
            fmt_num(f->comments, nm, sizeof nm);
            fmt_num(f->blanks, nb, sizeof nb);
            fmt_bytes(f->bytes, nby, sizeof nby);
            const char *cells[7] = { nl, f->path, nc, nm, nb, nby, f->lang };
            pr_table_row(cells, widths, pf_aligns, 7);
        }
    }

    /* footer */
    printf("\n");
    if (pr_color()) fputs(C_GRAY, stdout);
    printf("  %s v%s - %s\n", CODOCO_NAME, CODOCO_VERSION, CODOCO_URL);
    if (pr_color()) fputs(C_RESET, stdout);
}




/* === CODOCO_HTML_BEGIN === */
/* Codoco -- HTML report (v5.1)
 * Pure SVG donut with pathLength normalization; footer glued to bottom.
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

static void html_escape(const char *s, char *out, size_t n) {
    size_t j = 0;
    for (size_t i = 0; s[i] && j + 8 < n; i++) {
        switch (s[i]) {
            case '&':  j += (size_t)snprintf(out + j, n - j, "&amp;");  break;
            case '<':  j += (size_t)snprintf(out + j, n - j, "&lt;");   break;
            case '>':  j += (size_t)snprintf(out + j, n - j, "&gt;");   break;
            case '"':  j += (size_t)snprintf(out + j, n - j, "&quot;"); break;
            case '\'': j += (size_t)snprintf(out + j, n - j, "&#39;");  break;
            default:   out[j++] = s[i];
        }
    }
    out[j] = 0;
}

/* CSS split across multiple string literals to stay below
 * ISO C99's 4095-character minimum for a single string literal.
 * At runtime the parts are concatenated by html_emit_css(). */
static const char *HTML_CSS_PARTS[] = {
    ":root{\n"
    "  color-scheme: dark;\n"
    "  --bg:#0a0e14;--bg2:#0d1117;--card:#151b23;--card2:#1c232c;\n"
    "  --border:#30363d;--border2:#21262d;\n"
    "  --text:#e6edf3;--strong:#f0f6fc;--muted:#7d8590;\n"
    "  --accent:#58a6ff;--purple:#bc8cff;\n"
    "  --green:#3fb950;--yellow:#d29922;--orange:#ffa657;\n"
    "}\n"
    "html.light{\n"
    "  color-scheme: light;\n"
    "  --bg:#f6f8fa;--bg2:#ffffff;--card:#ffffff;--card2:#f3f5f8;\n"
    "  --border:#d0d7de;--border2:#e5e8ec;\n"
    "  --text:#1f2328;--strong:#0d1117;--muted:#656d76;\n"
    "  --accent:#0969da;--purple:#8250df;\n"
    "  --green:#1a7f37;--yellow:#9a6700;--orange:#bc4c00;\n"
    "}\n"
    "*,*::before,*::after{box-sizing:border-box}\n"
    "html,body{margin:0;padding:0}\n"
    "html{background:var(--bg)}\n"
    "body{\n"
    "  background:var(--bg);color:var(--text);\n"
    "  font-family:'Inter',-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;\n"
    "  font-size:13px;line-height:1.5;padding:14px;\n"
    "  -webkit-font-smoothing:antialiased;text-rendering:optimizeLegibility;\n"
    "}\n"
    "code,.mono{font-family:'JetBrains Mono',ui-monospace,Menlo,Consolas,monospace;font-variant-numeric:tabular-nums}\n"
    "\n"
    ".page{\n"
    "  max-width:1180px;margin:0 auto;\n"
    "  min-height:calc(100vh - 28px);\n"
    "  display:flex;flex-direction:column;gap:14px;\n"
    "}\n"
    "\n"
    "/* banner */\n"
    ".banner{\n"
    "  display:flex;justify-content:space-between;align-items:center;gap:14px;\n"
    "  padding:16px 20px;border:1px solid var(--border);border-radius:12px;\n"
    "  background:linear-gradient(135deg,var(--card) 0%,var(--card2) 100%);\n"
    "}\n"
    ".brand .name{font-size:26px;font-weight:700;color:var(--strong);\n"
    "  letter-spacing:-.02em;line-height:1}\n"
    ".brand .name .dot{background:linear-gradient(135deg,var(--accent),var(--purple));\n"
    "  -webkit-background-clip:text;background-clip:text;color:transparent}\n"
    ".brand .tag{font-size:11px;color:var(--muted);margin-top:4px}\n"
    ".meta{text-align:right;font-size:11px;color:var(--muted);line-height:1.7}\n"
    ".meta strong{color:var(--text);font-weight:600}\n"
    ".meta code{background:var(--bg2);padding:2px 6px;border-radius:4px;\n"
    "  color:var(--orange);font-size:10px}\n"
    "\n"
    "/* summary */\n"
    ".summary{display:grid;grid-template-columns:repeat(6,1fr);gap:10px}\n"
    ".stat{background:var(--card);border:1px solid var(--border);border-radius:10px;\n"
    "  padding:12px 14px;position:relative;overflow:hidden}\n"
    ".stat::before{content:\"\";position:absolute;left:0;top:0;bottom:0;width:3px;\n"
    "  background:linear-gradient(180deg,var(--accent),var(--purple))}\n"
    ".stat.green::before{background:var(--green)}\n"
    ".stat.yellow::before{background:var(--yellow)}\n"
    ".stat.purple::before{background:var(--purple)}\n"
    ".stat.orange::before{background:var(--orange)}\n"
    ".stat .n{display:block;font-size:20px;font-weight:700;color:var(--strong);\n"
    "  line-height:1;font-variant-numeric:tabular-nums}\n"
    ".stat .l{display:block;font-size:9px;color:var(--muted);text-transform:uppercase;\n"
    "  letter-spacing:.1em;margin-top:5px;font-weight:600}\n"
    "\n"
    "/* main grid */\n"
    ".main{display:grid;grid-template-columns:minmax(0,1fr) minmax(0,1.4fr);\n"
    "  gap:14px;align-items:start}\n"
    ".panel{background:var(--card);border:1px solid var(--border);border-radius:12px;\n"
    "  padding:14px 16px;min-width:0}\n"
    ".panel h2{font-size:10px;font-weight:600;text-transform:uppercase;\n"
    "  letter-spacing:.15em;color:var(--muted);margin:0 0 10px 0;\n"
    "  display:flex;align-items:center;gap:8px}\n"
    ".panel h2::after{content:\"\";flex:1;height:1px;\n"
    "  background:linear-gradient(90deg,var(--border),transparent)}\n"
    "\n"
    "/* donut */\n",
    ".donut-wrap{position:relative;width:100%;max-width:240px;margin:0 auto;\n"
    "  aspect-ratio:1/1}\n"
    ".donut{width:100%;height:100%;display:block;overflow:visible;color:var(--text)}\n"
    ".donut circle{transition:opacity .15s}\n"
    ".donut circle:hover{opacity:.7}\n"
    ".donut-center{position:absolute;inset:0;display:flex;flex-direction:column;\n"
    "  align-items:center;justify-content:center;pointer-events:none}\n"
    ".donut-center .big{font-size:26px;font-weight:700;color:var(--strong);\n"
    "  line-height:1;font-variant-numeric:tabular-nums}\n"
    ".donut-center .small{font-size:9px;color:var(--muted);text-transform:uppercase;\n"
    "  letter-spacing:.15em;margin-top:3px}\n"
    "\n"
    "/* legend */\n"
    ".legend{list-style:none;padding:0;margin:14px 0 0 0;display:grid;\n"
    "  grid-template-columns:repeat(2,minmax(0,1fr));gap:3px 12px;font-size:11px}\n"
    ".legend li{display:flex;align-items:center;gap:7px;padding:2px 0;min-width:0}\n"
    ".legend li i{width:9px;height:9px;border-radius:3px;flex-shrink:0}\n"
    ".legend li .nm{flex:1;white-space:nowrap;overflow:hidden;\n"
    "  text-overflow:ellipsis;color:var(--text);font-weight:500}\n"
    ".legend li .pc{color:var(--muted);font-family:'JetBrains Mono',monospace;\n"
    "  font-size:10px;font-variant-numeric:tabular-nums}\n"
    "\n"
    "/* table */\n"
    "table{width:100%;border-collapse:collapse;font-size:11px}\n"
    "thead th{text-align:left;padding:7px 9px;font-size:9px;font-weight:600;\n"
    "  text-transform:uppercase;letter-spacing:.08em;color:var(--muted);\n"
    "  border-bottom:1px solid var(--border)}\n"
    "thead th.num{text-align:right}\n"
    "tbody td{padding:6px 9px;border-bottom:1px solid var(--border2)}\n"
    "tbody tr:last-child td{border-bottom:none}\n"
    "tbody td.num{text-align:right;font-family:'JetBrains Mono',monospace;\n"
    "  font-variant-numeric:tabular-nums;color:var(--text)}\n"
    "tbody td.lang{font-weight:600;color:var(--strong)}\n"
    "tbody tr.total td{font-weight:700;color:var(--strong);\n"
    "  border-top:1px solid var(--border);padding-top:8px}\n"
    "\n"
    "/* footer: glued to the bottom */\n"
    "footer{\n"
    "  margin-top:auto;\n"
    "  display:flex;justify-content:space-between;align-items:flex-end;gap:12px;\n"
    "  font-size:10px;color:var(--muted);\n"
    "  padding:14px 0 4px;border-top:1px solid var(--border);\n"
    "}\n"
    "footer a{color:var(--accent);text-decoration:none}\n"
    "\n"
    "/* mobile */\n"
    "@media (max-width:760px){\n"
    "  body{padding:12px}\n"
    "  .page{min-height:calc(100vh - 24px);gap:12px}\n"
    "  .banner{flex-direction:column;align-items:flex-start;padding:14px 16px}\n"
    "  .meta{text-align:left}\n"
    "  .brand .name{font-size:22px}\n"
    "  .summary{grid-template-columns:repeat(2,1fr);gap:8px}\n"
    "  .stat{padding:10px 12px}\n"
    "  .stat .n{font-size:17px}\n"
    "  .main{grid-template-columns:1fr;gap:12px}\n"
    "  .donut-wrap{max-width:220px}\n"
    "  .legend{grid-template-columns:1fr}\n"
    "  table{font-size:11px}\n"
    "  thead th{padding:6px 8px}\n"
    "  tbody td{padding:5px 8px}\n"
    "}\n"
    "@media (max-width:420px){\n"
    "  .summary{grid-template-columns:1fr 1fr}\n"
    "  .col-bytes{display:none}\n"
    "  footer{flex-direction:column;align-items:flex-start;gap:4px}\n"
    "}\n"
    "\n"
    "/* print: single A4 landscape */\n"
    "@media print{\n"
    "  @page{size:A4 landscape;margin:6mm}\n"
    "  html,body{background:var(--bg)!important;color:var(--text)!important}\n"
    "  body{padding:0;font-size:8pt;line-height:1.3}\n"
    "  *{-webkit-print-color-adjust:exact!important;print-color-adjust:exact!important}\n"
    "  .page{max-width:100%;gap:2.5mm;min-height:190mm}\n"
    "  .banner{padding:2.5mm 3.5mm;border-radius:2mm}\n"
    "  .brand .name{font-size:15pt}\n",
    "  .brand .tag{font-size:6.5pt;margin-top:.3mm}\n"
    "  .meta{font-size:6.5pt;line-height:1.5}\n"
    "  .meta code{font-size:6pt;padding:1px 3px}\n"
    "  .summary{gap:1.5mm}\n"
    "  .stat{padding:1.8mm 2mm;border-radius:1.5mm}\n"
    "  .stat::before{width:2px}\n"
    "  .stat .n{font-size:10pt}\n"
    "  .stat .l{font-size:5pt;margin-top:1mm;letter-spacing:.06em}\n"
    "  .main{grid-template-columns:2fr 3fr;gap:2.5mm}\n"
    "  .panel{padding:2mm 2.5mm;border-radius:2mm}\n"
    "  .panel h2{font-size:6.5pt;margin:0 0 1.2mm 0;letter-spacing:.08em}\n"
    "  .donut-wrap{max-width:52mm;margin:0 auto}\n"
    "  .donut-center .big{font-size:14pt}\n"
    "  .donut-center .small{font-size:5.5pt;letter-spacing:.1em;margin-top:2px}\n"
    "  .legend{margin-top:1.8mm;gap:.5mm 3mm;font-size:6.5pt}\n"
    "  .legend li{padding:1px 0;gap:5px}\n"
    "  .legend li i{width:6px;height:6px;border-radius:2px}\n"
    "  .legend li .pc{font-size:6pt}\n"
    "  table{font-size:7pt;line-height:1.2}\n"
    "  thead th{padding:1.1mm 1.4mm;font-size:5.5pt}\n"
    "  tbody td{padding:.9mm 1.4mm}\n"
    "  tbody tr.total td{padding-top:1.3mm}\n"
    "  footer{font-size:6pt;padding:2mm 0 0;margin-top:auto;\n"
    "    border-top:1px solid var(--border)}\n"
    "  section,.banner,.panel,table,tr,svg{break-inside:avoid;page-break-inside:avoid}\n"
    "}\n"
    "/* density for many languages */\n"
    "body.dense table{font-size:7pt}\n"
    "body.x-dense table{font-size:6pt}\n"
    "@media print{\n"
    "  body.dense table{font-size:6pt;line-height:1.15}\n"
    "  body.dense tbody td{padding:.6mm 1.2mm}\n"
    "  body.dense thead th{padding:.9mm 1.2mm;font-size:5pt}\n"
    "  body.dense .legend{font-size:6pt}\n"
    "  body.dense .donut-wrap{max-width:46mm}\n"
    "  body.x-dense table{font-size:5.5pt;line-height:1.1}\n"
    "  body.x-dense tbody td{padding:.45mm 1mm}\n"
    "  body.x-dense thead th{padding:.7mm 1mm;font-size:4.5pt}\n"
    "  body.x-dense .legend{font-size:5.5pt}\n"
    "  body.x-dense .donut-wrap{max-width:42mm}\n"
    "  body.x-dense .legend li .pc{font-size:5pt}\n"
    "}\n"
    "",
    NULL
};

/* Emit all CSS parts in order. */
static void html_emit_css(FILE *fp) {
    for (int i = 0; HTML_CSS_PARTS[i]; i++)
        fputs(HTML_CSS_PARTS[i], fp);
}

static const char *HTML_PALETTE[] = {
    "#58a6ff","#bc8cff","#3fb950","#d29922","#ffa657",
    "#f778ba","#39d353","#79c0ff","#d2a8ff","#7ee787",
    "#ffa657","#ff7b72","#a5d6ff","#f0883e","#8b949e",
    NULL
};
static const int HTML_PALETTE_N = 15;

/* One donut segment.
   viewBox 100x100, center (50,50), r=38.
   pathLength="100" normalizes the circumference to 100 units so that
   stroke-dasharray values map 1:1 to percentages. */
static void html_donut_seg(double pct, double cum,
                           const char *color, const char *name,
                           uint64_t lines)
{
    char esc[256];
    html_escape(name, esc, sizeof esc);
    printf("<circle cx=\"50\" cy=\"50\" r=\"38\" fill=\"none\" "
           "stroke=\"%s\" stroke-width=\"14\" pathLength=\"100\" "
           "stroke-dasharray=\"%.4f %.4f\" "
           "stroke-dashoffset=\"%.4f\" "
           "transform=\"rotate(-90 50 50)\" "
           "stroke-linecap=\"butt\">"
           "<title>%s: %.2f%% (%" PRIu64 " lines)</title>"
           "</circle>",
           color, pct, 100.0 - pct, -cum, esc, pct, lines);
}

static void html_write_head(const Options *opt, const char *title, int theme,
                            int density_class)
{
    (void)opt;
    char esc[512];
    html_escape(title, esc, sizeof esc);
    fputs("<!DOCTYPE html>\n", stdout);
    printf("<html lang=\"en\" class=\"%s\">\n<head>\n",
           theme == 2 ? "light" : "dark");
    fputs("<meta charset=\"utf-8\">\n", stdout);
    fputs("<meta name=\"viewport\" content=\"width=device-width,"
          "initial-scale=1,viewport-fit=cover\">\n", stdout);
    printf("<title>Codoco -- %s</title>\n", esc);
    fputs("<link rel=\"preconnect\" href=\"https://fonts.googleapis.com\">\n", stdout);
    fputs("<link rel=\"preconnect\" href=\"https://fonts.gstatic.com\" crossorigin>\n", stdout);
    fputs("<link href=\"https://fonts.googleapis.com/css2?"
          "family=Inter:wght@400;500;600;700&"
          "family=JetBrains+Mono:wght@400;500;600&"
          "display=swap\" rel=\"stylesheet\">\n", stdout);
    fputs("<style>", stdout);
    html_emit_css(stdout);
    fputs("</style>\n</head>\n", stdout);
    fputs("<body", stdout);
    if (density_class == 1) fputs(" class=\"dense\"", stdout);
    else if (density_class == 2) fputs(" class=\"x-dense\"", stdout);
    fputs(">\n<div class=\"page\">\n", stdout);
}

static void html_write_banner(const ScanResult *r, const Options *opt)
{
    (void)r;
    char esc[1024], title[1024];
    if (opt->npaths == 1)      snprintf(title, sizeof title, "%s", opt->paths[0]);
    else if (opt->npaths > 1)  snprintf(title, sizeof title, "%s (+%d more)",
                                        opt->paths[0], opt->npaths - 1);
    else                       snprintf(title, sizeof title, ".");
    html_escape(title, esc, sizeof esc);

    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    char ts[64];
    if (tm) strftime(ts, sizeof ts, "%Y-%m-%d %H:%M", tm);
    else    snprintf(ts, sizeof ts, "-");

    fputs("<header class=\"banner\">\n", stdout);
    fputs("  <div class=\"brand\">\n", stdout);
    fputs("    <div class=\"name\">Codoco<span class=\"dot\">.</span></div>\n", stdout);
    fputs("    <div class=\"tag\">Count every line. Know every project.</div>\n", stdout);
    fputs("  </div>\n  <div class=\"meta\">\n", stdout);
    printf("    <div><strong>Path</strong> <code>%s</code></div>\n", esc);
    printf("    <div><strong>Generated</strong> %s</div>\n", ts);
    printf("    <div><strong>Codoco</strong> v%s &middot; %s</div>\n",
           CODOCO_VERSION, CODOCO_LICENSE);
    fputs("  </div>\n</header>\n", stdout);
}

static void html_write_summary(const ScanResult *r)
{
    char bf[32], bl[32], bc[32], bm[32], bk[32], bs[64];
    fmt_num(r->langs.total_files,    bf, sizeof bf);
    fmt_num(r->langs.total_lines,    bl, sizeof bl);
    fmt_num(r->langs.total_code,     bc, sizeof bc);
    fmt_num(r->langs.total_comments, bm, sizeof bm);
    fmt_num(r->langs.total_blanks,   bk, sizeof bk);
    fmt_bytes(r->langs.total_bytes,  bs, sizeof bs);
    fputs("<section class=\"summary\">\n", stdout);
    printf("  <div class=\"stat\"><span class=\"n\">%s</span><span class=\"l\">Files</span></div>\n", bf);
    printf("  <div class=\"stat\"><span class=\"n\">%s</span><span class=\"l\">Lines</span></div>\n", bl);
    printf("  <div class=\"stat green\"><span class=\"n\">%s</span><span class=\"l\">Code</span></div>\n", bc);
    printf("  <div class=\"stat yellow\"><span class=\"n\">%s</span><span class=\"l\">Comments</span></div>\n", bm);
    printf("  <div class=\"stat purple\"><span class=\"n\">%s</span><span class=\"l\">Blanks</span></div>\n", bk);
    printf("  <div class=\"stat orange\"><span class=\"n\">%s</span><span class=\"l\">Size</span></div>\n", bs);
    fputs("</section>\n", stdout);
}

static void html_write_chart_panel(const ScanResult *r, int n)
{
    uint64_t total = r->langs.total_lines ? r->langs.total_lines : 1;
    fputs("<div class=\"panel chart-panel\">\n", stdout);
    fputs("  <h2>Share by lines</h2>\n", stdout);

    fputs("  <div class=\"donut-wrap\">", stdout);
    fputs("<svg viewBox=\"0 0 100 100\" class=\"donut\" "
          "xmlns=\"http://www.w3.org/2000/svg\" role=\"img\" "
          "aria-label=\"Language distribution by lines\">", stdout);
    /* backdrop ring */
    fputs("<circle cx=\"50\" cy=\"50\" r=\"38\" fill=\"none\" "
          "stroke=\"currentColor\" stroke-opacity=\".08\" "
          "stroke-width=\"14\" pathLength=\"100\"/>", stdout);

    double cum = 0.0;
    for (int i = 0; i < n; i++) {
        double pct = 100.0 * (double)r->langs.items[i].lines / (double)total;
        if (pct < 0.15) continue;
        const char *c = HTML_PALETTE[i % HTML_PALETTE_N];
        html_donut_seg(pct, cum, c, r->langs.items[i].name,
                       r->langs.items[i].lines);
        cum += pct;
        if (cum >= 99.99) break;
    }
    fputs("</svg>", stdout);

    char totalstr[32];
    fmt_num(r->langs.total_lines, totalstr, sizeof totalstr);
    printf("<div class=\"donut-center\">"
           "<span class=\"big\">%s</span>"
           "<span class=\"small\">lines</span></div>",
           totalstr);
    fputs("  </div>\n", stdout);

    fputs("  <ul class=\"legend\">\n", stdout);
    double leg_cum = 0.0;
    int shown = 0;
    for (int i = 0; i < n; i++) {
        double pct = 100.0 * (double)r->langs.items[i].lines / (double)total;
        if (pct < 0.15) continue;
        if (shown >= 10) break;
        const char *c = HTML_PALETTE[i % HTML_PALETTE_N];
        char esc[256];
        html_escape(r->langs.items[i].name, esc, sizeof esc);
        printf("    <li><i style=\"background:%s\"></i>"
               "<span class=\"nm\">%s</span>"
               "<span class=\"pc\">%.2f%%</span></li>\n",
               c, esc, pct);
        leg_cum += pct;
        shown++;
    }
    if (shown < n && 100.0 - leg_cum > 0.15) {
        printf("    <li><i style=\"background:#8b949e\"></i>"
               "<span class=\"nm\">Other</span>"
               "<span class=\"pc\">%.2f%%</span></li>\n",
               100.0 - leg_cum);
    }
    fputs("  </ul>\n", stdout);
    fputs("</div>\n", stdout);
}

static void html_write_table_panel(const ScanResult *r, const Options *opt, int n)
{
    char esc[512];
    fputs("<div class=\"panel table-panel\">\n", stdout);
    fputs("  <h2>Languages</h2>\n", stdout);
    fputs("  <table>\n  <thead>\n    <tr>\n", stdout);
    fputs("      <th>Language</th>\n", stdout);
    fputs("      <th class=\"num\">Files</th>\n", stdout);
    fputs("      <th class=\"num\">Lines</th>\n", stdout);
    fputs("      <th class=\"num\">Code</th>\n", stdout);
    fputs("      <th class=\"num\">Comments</th>\n", stdout);
    fputs("      <th class=\"num\">Blanks</th>\n", stdout);
    fputs("      <th class=\"num col-bytes\">Bytes</th>\n", stdout);
    fputs("      <th class=\"num\">Share</th>\n", stdout);
    fputs("    </tr>\n  </thead>\n  <tbody>\n", stdout);

    for (int i = 0; i < n; i++) {
        const LangEntry *e = &r->langs.items[i];
        html_escape(e->name, esc, sizeof esc);
        char nf[32], nl[32], nc[32], nm[32], nb[32], nby[64], pct[16];
        fmt_num(e->files,    nf,  sizeof nf);
        fmt_num(e->lines,    nl,  sizeof nl);
        fmt_num(e->code,     nc,  sizeof nc);
        fmt_num(e->comments, nm,  sizeof nm);
        fmt_num(e->blanks,   nb,  sizeof nb);
        fmt_bytes(e->bytes,  nby, sizeof nby);
        double p = r->langs.total_lines
                 ? (100.0 * (double)e->lines / (double)r->langs.total_lines)
                 : 0.0;
        fmt_pct(p, pct, sizeof pct);
        fputs("    <tr>\n", stdout);
        printf("      <td class=\"lang\">%s</td>\n", esc);
        printf("      <td class=\"num\">%s</td>\n", nf);
        printf("      <td class=\"num\">%s</td>\n", nl);
        printf("      <td class=\"num\">%s</td>\n", nc);
        printf("      <td class=\"num\">%s</td>\n", nm);
        printf("      <td class=\"num\">%s</td>\n", nb);
        printf("      <td class=\"num col-bytes\">%s</td>\n", nby);
        printf("      <td class=\"num\">%s</td>\n", pct);
        fputs("    </tr>\n", stdout);
    }
    if (opt->show_total) {
        char nf[32], nl[32], nc[32], nm[32], nb[32], nby[64];
        fmt_num(r->langs.total_files,    nf,  sizeof nf);
        fmt_num(r->langs.total_lines,    nl,  sizeof nl);
        fmt_num(r->langs.total_code,     nc,  sizeof nc);
        fmt_num(r->langs.total_comments, nm,  sizeof nm);
        fmt_num(r->langs.total_blanks,   nb,  sizeof nb);
        fmt_bytes(r->langs.total_bytes,  nby, sizeof nby);
        fputs("    <tr class=\"total\">\n", stdout);
        fputs("      <td class=\"lang\">TOTAL</td>\n", stdout);
        printf("      <td class=\"num\">%s</td>\n", nf);
        printf("      <td class=\"num\">%s</td>\n", nl);
        printf("      <td class=\"num\">%s</td>\n", nc);
        printf("      <td class=\"num\">%s</td>\n", nm);
        printf("      <td class=\"num\">%s</td>\n", nb);
        printf("      <td class=\"num col-bytes\">%s</td>\n", nby);
        fputs("      <td class=\"num\">100.00%</td>\n", stdout);
        fputs("    </tr>\n", stdout);
    }
    fputs("  </tbody>\n  </table>\n</div>\n", stdout);
}

static void html_write_footer(void)
{
    fputs("<footer>\n", stdout);
    printf("  <div><strong>Codoco</strong> v%s &middot; %s</div>\n",
           CODOCO_VERSION, CODOCO_LICENSE);
    printf("  <div><a href=\"%s\">%s</a></div>\n", CODOCO_URL, CODOCO_URL);
    fputs("</footer>\n</div>\n</body>\n</html>\n", stdout);
}

static void output_html(const ScanResult *r, const Options *opt, int theme)
{
    int n = r->langs.count;
    if (opt->top_n > 0 && opt->top_n < n) n = opt->top_n;

    int density = 0;
    if      (n > 30) density = 2;
    else if (n > 15) density = 1;

    char title[1024];
    if (opt->npaths >= 1) snprintf(title, sizeof title, "%s", opt->paths[0]);
    else                  snprintf(title, sizeof title, "report");

    html_write_head(opt, title, theme, density);
    html_write_banner(r, opt);
    html_write_summary(r);
    fputs("<section class=\"main\">\n", stdout);
    html_write_chart_panel(r, n);
    html_write_table_panel(r, opt, n);
    fputs("</section>\n", stdout);
    html_write_footer();
}

static void make_theme_path(const char *user_out, const char *theme,
                            char *buf, size_t cap)
{
    if (!user_out || !*user_out) {
        snprintf(buf, cap, "codoco-report-%s.html", theme);
        return;
    }
    const char *ext = strrchr(user_out, '.');
    if (ext && !strcmp(ext, ".html")) {
        size_t base = (size_t)(ext - user_out);
        if (base + strlen(theme) + 8 < cap)
            snprintf(buf, cap, "%.*s-%s.html", (int)base, user_out, theme);
        else
            snprintf(buf, cap, "%s-%s.html", user_out, theme);
    } else {
        snprintf(buf, cap, "%s-%s.html", user_out, theme);
    }
}
/* === CODOCO_HTML_END === */

int main(int argc, char **argv) {
    Options opt;
    options_default(&opt);
    pr_init(COLOR_AUTO, 0); /* early init so --help cannot crash */

    if (options_parse(&opt, argc, argv) != 0) {
        fprintf(stderr, "Try 'codoco --help' for usage.\n");
        options_free(&opt);
        return 2;
    }

    if (opt.want_help)    { print_help(); options_free(&opt); return 0; }
    if (opt.want_version) { printf("%s %s\n", CODOCO_NAME, CODOCO_VERSION); options_free(&opt); return 0; }
    if (opt.want_license) { print_license(); options_free(&opt); return 0; }
    if (opt.want_list_langs) { lang_list_all(); options_free(&opt); return 0; }

    ColorMode cm = COLOR_AUTO;
    if (opt.no_color == 1)  cm = COLOR_NEVER;
    if (opt.no_color == -1) cm = COLOR_ALWAYS;
    pr_init(cm, opt.ascii_only);

    ScanResult res;
    scanner_init(&res);

    struct timespec _t0, _t1;
    clock_gettime(CLOCK_MONOTONIC, &_t0);
    scanner_run(&res, &opt);
    clock_gettime(CLOCK_MONOTONIC, &_t1);
    double elapsed = (_t1.tv_sec  - _t0.tv_sec) +
                     (_t1.tv_nsec - _t0.tv_nsec) / 1e9;
    g_elapsed_seconds = elapsed;

    stats_lang_sort(&res.langs, opt.sort_key, opt.sort_order);
    if (opt.by_file) stats_file_sort(&res.files, opt.sort_key, opt.sort_order);

    switch (opt.format) {
        case OUT_JSON: output_json(&res, &opt); break;
        case OUT_CSV:  output_csv(&res, &opt);  break;
        case OUT_XML:  output_xml(&res, &opt);  break;
        case OUT_HTML: {
            const char *user_out = opt.explicit_out;
            int want_dark  = (opt.html_theme == 0 || opt.html_theme == 1);
            int want_light = (opt.html_theme == 0 || opt.html_theme == 2);
            char path_dark[1024], path_light[1024];
            make_theme_path(user_out, "dark",  path_dark,  sizeof path_dark);
            make_theme_path(user_out, "light", path_light, sizeof path_light);
            if (want_dark) {
                if (!freopen(path_dark, "w", stdout)) {
                    fprintf(stderr, "codoco: cannot write %s\n", path_dark);
                } else {
                    output_html(&res, &opt, 1);
                    if (!opt.quiet) fprintf(stderr, "codoco: wrote %s\n", path_dark);
                }
            }
            if (want_light) {
                if (!freopen(path_light, "w", stdout)) {
                    fprintf(stderr, "codoco: cannot write %s\n", path_light);
                } else {
                    output_html(&res, &opt, 2);
                    if (!opt.quiet) fprintf(stderr, "codoco: wrote %s\n", path_light);
                }
            }
            break;
        }
        case OUT_TABLE:
        default:       output_table(&res, &opt); break;
    }

        options_free(&opt);
    scanner_free(&res);
    return 0;
}
