#ifndef SIF_KLOG_H
#define SIF_KLOG_H

#include <stdint.h>

void klog_init(void);
void klog_write(const char *s, uint32_t n);
void klog_putc(char c);
/* dump last messages to printk sinks; max lines 0 = all */
void klog_dump(uint32_t max_lines);
uint32_t klog_bytes(void);

#endif
