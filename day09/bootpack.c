#include "bootpack.h"

#define EFLAGS_AC_BIT		0x00040000
#define CR0_CACHE_DISABLE	0x60000000
#define MEMMAN_FREES		4090
#define MEMMAN_ADDR			0x003c0000

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
void memman_init(struct MEMMAN *man);										// 初始化内存管理
unsigned int memman_total(struct MEMMAN *man);								// 报告总空余内存
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size);			// 分配内存
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size);	// 释放内存

void HariMain(void)
{
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;	// 结构体指针指向储存系统信息的地址
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	unsigned int memtotal;
	char s[40];
	unsigned char mcursor[256], keybuf[32], mousebuf[128];		// Define FIFO buffer
	int i, j, mx, my;

	init_gdtidt();							// 初始化 全局段记录表，中断记录表
	init_pic();								// 初始化 PIC
	io_sti();								// IDT/PIC 的初始化已经完成，开放 CPU 的中断
	io_out8(PIC0_IMR, 0xf9);				// 开放PIC1和键盘中断(11111001)
	io_out8(PIC1_IMR, 0xef);				// 开放鼠标中断(11101111)
	fifo8_init(&keyfifo, 32, keybuf);		// 初始化keybuf缓冲区
	fifo8_init(&mousefifo, 128, mousebuf);	// 初始化mousebuf缓冲区

	init_keyboard();
	enable_mouse(&mdec);
	memtotal = memtest(0x00400000, 0xbfffffff);
	memman_init(memman);
	memman_free(memman, 0x00001000, 0x0009e000); // 0x00001000 - 0x0009efff
	memman_free(memman, 0x00400000, memtotal - 0x00400000);

	init_palette();
	init_screen(binfo->vram, binfo->scrnx, binfo->scrny);
	mx = (binfo->scrnx - 16) / 2;
	my = (binfo->scrny - 28 - 16) / 2;
	init_mouse_cursor8(mcursor, COL8_008484);
	putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
	sprintf(s, "memory %dMB free: %dKB", memtotal / (1024 * 1024), memman_total(memman) / 1024);
	putfont8_str(binfo->vram, binfo->scrnx, 0, 50, COL8_FFFFFF, s);
	putfont8_pos(binfo->vram, binfo->scrnx, 0, 30, COL8_FFFFFF, "CLAY");

	for (;;) {
		// 屏蔽其他中断
		io_cli();
			// 接收中断并进入等待
		if (fifo8_status(&keyfifo) + fifo8_status(&mousefifo) == 0) io_stihlt();
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
					if ((mdec.btn & 0x01) != 0) s[1] = 'L';
					if ((mdec.btn & 0x02) != 0) s[3] = 'R';
					if ((mdec.btn & 0x04) != 0) s[2] = 'C';
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

// 初始化内存管理
void memman_init(struct MEMMAN *man)
{
	man->frees    = 0;	// 可用信息数目
	man->maxfrees = 0;	// 用于观察可用状况：frees的最大值
	man->lostsize = 0;	// 释放失败的内存大小的总和
	man->losts    = 0;	// 释放失败次数
}

// 报告总空余内存
unsigned int memman_total(struct MEMMAN *man)
{
	unsigned int i, t = 0;
	for (i = 0; i < man->frees; i++) t += man->free[i].size;
	return t;
}

// 分配内存
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size)
{
	unsigned int i, a;
	for (i = 0; i < man->frees; i++) {
		// 有足够大的内存
		if (man->free[i].size >= size) {
			a = man->free[i].addr;
			man->free[i].addr += size;
			man->free[i].size -= size;
			// free[i] 变为 0，就删掉一条可用信息
			if (man->free[i].size == 0) {
				man->frees--;
				for (; i < man->frees; i++) {
					// 往前移送
					man->free[i] = man->free[i + 1];
				}
			}
			return a;
		}
	}
	// 没有可用空间
	return 0;
}

// 释放内存
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size)
{
	int i, j;
	// 为便于归纳内存，将 free[] 按照 addr 的顺序排列
	// 所以，先决定放在哪里
	for (i = 0; i < man->frees; i++) {
		if (man->free[i].addr > addr) break;
	}
	// free[i - 1].addr < addr < free[i].addr
	if (i > 0) {
		// 前面可用内存
		if (man->free[i - 1].addr + man->free[i - 1].size == addr) {
			// 可以与前面的内存归纳到一起
			man->free[i - 1].size += size;
			if (i < man->frees) {
				// 后面也有
				if (addr + size == man->free[i].addr) {
					// 也可以与后面的归纳到一起
					man->free[i - 1].size += man->free[i].size;
					// man->free[i] 删除
					// free[i]变成0后归纳到前面去
					man->frees--;
					for (; i < man->frees; i++) {
						// 往前移送
						man->free[i] = man->free[i + 1];
					}
				}
			}
			// 成功完成
			return 0;
		}
	}
	// 无法与前面的可用空间归纳到一起
	if (i < man->frees) {
		// 后面还有
		if (addr + size == man->free[i].addr) {
			// 可以和后面的内容归纳到一起
			man->free[i].addr = addr;
			man->free[i].size += size;
			// 成功完成
			return 0;
		}
	}
	// 既不能与前面归纳到一起，也不能和后面归纳到一起
	if (man->frees < MEMMAN_FREES) {
		// free[i] 之后的，向后移动，腾出一点可用空间
		for (j = man->frees; j > i; j--) {
			man->free[j] = man->free[j - 1];
		}
		man->frees++;
		// 更新最大值
		if (man->maxfrees < man->frees) man->maxfrees = man->frees;
		man->free[i].addr = addr;
		man->free[i].size = size;
		// 成功完成
		return 0;
	}
	// 不能往后移动
	man->losts++;
	man->lostsize += size;
	// 失败
	return -1;
}
