#include "vfs.h"
#include "fs.h"
#include "string.h"
#include "printk.h"
#include "keyboard.h"
#include "vga.h"
#include "serial.h"
#define FD_SPECIAL 1
#define FD_FILE 2
static struct {
    int used, kind, special_id, writable;
    char name[VFS_PATH_LEN];
    uint32_t offset;
} fds[VFS_MAX_FD];
void vfs_init(void) {
    for (int i = 0; i < VFS_MAX_FD; i++) fds[i].used = 0;
    fds[0].used=1; fds[0].kind=FD_SPECIAL; fds[0].special_id=0; fds[0].writable=0;
    fds[1].used=1; fds[1].kind=FD_SPECIAL; fds[1].special_id=1; fds[1].writable=1;
    fds[2].used=1; fds[2].kind=FD_SPECIAL; fds[2].special_id=2; fds[2].writable=1;
    printk("[vfs] fd 0/1/2 = stdin/stdout/stderr\n");
}
int vfs_open(const char *path, int write) {
    if (!path || !path[0]) return -1;
    if (!write && !fs_exists(path)) return -1;
    if (write && !fs_exists(path) && fs_create(path,"",0)<0) return -1;
    int slot=-1;
    for (int i=3;i<VFS_MAX_FD;i++) if (!fds[i].used){slot=i;break;}
    if (slot<0) return -1;
    strncpy(fds[slot].name, path, VFS_PATH_LEN-1);
    fds[slot].name[VFS_PATH_LEN-1]=0;
    fds[slot].offset=0; fds[slot].writable=write?1:0;
    fds[slot].kind=FD_FILE; fds[slot].used=1;
    return slot;
}
int vfs_close(int fd) {
    if (fd<0||fd>=VFS_MAX_FD||!fds[fd].used) return -1;
    if (fd<=2) return 0;
    fds[fd].used=0; return 0;
}
int vfs_read(int fd, void *buf, uint32_t n) {
    if (fd<0||fd>=VFS_MAX_FD||!fds[fd].used||!n) return -1;
    if (fds[fd].kind==FD_SPECIAL && fds[fd].special_id==0) {
        uint8_t *out=(uint8_t*)buf; uint32_t got=0;
        while (got<n) {
            while (!keyboard_has_input()) __asm__ volatile("hlt");
            int ch=keyboard_read_char(); if(!ch) continue;
            out[got++]=(uint8_t)ch;
            if (ch=='\n'||ch=='\r') break;
        }
        return (int)got;
    }
    if (fds[fd].kind!=FD_FILE) return -1;
    uint8_t tmp[512]; uint32_t sz=0;
    if (fs_read(fds[fd].name,tmp,sizeof(tmp),&sz)<0) return -1;
    if (fds[fd].offset>=sz) return 0;
    uint32_t avail=sz-fds[fd].offset; if(n>avail)n=avail;
    memcpy(buf,tmp+fds[fd].offset,n); fds[fd].offset+=n;
    return (int)n;
}
int vfs_write(int fd, const void *buf, uint32_t n) {
    if (fd<0||fd>=VFS_MAX_FD||!fds[fd].used) return -1;
    if (fds[fd].kind==FD_SPECIAL) {
        if (fds[fd].special_id==1||fds[fd].special_id==2) {
            const char *p=(const char*)buf;
            for (uint32_t i=0;i<n;i++){ vga_putc(p[i]); serial_putc(p[i]); }
            return (int)n;
        }
        return -1;
    }
    if (!fds[fd].writable) return -1;
    uint8_t old[4096]; uint32_t osz=0;
    fs_read(fds[fd].name,old,sizeof(old),&osz);
    if (osz+n>sizeof(old)) n=sizeof(old)-osz;
    memcpy(old+osz,buf,n);
    if (fs_write(fds[fd].name,old,osz+n)<0) return -1;
    fds[fd].offset=osz+n; return (int)n;
}
int vfs_is_open(int fd){ return fd>=0&&fd<VFS_MAX_FD&&fds[fd].used; }
