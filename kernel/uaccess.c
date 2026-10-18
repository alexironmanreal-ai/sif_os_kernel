#include "uaccess.h"
#include "paging.h"
#include "task.h"
#include "string.h"

/*
 * Security helpers for ring-3 boundaries.
 * Kernel tasks (shell) may pass kernel pointers; user tasks may not.
 */

static int is_user_task(void) {
    struct task *t = task_current();
    return t && t->is_user;
}

int access_ok(const void *ptr, uint32_t len) {
    if (!ptr && len)
        return 0;
    if (!len)
        return 1;

    uint32_t start = (uint32_t)ptr;
    uint32_t end = start + len;
    if (end < start)
        return 0; /* overflow */

    if (!is_user_task())
        return 1; /* kernel context trusted */

    /* user may only touch [USER_BASE, USER_TOP) */
    if (start < USER_BASE || end > USER_TOP)
        return 0;
    return 1;
}

int copy_from_user(void *dst, const void *user_src, uint32_t n) {
    if (!n)
        return 0;
    if (!dst || !user_src)
        return -1;
    if (!access_ok(user_src, n))
        return -1;
    memcpy(dst, user_src, n);
    return 0;
}

int copy_to_user(void *user_dst, const void *src, uint32_t n) {
    if (!n)
        return 0;
    if (!user_dst || !src)
        return -1;
    if (!access_ok(user_dst, n))
        return -1;
    memcpy(user_dst, src, n);
    return 0;
}

int user_strnlen(const char *user_s, uint32_t max) {
    if (!user_s || !max)
        return -1;
    if (!access_ok(user_s, 1))
        return -1;
    uint32_t i = 0;
    while (i < max) {
        if (!access_ok(user_s + i, 1))
            return -1;
        if (user_s[i] == 0)
            return (int)i;
        i++;
    }
    return -1; /* not terminated within max */
}

int path_ok(const char *path) {
    if (!path || !path[0])
        return 0;
    /* no absolute paths, no .., no control chars */
    if (path[0] == '/' || path[0] == '\\')
        return 0;
    for (const char *p = path; *p; p++) {
        unsigned char c = (unsigned char)*p;
        if (c < 0x20 || c == 0x7F)
            return 0;
        if (c == '/' || c == '\\')
            return 0;
    }
    /* reject ".." and "." as whole name */
    if (path[0] == '.' && (path[1] == 0 || (path[1] == '.' && path[2] == 0)))
        return 0;
    if (strlen(path) >= PROC_PATH_LEN)
        return 0;
    return 1;
}

int copy_path_from_user(char *kbuf, uint32_t kbuf_len, const char *user_path) {
    if (!kbuf || kbuf_len < 2)
        return -1;

    if (!is_user_task()) {
        /* kernel path */
        if (!user_path || !path_ok(user_path))
            return -1;
        strncpy(kbuf, user_path, kbuf_len - 1);
        kbuf[kbuf_len - 1] = 0;
        return path_ok(kbuf) ? 0 : -1;
    }

    int n = user_strnlen(user_path, kbuf_len - 1);
    if (n < 0 || n == 0)
        return -1;
    if (copy_from_user(kbuf, user_path, (uint32_t)n) < 0)
        return -1;
    kbuf[n] = 0;
    return path_ok(kbuf) ? 0 : -1;
}
