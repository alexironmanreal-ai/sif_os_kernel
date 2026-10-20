#ifndef SIF_NS_H
#define SIF_NS_H

#include <stdint.h>

#define NS_PATH_LEN 40
#define NS_MAX_CELLS 48

void ns_init(void);

int ns_valid_path(const char *path);
int ns_exists(const char *path);
int ns_forge(const char *path);
int ns_scribe(const char *path, const void *data, uint32_t n);
int ns_glance(const char *path, void *buf, uint32_t max, uint32_t *out);
int ns_erase(const char *path);
int ns_cells(const char *prefix);
int ns_link(const char *path, const char *key); /* bind path -> store key */
const char *ns_store_key(const char *path);

#endif
