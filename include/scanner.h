#ifndef CODOCO_SCANNER_H
#define CODOCO_SCANNER_H

#include <stdint.h>

#include "options.h"
#include "stats.h"

typedef struct {
    LangArray langs;
    FileArray files;
    uint64_t  files_scanned;
    uint64_t  files_skipped;
    uint64_t  files_binary;
    uint64_t  errors;
    uint64_t  total_bytes_read;
} ScanResult;

void scanner_init(ScanResult *r);
void scanner_free(ScanResult *r);

int  scanner_run(ScanResult *r, const Options *opt);

int  scan_file(const char *path, const Options *opt,
               LangArray *langs, FileArray *files,
               uint64_t *bytes_read, int *was_binary);

#endif /* CODOCO_SCANNER_H */
