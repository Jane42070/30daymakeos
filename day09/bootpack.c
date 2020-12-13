#include "bootpack.h"

#define EFLAGS_AC_BIT		0x00040000
#define CR0_CACHE_DISABLE	0x60000000

unsigned int memtest(unsigned int start, unsigned int end);

void HariMain(void)
{
	// 结构体指针指向储存系统信息的地址
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
	// ASCII 字体
	extern char hankaku[4096];
	extern struct FIFO8 keyfifo, mousefifo;
	extern struct MOUSE_DEC mdec;
	// Define FIFO buffer
	unsigned char mcursor[256], keybuf[32], mousebuf[128];
	char s[40];
	int mx = (binfo->scrnx - 16) / 2; /* 屏幕 */
	int my = (binfo->scrny - 28 - 16) / 2;
	int i, j;

	// 初始化 全局段记录表，中断记录表
	init_gdtidt();
	// 初始化 PIC
	init_pic();
	// IDT/PIC 的初始化已经完成，开放 CPU 的中断
	io_sti();

	// 开放PIC1和键盘中断(11111001)
	io_out8(PIC0_IMR, 0xf9);
	// 开放鼠标中断(11101111)
	io_out8(PIC1_IMR, 0xef);

	// 尽快的开放中断接收数据
	// 所以初始化调色板应放在开放中断之后
	init_palette();
	init_screen((unsigned char *)binfo->vram, binfo->scrnx, binfo->scrny);
	init_mouse_cursor8(mcursor, COL8_008484);
	putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
	putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
	sprintf((char *)s, "(%d, %d)", mx, my);
	putfont8_str(binfo->vram, binfo->scrnx, 0, 16, COL8_FFFFFF, s);

	putfont8_pos(binfo->vram, binfo->scrnx, 0, 30, COL8_FFFFFF, "CLAY");

	i = memtest(0x00400000, 0xbfffffff) / (1024 * 1024);
	sprintf(s, "memory %dMB", i);
	putfont8_pos(binfo->vram, binfo->scrnx, 0, 46, COL8_FFFFFF, s);
	// 初始化keybuf缓冲区
	fifo8_init(&keyfifo, 32, keybuf);
	// 初始化mousebuf缓冲区
	fifo8_init(&mousefifo, 128, mousebuf);

	init_keyboard();
	enable_mouse(&mdec);

	// 储存键盘数据
	for (;;) {
		// 屏蔽其他中断
		io_cli();
			// 接收中断并进入等待
		if (fifo8_status(&keyfifo) + fifo8_status(&mousefifo) == 0) { io_stihlt(); }
		else {
			if (fifo8_status(&keyfifo) != 0) {
				i = fifo8_get(&keyfifo);
				io_sti();
				sprintf(s, "%02X", i);
				boxfill8((unsigned char *)binfo->vram, binfo->scrnx, COL8_008484, 0, 0, 16, 16);
				putfont8_str(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);
			}
			else if (fifo8_status(&mousefifo) != 0)
			{
				i = fifo8_get(&mousefifo);
				io_sti();
				if (mouse_decode(&mdec, i) != 0) {
					sprintf(s, "[lcr %4d %4d]", mdec.x, mdec.y);
					if ((mdec.btn & 0x01) != 0) s[1] = 'L' ;
					if ((mdec.btn & 0x02) != 0) s[3] = 'R' ;
					if ((mdec.btn & 0x04) != 0) s[2] = 'C' ;
					boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 32, 16 + 15 * 8 - 1, 48);
					putfont8_str(binfo->vram, binfo->scrnx, 0, 32, COL8_FFFFFF, s);
					// 鼠标移动
					// 隐藏鼠标
					boxfill8(binfo->vram, binfo->scrnx, COL8_008484, mx, my, mx + 15, my + 15);
					mx += mdec.x;
					my += mdec.y;
					// 防止超出屏幕
					if (mx < 0) mx = 0;
					if (my < 0) my = 0;
					if (mx > binfo->scrnx - 16) mx = binfo->scrnx - 16;
					if (my > binfo->scrny - 16) my = binfo->scrny - 16;
					sprintf(s, "(%3d %3d)", mx, my);
					// 隐藏坐标
					boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 16, 100, 32);
					// 显示坐标
					putfont8_str(binfo->vram, binfo->scrnx, 0, 16, COL8_FFFFFF, s);
					// 描绘鼠标
					putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
				}
			}
		}
	}
}

unsigned int memtest(unsigned int start, unsigned int end)
{
	// CPU 架构 >= 486 才有高速缓存
	// 486CPU 的 EFLAGS寄存器 18 位存储着AC标志位
	// 386CPU 这位为 0
	char flg486 = 0;
	unsigned int eflg, cr0, i;

	/* 确认CPU是386还是486以上 */
	eflg = io_load_eflags();
	/* AC-bit = 1 */
	eflg |= EFLAGS_AC_BIT;
	io_store_eflags(eflg);
	eflg = io_load_eflags();
	// 如果是 386，即使设置AC=1，AC的值还是会回到 0
	if ((eflg & EFLAGS_AC_BIT) != 0) { flg486 = 1; }
	/* AC-bit = 1 */
	// 取反
	eflg &= ~EFLAGS_AC_BIT;
	io_store_eflags(eflg);

	// 禁止缓存
	// 对CR0寄存器操作
	if (flg486 != 0) {
		cr0 = load_cr0();
		cr0 |= CR0_CACHE_DISABLE;
		store_cr0(cr0);
	}

	i = memtest_sub(start, end);

	// 允许缓存
	if (flg486 != 0) {
		cr0 = load_cr0();
		cr0 &= ~CR0_CACHE_DISABLE;
		store_cr0(cr0);
	}

	return i;
}
