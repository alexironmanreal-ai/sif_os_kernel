#ifndef SIF_UACCESS_H
#define SIF_UACCESS_H

#include <stdint.h>

/* 1 if [ptr, ptr+len) is a valid user range (or kernel task) */
int access_ok(const void *ptr, uint32_t len);

/* copy between kernel and user; 0 ok, -1 fault */
int copy_from_user(void *dst, const void *user_src, uint32_t n);
int copy_to_user(void *user_dst, const void *src, uint32_t n);

/* length of user C string up to max (not including NUL), or -1 */
int user_strnlen(const char *user_s, uint32_t max);

/*
 * Copy user path into kbuf (size kbuf_len). Rejects empty, too long,
 * ".." components, absolute weirdness. 0 ok, -1 bad.
 */
int copy_path_from_user(char *kbuf, uint32_t kbuf_len, const char *user_path);

/* validate already-in-kernel path string */
int path_ok(const char *path);

#endif
