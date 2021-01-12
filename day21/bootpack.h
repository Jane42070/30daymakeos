/* asmhead.nas */
#include <stdio.h>
#include <string.h>
/** 储存启动信息
 * cyls 柱面
 * leds 指示灯状态
 * vmode 显示模式
 * reserve 反转
 * */
struct BOOTINFO { // 0x0ff0 ~ 0x0fff
	char cyls, leds, vmode, reserve;
	short scrnx, scrny;
	unsigned char *vram;
};
// 定义启动信息存放的内存地址
#define ADR_BOOTINFO 0x00000ff0
// 数据存放在 0x00100000 ~ 0x00267fff
#define ADR_DISKIMG  0x00100000

/* naskfunc.nas */
void io_hlt(void);
void io_cli(void);
void io_sti(void);
void io_stihlt(void);
int io_in8(int port);
void io_out8(int port, int data);
int io_load_eflags(void);
void io_store_eflags(int eflags);
int load_cr0(void);
void load_tr(int tr);
void farjmp(int eip, int cs);
void farcall(int eip, int cs);
void store_cr0(int cr0);
void asm_inthandler20(void);
void asm_inthandler21(void);
void asm_inthandler27(void);
void asm_inthandler2c(void);
void asm_hrb_api();
unsigned int memtest_sub(unsigned int start, unsigned int end);
void start_app(int eip, int cs, int esp, int ds);

/** 装载段号寄存器函数
 *  装载中断记录表函数
 *  C语言无法实现*/
void load_gdtr(int limit, int addr);
void load_idtr(int limit, int addr);

/* graphic.c */
void init_palette(void);
void set_palette(int start, int end, unsigned char *rgb);
void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1);
void init_screen(unsigned char *vram, int x, int y);
void putfont8(unsigned char *vram, int xsize, int x, int y, char c, char *font);
void putfonts8_str(unsigned char *vram, int xsize, int x, int y, char c, char *s);
void init_mouse_cursor8(unsigned char *mouse, char bc);
void putblock8_8(unsigned char *vram, int vxsize, int pxsize, int pysize, int px0, int py0, unsigned char *buf, int bxsize);

#define COL8_000000		0  /*  0: 黑 */
#define COL8_FF0000		1  /*  1: 亮红 */
#define COL8_00FF00		2  /*  2: 亮绿 */
#define COL8_FFFF00		3  /*  3: 亮黄 */
#define COL8_0000FF		4  /*  4: 亮蓝 */
#define COL8_FF00FF		5  /*  5: 亮紫 */
#define COL8_00FFFF		6  /*  6: 浅亮蓝 */
#define COL8_FFFFFF		7  /*  7: 白 */
#define COL8_C6C6C6		8  /*  8: 亮灰 */
#define COL8_840000		9  /*  9: 暗红 */
#define COL8_008400		10 /* 10: 暗绿 */
#define COL8_848400		11 /* 11: 暗黄 */
#define COL8_000084		12 /* 12: 暗青 */
#define COL8_840084		13 /* 13: 暗紫 */
#define COL8_008484		14 /* 14: 浅暗蓝 */
#define COL8_848484		15 /* 15: 暗灰 */

/* dsctbl.c */
/** 段号记录 */
struct SEGMENT_DESCRIPTOR {
	short limit_low, base_low;
	char base_mid, access_right;
	char limit_high, base_high;
};
/** 中断记录表 */
struct GATE_DESCRIPTOR {
	short offset_low, selector;
	char dw_count, access_right;
	short offset_high;
};

/** 段号信息设置 */
void set_segmdesc(struct SEGMENT_DESCRIPTOR *sd, unsigned int limit, int base, int ar);
/** 全局中断记录表设置 */
void set_gatedesc(struct GATE_DESCRIPTOR *gd, int offset, int selector, int ar);
/** GDT 和 IDT 初始化函数 */
void init_gdtidt(void);

#define ADR_IDT			0x0026f800
#define LIMIT_IDT		0x000007ff
#define ADR_GDT			0x00270000
#define LIMIT_GDT		0x0000ffff
#define ADR_BOTPAK		0x00280000
#define LIMIT_BOTPAK	0x0007ffff
#define AR_DATA32_RW	0x4092
#define AR_TSS32		0x0089
#define AR_CODE32_ER	0x409a
// 中断处理属性
#define AR_INTGATE32	0x008e

/* fifo.c */
#define FLAGS_OVERRUN	0x0001
// FIFO 优化缓冲区
struct FIFO32 {
	int *buf;
	int w, r, size, free, flags;
	struct TASK *task;
};
void fifo32_init(struct FIFO32 *fifo, int size, int *buf, struct TASK *task);
int fifo32_put(struct FIFO32 *fifo, int data);
int fifo32_get(struct FIFO32 *fifo);
int fifo32_status(struct FIFO32 *fifo);

/* init.c */
#define PORT_KEYDAT		0x0060
/** 初始化 PIC */
void init_pic(void);
void inthandler27(int *esp);
// PIC 的 IO 定义
#define PIC0_ICW1		0x0020
#define PIC0_OCW2		0x0020
#define PIC0_IMR		0x0021
#define PIC0_ICW2		0x0021
#define PIC0_ICW3		0x0021
#define PIC0_ICW4		0x0021
#define PIC1_ICW1		0x00a0
#define PIC1_OCW2		0x00a0
#define PIC1_IMR		0x00a1
#define PIC1_ICW2		0x00a1
#define PIC1_ICW3		0x00a1
#define PIC1_ICW4		0x00a1

/* keyboard.c */
#define PORT_KEYCMD				0x0064
#define PORT_KEYSTA				0x0064
#define KEYSTA_SEND_NOTREADY	0x02
#define KEYCMD_WRITE_MODE		0x60
#define KBC_MODE				0x47
void inthandler21(int *esp);
void wait_KBC_sendready(void);
void init_keyboard(struct FIFO32 *fifo, int data0);

/* mouse.c */
#define KEYCMD_SENDTO_MOUSE		0xd4
#define MOUSECMD_ENABLE			0xf4
// 鼠标实时数据结构体
struct MOUSE_DEC {
	unsigned char buf[3], phase;
	// 鼠标移动、按键状态
	int x, y, btn;
};
void inthandler2c(int *esp);
void enable_mouse(struct FIFO32 *fifo, int data0, struct MOUSE_DEC *mdec);
int mouse_decode(struct MOUSE_DEC *mdec, unsigned char data);
extern struct MOUSE_DEC mdec;

/* memory.c */
#define MEMMAN_FREES		4090
#define MEMMAN_ADDR			0x003c0000
#define EFLAGS_AC_BIT		0x00040000
#define CR0_CACHE_DISABLE	0x60000000
// 内存可用信息
struct FREEINFO {
	unsigned int addr, size;
};
// 内存管理
struct MEMMAN {
	int frees, maxfrees, lostsize, losts;
	struct FREEINFO free[MEMMAN_FREES];
};
unsigned int memtest(unsigned int start, unsigned int end);
void memman_init(struct MEMMAN *man);
unsigned int memman_total(struct MEMMAN *man);
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size);
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size);
unsigned int memman_alloc_4k(struct MEMMAN *man, unsigned int size);
unsigned int memman_free_4k(struct MEMMAN *man, unsigned int addr, unsigned int size);

/* sheet.c */
// 最大图层数
#define MAX_SHEETS	256
// 图层使用
#define SHEET_USE	1
// 透明图层
// 图层大小 = bxsize * bysize
// 坐标 vx0 & vy0
// 透明色色号 col_inv
// 图层高度 height
// 图层设定信息 flags 默认未使用 0
struct SHEET {
	unsigned char *buf;
	int bxsize, bysize, vx0, vy0, col_inv, height, flags;
	struct SHTCTL *ctl;
};
// 图层管理
// 显存地址 vram
// 显存的内存映射 vmap
// 屏幕画面大小 xsize, ysize
// 最上面图层的高度 top
// 记忆地址变量（根据图层高度顺序写入） sheets
// 存放256个图层的信息 sheets0
struct SHTCTL {
	unsigned char *vram, *vmap;
	int xsize, ysize, top;
	struct SHEET *sheets[MAX_SHEETS];
	struct SHEET sheets0[MAX_SHEETS];
};
struct SHTCTL *shtctl_init(struct MEMMAN *memman, unsigned char *vram, int xsize, int ysize);
struct SHEET *sheet_alloc(struct SHTCTL *ctl);
void sheet_setbuf(struct SHEET *sht, unsigned char *buf, int xsize, int ysize, int col_inv);
void sheet_refreshsub(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1, int h0, int h1);
void sheet_refreshmap(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1, int h0);
void sheet_refresh(struct SHEET *sht, int bx0, int by0, int bx1, int by1);
void sheet_updown(struct SHEET *sht, int height);
void sheet_slide(struct SHEET *sht, int vx0, int vy0);
void sheet_free(struct SHEET *sht);
void putfonts8_str_sht(struct SHEET *sht, int x, int y, int c, char b, char *s);

/* timer.c */
#define PIT_CTRL	0x0043
#define PIT_CNT0	0x0040
#define MAX_TIMER	500
#define TIMER_FLAGS_ALLOC	1	// 是否已配置
#define TIMER_FLAGS_ACTING	2	// 定时器运行中
// 计时器单向链表
// 下一个定时器地址 next
// 计时时间 timeout
// 缓冲区 fifo
// 剩余时间没有后需要向缓冲区写入的数据 data
struct TIMER {
	struct TIMER *next;
	unsigned int timeout, flags;
	struct FIFO32 *fifo;
	int data;
};
// 计时器管理
// 计时 count
// 最近的计时器时间 next
// 正在活动 acting == SHEETCTL->top
// 排好序的计时器结构体
// 计时器结构体
struct TIMERCTL {
	unsigned int count, next;
	struct TIMER *t0;
	struct TIMER timers0[MAX_TIMER];
};
extern struct TIMERCTL timerctl;
void init_pit();
struct TIMER *timer_alloc();
void timer_free(struct TIMER *timer);
void timer_init(struct TIMER *timer, struct FIFO32 *fifo, int data);
void timer_settime(struct TIMER *timer, unsigned int timeout);
void inthandler20(int *esp);

/* mtask.c */
#define MAX_TASKS		1000	// 最大任务数量
#define TASK_GDT0		3		// 定义从 GDT 的几号开始分配给 TSS
#define MAX_TASKS_LV	100		// 单个优先级任务数量
#define MAX_TASKLEVELS	10		// 优先级数量
/*
 * backlink ~ cr3 任务相关设置信息,任务切换时不会被写入
 * eip ~ edi 32位寄存器
 * es ~ gs 16位寄存器
 * */
struct TSS32 {
	int backlink, esp0, ss0, esp1, ss1, esp2, ss2, cr3;
	int eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;
	int es, cs, ss, ds, fs, gs;
	int ldtr, iomap;
};
/*
 * sel(selector) 用来存放 GDT 的编号
 * level 优先级级别
 * priority 任务优先级
 * fifo 缓冲区
 * tss 任务属性
 * */
struct TASK {
	int sel, flags;
	int level, priority;
	struct FIFO32 fifo;
	struct TSS32 tss;
};

struct TASKLEVEL {
	int running;	// 正在运行的任务数量
	int now;		// 当前运行的任务序号
	struct TASK *tasks[MAX_TASKS_LV];
};

struct TASKCTL {
	int now_lv;								// 现在活动中的 LEVEL
	char lv_change;							// 下次任务切换时是否需要改变 LEVEL
	struct TASK tasks0[MAX_TASKS];			// 所有的任务
	struct TASKLEVEL level[MAX_TASKLEVELS];	// 某优先级内的任务;
};
struct TASK *task_init(struct MEMMAN *memman);	// 初始化所有预分配任务
struct TASK *task_alloc();						// 分配任务
extern struct TIMER *task_timer;				// 任务切换计时器
void task_run(struct TASK *task, int level, int priority);	// 运行任务
void task_switch();								// 任务切换
void task_sleep(struct TASK *task);				// 任务休眠
struct TASK *task_now();						// 返回在活动的 struct TASK 的内存地址
void task_add(struct TASK *task);				// 在相应优先级任务管理中添加任务
void task_remove(struct TASK *task);			// 在 struct TASKLEVEL 中删除任务
void task_switchsub();							// 根据等级切换任务
void task_idle();								// 闲置任务

/* window.c */
void make_window8(unsigned char *buf, int xsize, int ysize, char *title, char act);
void make_wtitle8(unsigned char *buf, int xsize, char *title, char act);
void make_textbox8(struct SHEET *sht, int x0, int y0, int sx, int sy, int c);

/* terminal.c */
// 终端
struct TERM {
	struct SHEET *sht;
	int cur_x, cur_y, cur_c;
};
void term_task(struct SHEET *sheet, unsigned int memtotal);
void term_newline(struct TERM *term);
void term_putchar(struct TERM *term, int c, char mv);
void term_putstr(struct TERM *term, char *s);
void term_putnstr(struct TERM *term, char *s, int l);
void term_runcmd(char *cmdline, struct TERM *term, int *fat, unsigned int memtotal);
void cmd_mem(struct TERM *term, unsigned int memtotal);
void cmd_clear(struct TERM *term);
void cmd_ls(struct TERM *term);
void cmd_cat(struct TERM *term, int *fat, char *cmdline);
void cmd_uname(struct TERM *term, char *cmdline);
int cmd_app(struct TERM *term, int *fat, char *cmdline);
void hrb_api(int edi, int esi, int ebp, int esp, int ebx, int edx, int ecx, int eax);

/* file.c */
// 文件信息
struct FILEINFO {
	unsigned char name[8], ext[3], type;
	char reserve[10];
	unsigned short time, date, clustno;
	unsigned int size;
};
void file_readfat(int *fat, unsigned char *img);
void file_loadfile(int clustno, int size, char *buf, int *fat, char *img);
struct FILEINFO *file_search(char *name, struct FILEINFO *finfo, int max);
