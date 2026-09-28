#include "shell.h"
#include "keyboard.h"
#include "printk.h"
#include "task.h"
#include "pmm.h"
#include "timer.h"
#include "vga.h"
#include "fs.h"
#include "ata.h"
#include <stddef.h>
#define LINE_MAX 128
static void cmd_help(void) {
    printk("\nComandos:\n  help ps mem ticks yield demo clear\n");
    printk("  echo TEXTO  ls  cat ARCHIVO  disk  version\n\n");
}
static void worker_a(void) {
    for (int i=0;i<5;i++){printk("[A] %d\n",i);for(volatile int d=0;d<800000;d++){}task_yield();}
    printk("[A] fin\n"); task_exit();
}
static void worker_b(void) {
    for (int i=0;i<5;i++){printk("[B] %d\n",i);for(volatile int d=0;d<800000;d++){}task_yield();}
    printk("[B] fin\n"); task_exit();
}
static int starts(const char *s,const char *p){while(*p){if(*s++!=*p++)return 0;}return 1;}
static void run_line(char *line) {
    while (*line==' ') line++;
    if (!*line) return;
    if (starts(line,"help")){cmd_help();return;}
    if (starts(line,"ps")){task_list();return;}
    if (starts(line,"mem")){printk("free=%u used=%u total=%u\n",pmm_free_frames(),pmm_used_frames(),pmm_total_frames());return;}
    if (starts(line,"ticks")){printk("ticks=%u\n",timer_ticks());return;}
    if (starts(line,"yield")){task_yield();return;}
    if (starts(line,"demo")){task_create("workerA",worker_a);task_create("workerB",worker_b);printk("workers OK\n");return;}
    if (starts(line,"clear")){vga_clear();return;}
    if (starts(line,"echo")){char *p=line+4;while(*p==' ')p++;printk("%s\n",p);return;}
    if (starts(line,"ls")){fs_list();return;}
    if (starts(line,"cat ")){
        char *n=line+4;while(*n==' ')n++;
        char buf[512]; uint32_t sz=0;
        if (fs_read(n,buf,sizeof(buf)-1,&sz)<0){printk("no existe: %s\n",n);return;}
        buf[sz]=0; printk("%s",buf); if(sz&&buf[sz-1]!='\n')printk("\n"); return;
    }
    if (starts(line,"disk")){
        if(!ata_present())printk("ATA: sin disco\n");
        else{uint8_t sec[512];if(ata_read_sectors(0,1,sec)==0)printk("LBA0 OK\n");else printk("error\n");}
        return;
    }
    if (starts(line,"version")){printk("SIF Kernel v0.5\n");return;}
    printk("desconocido: %s\n",line);
}
void shell_run(void) {
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
