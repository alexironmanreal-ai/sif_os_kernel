#include "ns.h"
#include "fs.h"
#include "string.h"
#include "printk.h"

/*
 * Namespace maps SIF paths -> store keys in the byte vault (fs).
 * Preloaded map for system cells; forge/scribe create path==key entries.
 */

struct ns_cell {
    char path[NS_PATH_LEN];
    char key[FS_NAME_LEN];
    int used;
};

static struct ns_cell cells[NS_MAX_CELLS];

static int find_path(const char *path) {
    for (int i = 0; i < NS_MAX_CELLS; i++)
        if (cells[i].used && strcmp(cells[i].path, path) == 0)
            return i;
    return -1;
}

static int free_slot(void) {
    for (int i = 0; i < NS_MAX_CELLS; i++)
        if (!cells[i].used)
            return i;
    return -1;
}

int ns_valid_path(const char *path) {
    if (!path || !path[0] || strlen(path) >= NS_PATH_LEN)
        return 0;
    if (path[0] == '.' || path[0] == '/')
        return 0;
    int dots = 0;
    for (const char *p = path; *p; p++) {
        unsigned char c = (unsigned char)*p;
        if (c < 0x20 || c == 0x7F)
            return 0;
        if (c == '/' || c == '\\' || c == ' ')
            return 0;
        if (c == '.') {
            dots++;
            if (p[1] == '.' || p[1] == 0)
                return 0; /* no ".." or trailing dot */
        }
        /* allow a-z A-Z 0-9 . _ - */
        int ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                 (c >= '0' && c <= '9') || c == '.' || c == '_' || c == '-';
        if (!ok)
            return 0;
    }
    return 1;
}

static int bind(const char *path, const char *key) {
    if (!ns_valid_path(path))
        return -1;
    int i = find_path(path);
    if (i < 0)
        i = free_slot();
    if (i < 0)
        return -1;
    strncpy(cells[i].path, path, NS_PATH_LEN - 1);
    cells[i].path[NS_PATH_LEN - 1] = 0;
    strncpy(cells[i].key, key, FS_NAME_LEN - 1);
    cells[i].key[FS_NAME_LEN - 1] = 0;
    cells[i].used = 1;
    return 0;
}

void ns_init(void) {
    for (int i = 0; i < NS_MAX_CELLS; i++) {
        cells[i].used = 0;
        cells[i].path[0] = 0;
        cells[i].key[0] = 0;
    }

    /* system map — SIF layout */
    bind("apps.hello", "hello.elf");
    bind("apps.echo", "echo.elf");
    bind("apps.cat", "cat.elf");
    bind("apps.ls", "ls.elf");
    bind("docs.motd", "motd");
    bind("docs.readme", "readme.txt");
    bind("docs.hello", "hello.txt");

    /* also allow direct elf names as short paths */
    bind("hello.elf", "hello.elf");
    bind("echo.elf", "echo.elf");
    bind("cat.elf", "cat.elf");
    bind("ls.elf", "ls.elf");

    printk("[ns] cell namespace online (paths use '.')\n");
}

const char *ns_store_key(const char *path) {
    int i = find_path(path);
    if (i >= 0)
        return cells[i].key;
    /* unmapped: path itself is the key if valid */
    if (ns_valid_path(path))
        return path;
    return 0;
}

int ns_exists(const char *path) {
    const char *k = ns_store_key(path);
    return k && fs_exists(k);
}

int ns_forge(const char *path) {
    if (!ns_valid_path(path))
        return -1;
    if (find_path(path) < 0 && bind(path, path) < 0)
        return -1;
    const char *k = ns_store_key(path);
    return fs_create(k, "", 0);
}

int ns_scribe(const char *path, const void *data, uint32_t n) {
    if (!ns_valid_path(path))
        return -1;
    if (find_path(path) < 0 && bind(path, path) < 0)
        return -1;
    const char *k = ns_store_key(path);
    return fs_write(k, data, n);
}

int ns_glance(const char *path, void *buf, uint32_t max, uint32_t *out) {
    const char *k = ns_store_key(path);
    if (!k)
        return -1;
    return fs_read(k, buf, max, out);
}

int ns_erase(const char *path) {
    int i = find_path(path);
    const char *k = ns_store_key(path);
    if (!k)
        return -1;
    int r = fs_delete(k);
    if (i >= 0) {
        cells[i].used = 0;
        cells[i].path[0] = 0;
    }
    return r;
}

int ns_cells(const char *prefix) {
    int n = 0;
    printk("CELL                                  KEY\n");
    for (int i = 0; i < NS_MAX_CELLS; i++) {
        if (!cells[i].used)
            continue;
        if (prefix && prefix[0]) {
            uint32_t pl = (uint32_t)strlen(prefix);
            if (strncmp(cells[i].path, prefix, pl) != 0)
                continue;
            /* if prefix is "apps", also accept "apps." */
            if (cells[i].path[pl] != 0 && cells[i].path[pl] != '.')
                continue;
        }
        printk("%-36s %s\n", cells[i].path, cells[i].key);
        n++;
    }
    if (!n)
        printk("(empty)\n");
    return n;
}
