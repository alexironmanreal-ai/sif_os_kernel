#ifndef SIF_NS_H
#define SIF_NS_H

#include <stdint.h>

/*
 * SIF Namespace — own design.
 * Paths use '.' as separator:  apps.hello.elf   docs.motd
 * Not POSIX. Not Windows. Cells hold streams of bytes.
 */

#define NS_PATH_LEN 40
#define NS_MAX_CELLS 48

void ns_init(void);

/* 0 ok, -1 error */
int ns_valid_path(const char *path);
int ns_exists(const char *path);
int ns_forge(const char *path);                          /* empty cell */
int ns_scribe(const char *path, const void *data, uint32_t n);
int ns_glance(const char *path, void *buf, uint32_t max, uint32_t *out);
int ns_erase(const char *path);
int ns_cells(const char *prefix);                        /* list to console */

/* resolve to underlying store key (for exec of ELFs) */
const char *ns_store_key(const char *path);

#endif
