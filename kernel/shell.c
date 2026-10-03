#include "shell.h"
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
#include <stddef.h>
#define LINE_MAX 160
static void cmd_help(void) {
    printk("\n=== SIF Shell v0.7 ===\n");
    printk(" Sistema:  help version clear reboot uptime date\n");
    printk(" Memoria:  mem ticks hexdump ARCHIVO\n");
    printk(" Tasks:    ps demo yield sleep MS\n");
    printk(" Files:    ls cat touch write rm\n");
    printk(" HW:       disk pci\n");
    printk(" Misc:     echo TEXTO panic\n\n");
}
static void worker_a(void) {
    for (int i=0;i<5;i++){printk("[A] %d\n",i);for(volatile int d=0;d<600000;d++){}task_yield();}
    printk("[A] fin\n"); task_exit();
}
static void worker_b(void) {
    for (int i=0;i<5;i++){printk("[B] %d\n",i);for(volatile int d=0;d<600000;d++){}task_yield();}
    printk("[B] fin\n"); task_exit();
}
static int starts(const char *s,const char *p){while(*p){if(*s++!=*p++)return 0;}return 1;}
static uint32_t parse_u(const char *s){uint32_t n=0;while(*s>='0'&&*s<='9'){n=n*10+(uint32_t)(*s-'0');s++;}return n;}
static void cmd_hexdump(const char *name){
    uint8_t buf[256]; uint32_t sz=0;
    if(fs_read(name,buf,sizeof(buf),&sz)<0){printk("no existe: %s\n",name);return;}
    for(uint32_t i=0;i<sz;i+=16){
        printk("%04x  ",i);
        for(uint32_t j=0;j<16;j++){if(i+j<sz)printk("%02x ",buf[i+j]);else printk("   ");}
        printk(" |");
        for(uint32_t j=0;j<16&&i+j<sz;j++){char c=(char)buf[i+j];printk("%c",(c>=32&&c<127)?c:'.');}
        printk("|\n");
    }
}
static void cmd_reboot(void){
    printk("rebooting...\n");
    uint8_t good=0x02; while(good&0x02) good=inb(0x64);
    outb(0x64,0xFE);
    __asm__ volatile("cli; lidt (%0); int $0"::"r"((uint32_t)0));
    for(;;) __asm__ volatile("hlt");
}
static void run_line(char *line){
    while(*line==' ')line++; if(!*line)return;
    if(starts(line,"help")){cmd_help();return;}
    if(starts(line,"ps")){task_list();return;}
    if(starts(line,"mem")){uint32_t u,f,b;kmalloc_stats(&u,&f,&b);
        printk("PMM free=%u used=%u total=%u\n",pmm_free_frames(),pmm_used_frames(),pmm_total_frames());
        printk("HEAP used=%u free=%u blocks=%u\n",u,f,b);return;}
    if(starts(line,"ticks")){printk("ticks=%u (~%u s)\n",timer_ticks(),timer_seconds());return;}
    if(starts(line,"uptime")){uint32_t s=timer_seconds();printk("up %u min %u sec\n",s/60,s%60);return;}
    if(starts(line,"date")||starts(line,"time")){rtc_print();return;}
    if(starts(line,"yield")){task_yield();return;}
    if(starts(line,"demo")){task_create("workerA",worker_a);task_create("workerB",worker_b);printk("workers OK\n");return;}
    if(starts(line,"clear")){vga_clear();return;}
    if(starts(line,"echo")){char *p=line+4;while(*p==' ')p++;printk("%s\n",p);return;}
    if(starts(line,"ls")){fs_list();return;}
    if(starts(line,"cat ")){char *n=line+4;while(*n==' ')n++;char buf[512];uint32_t sz=0;
        if(fs_read(n,buf,sizeof(buf)-1,&sz)<0){printk("no existe\n");return;}buf[sz]=0;printk("%s",buf);if(sz&&buf[sz-1]!='\n')printk("\n");return;}
    if(starts(line,"touch ")){char *n=line+6;while(*n==' ')n++;if(fs_create(n,"",0)<0)printk("error\n");else printk("ok\n");return;}
    if(starts(line,"write ")){char *p=line+6;while(*p==' ')p++;char name[32];int i=0;
        while(*p&&*p!=' '&&i<31)name[i++]=*p++;name[i]=0;while(*p==' ')p++;
        if(!name[0]){printk("uso: write ARCHIVO texto\n");return;}
        uint32_t len=0;while(p[len])len++;
        if(fs_write(name,p,len)<0)printk("error\n");else printk("ok (%u bytes)\n",len);return;}
    if(starts(line,"rm ")){char *n=line+3;while(*n==' ')n++;if(fs_delete(n)<0)printk("no existe\n");else printk("borrado\n");return;}
    if(starts(line,"disk")){if(!ata_present())printk("ATA: sin disco\n");else{printk("ATA: presente\n");uint8_t sec[512];
        if(ata_read_sectors(0,1,sec)==0)printk("LBA0 OK\n");else printk("error\n");}return;}
    if(starts(line,"pci")){pci_list();return;}
    if(starts(line,"sleep ")){uint32_t ms=parse_u(line+6);if(!ms)ms=1000;printk("sleep %u ms...\n",ms);sys_sleep(ms);printk("wake\n");return;}
    if(starts(line,"hexdump ")){char *n=line+8;while(*n==' ')n++;cmd_hexdump(n);return;}
    if(starts(line,"reboot")){cmd_reboot();return;}
    if(starts(line,"version")){printk("%s %s\n",SIF_KERNEL_NAME,SIF_KERNEL_VERSION);
        printk("TSS ring3 | PCI | syscalls | ramfs | ATA | RTC\n");return;}
    if(starts(line,"panic")){printk("#DE\n");__asm__ volatile("int $0");return;}
    printk("desconocido: '%s'\n",line);
}
void shell_run(void){
    char buf[LINE_MAX]; int pos=0; printk("SIF> ");
    for(;;){
        if(task_needs_resched()){task_clear_resched();task_yield();}
        if(!keyboard_has_input()){__asm__ volatile("hlt");continue;}
        int ch=keyboard_read_char(); if(!ch)continue;
        if(ch=='\n'||ch=='\r'){buf[pos]=0;printk("\n");run_line(buf);pos=0;printk("SIF> ");continue;}
        if(ch=='\b'){if(pos>0){pos--;vga_putc('\b');}continue;}
        if(pos<LINE_MAX-1&&ch>=32&&ch<127){buf[pos++]=(char)ch;vga_putc((char)ch);}
    }
}
void shell_task(void){shell_run();}
