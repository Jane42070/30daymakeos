/* asmhead.nas */

#include <stdio.h>
#include <string.h>

/** 储存启动信息
 * cyls 柱面
 * leds 指示灯状态
 * vmode 显示模式
 * reserve 反转
 * */
struct BOOTINFO {
	char cyls, leds, vmode, reserve;
	short scrnx, scrny;
	char *vram;
};

// 定义启动信息存放的内存地址
#define ADR_BOOTINFO 0x00000ff0

/* naskfunc.nas */

void io_hlt(void);
void io_cli(void);
void io_sti(void);
void io_stihlt(void);
int io_in8(int port);
void io_out8(int port, int data);
int io_load_eflags(void);
void io_store_eflags(int eflags);
void asm_inthandler21(void);
void asm_inthandler27(void);
void asm_inthandler2c(void);

/** 装载段号寄存器函数
 *  装载中断记录表函数
 *  C语言无法实现*/
void load_gdtr(int limit, int addr);
void load_idtr(int limit, int addr);

/* graphic.c */

void io_hlt(void);
void io_cli(void);
void io_out8(int port, int data);
int io_load_eflags(void);
void io_store_eflags(int eflags);

void init_palette(void);
void set_palette(int start, int end, unsigned char *rgb);
void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1);
void init_screen(unsigned char *vram, int x, int y);
void putfont8(char *vram, int xsize, int x, int y, char c, char *font);
void putfont8_str(char *vram, int xsize, int x, int y, char c, unsigned char *s);
void putfont8_pos(char *vram, int xsize, int pos,int y, char c, unsigned char *s);
void init_mouse_cursor8(char *mouse, char bc);
void putblock8_8(char *vram, int vxsize, int pxsize, int pysize, int px0, int py0, char *buf, int bxsize);

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
#define AR_CODE32_ER	0x409a
// 中断处理属性
#define AR_INTGATE32	0x008e

/* init.c */
#define PORT_KEYDAT 0x0060

struct KEYBUF {
	unsigned char data[32];
	int next;
};

/** 初始化 PIC */
void init_pic(void);
/* 接收来自PS/2键盘的中断 */
void inthandler21(int *esp);
/* 来自PS/2鼠标的中断 */
void inthandler2c(int *esp);
/* PIC0 电气原因造成的中断 */
void inthandler27(int *esp);

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
