#include "shell.h"
#include "cmd.h"
#include "keyboard.h"
#include "printk.h"
#include "task.h"
#include "pmm.h"
#include "timer.h"
#include "vga.h"
#include "fs.h"
#include "ata.h"
#include "rtc.h"
#include "kmalloc.h"
#include "kernel.h"
#include "pci.h"
#include "syscall.h"
#include "io.h"
#include "userland.h"
#include <stddef.h>
#define LINE_MAX 160
#define HIST_MAX 16
static char hist[HIST_MAX][LINE_MAX];
static int hist_count;
static void hist_add(const char *line) {
    if (!line[0]) return;
    int i = 0;
    while (line[i] && i < LINE_MAX - 1) { hist[hist_count % HIST_MAX][i] = line[i]; i++; }
    hist[hist_count % HIST_MAX][i] = 0; hist_count++;
}
static void wa(void){for(int i=0;i<5;i++){printk("[A] %d\n",i);for(volatile int d=0;d<600000;d++){}task_yield();}printk("[A] fin\n");task_exit();}
static void wb(void){for(int i=0;i<5;i++){printk("[B] %d\n",i);for(volatile int d=0;d<600000;d++){}task_yield();}printk("[B] fin\n");task_exit();}
static int c_help(int a,char **v){(void)a;(void)v;printk("\n=== SIF Shell ===\n");cmd_list();printk("\n");return 0;}
static int c_version(int a,char **v){(void)a;(void)v;printk("%s %s\n",SIF_KERNEL_NAME,SIF_KERNEL_VERSION);return 0;}
static int c_clear(int a,char **v){(void)a;(void)v;vga_clear();return 0;}
static int c_echo(int a,char **v){for(int i=1;i<a;i++){if(i>1)printk(" ");printk("%s",v[i]);}printk("\n");return 0;}
static int c_ps(int a,char **v){(void)a;(void)v;task_list();return 0;}
static int c_mem(int a,char **v){(void)a;(void)v;uint32_t u,f,b;kmalloc_stats(&u,&f,&b);printk("PMM free=%u used=%u total=%u\n",pmm_free_frames(),pmm_used_frames(),pmm_total_frames());printk("HEAP used=%u free=%u blocks=%u\n",u,f,b);return 0;}
static int c_free(int a,char **v){(void)a;(void)v;return c_mem(0,0);}
static int c_ticks(int a,char **v){(void)a;(void)v;printk("ticks=%u\n",timer_ticks());return 0;}
static int c_uptime(int a,char **v){(void)a;(void)v;uint32_t s=timer_seconds();printk("up %u min %u sec\n",s/60,s%60);return 0;}
static int c_date(int a,char **v){(void)a;(void)v;rtc_print();return 0;}
static int c_yield(int a,char **v){(void)a;(void)v;task_yield();return 0;}
static int c_demo(int a,char **v){(void)a;(void)v;task_create("workerA",wa);task_create("workerB",wb);printk("workers OK\n");return 0;}
static int c_ls(int a,char **v){(void)a;(void)v;fs_list();return 0;}
static int c_cat(int a,char **v){if(a<2){printk("uso: cat ARCHIVO\n");return -1;}char b[512];uint32_t sz=0;if(fs_read(v[1],b,sizeof(b)-1,&sz)<0){printk("no existe\n");return -1;}b[sz]=0;printk("%s",b);if(sz&&b[sz-1]!='\n')printk("\n");return 0;}
static int c_touch(int a,char **v){if(a<2){printk("uso: touch A\n");return -1;}if(fs_create(v[1],"",0)<0){printk("error\n");return -1;}printk("ok\n");return 0;}
static int c_write(int a,char **v){if(a<3){printk("uso: write A texto\n");return -1;}char body[256];int pos=0;for(int i=2;i<a&&pos<255;i++){if(i>2)body[pos++]=' ';for(char *p=v[i];*p&&pos<255;p++)body[pos++]=*p;}body[pos]=0;if(fs_write(v[1],body,(uint32_t)pos)<0){printk("error\n");return -1;}printk("ok\n");return 0;}
static int c_rm(int a,char **v){if(a<2){printk("uso: rm A\n");return -1;}if(fs_delete(v[1])<0){printk("no existe\n");return -1;}printk("borrado\n");return 0;}
static int c_disk(int a,char **v){(void)a;(void)v;if(!ata_present())printk("ATA: no\n");else{uint8_t s[512];printk(ata_read_sectors(0,1,s)==0?"ATA OK\n":"ATA err\n");}return 0;}
static int c_pci(int a,char **v){(void)a;(void)v;pci_list();return 0;}
static int c_sleep(int a,char **v){uint32_t ms=1000;if(a>=2){ms=0;for(char *p=v[1];*p>='0'&&*p<='9';p++)ms=ms*10+(uint32_t)(*p-'0');if(!ms)ms=1000;}printk("sleep %u\n",ms);sys_sleep(ms);printk("wake\n");return 0;}
static int c_hexdump(int a,char **v){if(a<2){printk("uso: hexdump A\n");return -1;}uint8_t buf[256];uint32_t sz=0;if(fs_read(v[1],buf,sizeof(buf),&sz)<0){printk("no existe\n");return -1;}for(uint32_t i=0;i<sz;i+=16){printk("%04x ",i);for(uint32_t j=0;j<16;j++)if(i+j<sz)printk("%02x ",buf[i+j]);else printk("   ");printk("\n");}return 0;}
static int c_reboot(int a,char **v){(void)a;(void)v;printk("reboot\n");uint8_t g=0x02;while(g&0x02)g=inb(0x64);outb(0x64,0xFE);for(;;)__asm__ volatile("hlt");return 0;}
static int c_panic(int a,char **v){(void)a;(void)v;__asm__ volatile("int $0");return 0;}
static int c_uname(int a,char **v){(void)a;(void)v;printk("%s %s i386\n",SIF_KERNEL_NAME,SIF_KERNEL_VERSION);return 0;}
static int c_whoami(int a,char **v){(void)a;(void)v;printk("root\n");return 0;}
static int c_info(int a,char **v){(void)a;(void)v;printk("kernel %s %s\nuptime %u\npci %d\nata %s\n",SIF_KERNEL_NAME,SIF_KERNEL_VERSION,timer_seconds(),pci_count(),ata_present()?"yes":"no");return 0;}
static int c_history(int a,char **v){(void)a;(void)v;int s=hist_count>HIST_MAX?hist_count-HIST_MAX:0;for(int i=s;i<hist_count;i++)printk("%3d %s\n",i,hist[i%HIST_MAX]);return 0;}
static int c_true(int a,char **v){(void)a;(void)v;return 0;}
static int c_false(int a,char **v){(void)a;(void)v;return 1;}
static int c_user(int a,char **v){(void)a;(void)v;userland_run_test();return 0;}
static void reg(void){
    cmd_init();
    cmd_register("help","lista",c_help);
    cmd_register("version","version",c_version);
    cmd_register("uname","sistema",c_uname);
    cmd_register("whoami","usuario",c_whoami);
    cmd_register("info","resumen",c_info);
    cmd_register("clear","cls",c_clear);
    cmd_register("echo","print",c_echo);
    cmd_register("ps","procesos",c_ps);
    cmd_register("mem","memoria",c_mem);
    cmd_register("free","memoria",c_free);
    cmd_register("ticks","timer",c_ticks);
    cmd_register("uptime","uptime",c_uptime);
    cmd_register("date","RTC",c_date);
    cmd_register("time","RTC",c_date);
    cmd_register("yield","yield",c_yield);
    cmd_register("demo","workers",c_demo);
    cmd_register("ls","listar",c_ls);
    cmd_register("cat","leer",c_cat);
    cmd_register("touch","crear",c_touch);
    cmd_register("write","escribir",c_write);
    cmd_register("rm","borrar",c_rm);
    cmd_register("disk","ATA",c_disk);
    cmd_register("pci","PCI",c_pci);
    cmd_register("sleep","ms",c_sleep);
    cmd_register("hexdump","hex",c_hexdump);
    cmd_register("history","hist",c_history);
    cmd_register("reboot","reset",c_reboot);
    cmd_register("panic","crash",c_panic);
    cmd_register("true","0",c_true);
    cmd_register("false","1",c_false);
    cmd_register("user","entrar ring3",c_user);
}
void shell_run(void){
    reg();
    char buf[LINE_MAX]; int pos=0;
    printk("SIF shell v0.9 - help | user\nSIF> ");
    for(;;){
        if(task_needs_resched()){task_clear_resched();task_yield();}
        if(!keyboard_has_input()){__asm__ volatile("hlt");continue;}
        int ch=keyboard_read_char(); if(!ch)continue;
        if(ch=='\n'||ch=='\r'){
            buf[pos]=0; printk("\n");
            if(pos>0){hist_add(buf); char w[LINE_MAX]; int i=0; while(buf[i]&&i<LINE_MAX-1){w[i]=buf[i];i++;} w[i]=0; cmd_run_line(w);}
            pos=0; printk("SIF> "); continue;
        }
        if(ch=='\b'){if(pos>0){pos--;vga_putc('\b');}continue;}
        if(pos<LINE_MAX-1&&ch>=32&&ch<127){buf[pos++]=(char)ch;vga_putc((char)ch);}
    }
}
void shell_task(void){shell_run();}
