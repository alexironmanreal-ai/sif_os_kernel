#ifndef SIF_DEV_H
#define SIF_DEV_H

#include <stdint.h>

/*
 * SIF device registry — own model.
 * Each unit has a tag, kind, and present flag.
 */

#define DEV_MAX 16
#define DEV_TAG_LEN 16

enum dev_kind {
    DEV_KIND_INPUT = 1,
    DEV_KIND_STORAGE = 2,
    DEV_KIND_DISPLAY = 3,
    DEV_KIND_CLOCK = 4,
    DEV_KIND_BUS = 5,
    DEV_KIND_LINK = 6,
};

struct sif_dev {
    char tag[DEV_TAG_LEN];
    enum dev_kind kind;
    int present;
    const char *note;
};

void dev_init(void);
int dev_register(const char *tag, enum dev_kind kind, int present, const char *note);
int dev_set_present(const char *tag, int present);
void dev_list(void);
int dev_count_present(void);

#endif
