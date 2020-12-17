#include "bootpack.h"

void make_window8(unsigned char *buf, int xsize, int ysize, char *title);

void HariMain(void)
{
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;	// 结构体指针指向储存系统信息的地址
	// 内存管理
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	// 图层管理
	struct SHTCTL *shtctl;
	// 图层背景，鼠标
	struct SHEET *sht_bak, *sht_mouse, *sht_win;
	// 定义背景缓冲区、鼠标缓冲区
	unsigned char *buf_back, buf_mouse[256], *buf_win;

	unsigned int memtotal;
	char s[40];
	unsigned char mcursor[256], keybuf[32], mousebuf[128], timerbuf[8];		// Define FIFO buffer
	int i, j, mx, my;

	init_gdtidt();							// 初始化 全局段记录表，中断记录表
	init_pic();								// 初始化 PIC
	io_sti();								// IDT/PIC 的初始化已经完成，开放 CPU 的中断
	init_pit();								// 设定定时器频率
	io_out8(PIC0_IMR, 0xf8);				// 开放PIC1和键盘中断(11111001)
	io_out8(PIC1_IMR, 0xef);				// 开放鼠标中断(11101111)
	fifo8_init(&keyfifo, 32, keybuf);		// 初始化keybuf缓冲区
	fifo8_init(&mousefifo, 128, mousebuf);	// 初始化mousebuf缓冲区
	fifo8_init(&timerfifo, 8, timerbuf);	// 初始化timerbuf缓冲区

	settimer(1000, &timerfifo, 1);

	init_keyboard();
	enable_mouse(&mdec);
	memtotal = memtest(0x00400000, 0xbfffffff);
	memman_init(memman);
	memman_free(memman, 0x00001000, 0x0009e000); // 0x00001000 - 0x0009efff
	memman_free(memman, 0x00400000, memtotal - 0x00400000);

	init_palette();
	// 分配图层、内存
	shtctl    = shtctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);
	sht_bak   = sheet_alloc(shtctl);
	sht_mouse = sheet_alloc(shtctl);
	sht_win   = sheet_alloc(shtctl);
	buf_back  = (unsigned char *) memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);
	buf_win   = (unsigned char *) memman_alloc_4k(memman, 160 * 52);

	sheet_setbuf(sht_bak, buf_back, binfo->scrnx, binfo->scrny, -1);	// 没有透明色
	sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99);	// 透明色号 99
	sheet_setbuf(sht_win, buf_win, 160, 52, -1);	// 没有透明色

	init_screen(buf_back, binfo->scrnx, binfo->scrny);
	init_mouse_cursor8(buf_mouse, 99);	// 背景色号 99

	// 创建窗口
	make_window8(buf_win, 160, 52, "Counter");
	// putfonts8_str(buf_win, 160, 24, 28, COL8_000000, "Welcome to");
	// putfonts8_str(buf_win, 160, 24, 44, COL8_000000, "Spark-OS!");

	mx = (binfo->scrnx - 16) / 2;
	my = (binfo->scrny - 28 - 16) / 2;

	// 设置在移动图层时进行局部画面刷新
	sheet_slide(sht_bak, 0, 0);
	sheet_slide(sht_mouse, mx, my);
	sheet_slide(sht_win, 80, 72);

	// 设置叠加显示优先级
	sheet_updown(sht_bak, 0);
	sheet_updown(sht_win, 1);
	sheet_updown(sht_mouse, 2);

	sprintf(s, "memory %dMB free: %dKB", memtotal / (1024 * 1024), memman_total(memman) / 1024);
	putfonts8_str(buf_back, binfo->scrnx, 0, 50, COL8_FFFFFF, s);
	putfont8_pos(buf_back, binfo->scrnx, 0, 30, COL8_FFFFFF, "CLAY");

	// 全局画面刷新
	sheet_refresh(sht_bak, 0, 0, binfo->scrnx, 80);

	for (;;) {
		// 计时器开始
		sprintf(s, "%010d", timerctl.count);
		boxfill8(buf_win, 160, COL8_C6C6C6, 40, 28, 119, 43);
		putfonts8_str(buf_win, 160, 40, 28, COL8_000000, s);

		// 结束
		sheet_refresh(sht_win, 40, 28, 120, 44);

		// 屏蔽其他中断
		io_cli();
		// 接收中断并进入等待
		if (fifo8_status(&keyfifo) + fifo8_status(&mousefifo) + fifo8_status(&timerfifo) == 0) io_sti();
		else {
			if (fifo8_status(&keyfifo) != 0) {
				i = fifo8_get(&keyfifo);
				io_sti();
				sprintf(s, "%02X", i);
				boxfill8(buf_back, binfo->scrnx, COL8_008484, 0, 0, 16, 16);
				putfonts8_str(buf_back, binfo->scrnx, 0, 0, COL8_FFFFFF, s);
				sheet_refresh(sht_bak, 0, 0, 16, 16);
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
					boxfill8(buf_back, binfo->scrnx, COL8_008484, 0, 32, 16 + 15 * 8 - 1, 48);
					putfonts8_str(buf_back, binfo->scrnx, 0, 32, COL8_FFFFFF, s);
					// 鼠标移动
					mx += mdec.x;
					my += mdec.y;
					// 防止超出屏幕
					if (mx < 0) mx = 0;
					if (my < 0) my = 0;
					if (mx > binfo->scrnx - 1) mx = binfo->scrnx - 1;
					if (my > binfo->scrny - 1) my = binfo->scrny - 1;
					sprintf(s, "(%3d %3d)", mx, my);
					// 隐藏坐标
					boxfill8(buf_back, binfo->scrnx, COL8_008484, 0, 16, 100, 32);
					// 显示坐标
					putfonts8_str(buf_back, binfo->scrnx, 0, 16, COL8_FFFFFF, s);
					// 描绘鼠标
					// 包含 sheet_refresh
					sheet_refresh(sht_bak, 0, 16, 16 + 15 * 8 - 1, 48);
					sheet_slide(sht_mouse, mx, my);
				}
			}
			else if (fifo8_status(&timerfifo) != 0)
			{
				i = fifo8_get(&timerfifo);
				io_sti();
				putfonts8_str(buf_back, binfo->scrnx, 0, 64, COL8_FFFFFF, "10 sec");
				sheet_refresh(sht_bak, 0, 64, 56, 80);
			}
		}
	}
}

void make_window8(unsigned char *buf, int xsize, int ysize, char *title)
{
	static char closebtn[14][16] = {
		"OOOOOOOOOOOOOOO@",
		"OQQQQQQQQQQQQQ$@",
		"OQQQQQQQQQQQQQ$@",
		"OQQQ@@QQQQ@@QQ$@",
		"OQQQQ@@QQ@@QQQ$@",
		"OQQQQQ@@@@QQQQ$@",
		"OQQQQQQ@@QQQQQ$@",
		"OQQQQQ@@@@QQQQ$@",
		"OQQQQ@@QQ@@QQQ$@",
		"OQQQ@@QQQQ@@QQ$@",
		"OQQQQQQQQQQQQQ$@",
		"OQQQQQQQQQQQQQ$@",
		"O$$$$$$$$$$$$$$@",
		"@@@@@@@@@@@@@@@@"
	};
	int x, y;
	char c;
	boxfill8(buf, xsize, COL8_C6C6C6, 0,         0,         xsize - 1, 0        );
	boxfill8(buf, xsize, COL8_FFFFFF, 1,         1,         xsize - 2, 1        );
	boxfill8(buf, xsize, COL8_C6C6C6, 0,         0,         0,         ysize - 1);
	boxfill8(buf, xsize, COL8_FFFFFF, 1,         1,         1,         ysize - 2);
	boxfill8(buf, xsize, COL8_848484, xsize - 2, 1,         xsize - 2, ysize - 2);
	boxfill8(buf, xsize, COL8_000000, xsize - 1, 0,         xsize - 1, ysize - 1);
	boxfill8(buf, xsize, COL8_C6C6C6, 2,         2,         xsize - 3, ysize - 3);
	boxfill8(buf, xsize, COL8_000084, 3,         3,         xsize - 4, 20       );
	boxfill8(buf, xsize, COL8_848484, 1,         ysize - 2, xsize - 2, ysize - 2);
	boxfill8(buf, xsize, COL8_000000, 0,         ysize - 1, xsize - 1, ysize - 1);
	putfonts8_str(buf, xsize, 24, 4, COL8_FFFFFF, title);
	for (y = 0; y < 14; y++) {
		for (x = 0; x < 16; x++) {
			c = closebtn[y][x];
			if (c == '@') {
				c = COL8_000000;
			} else if (c == '$') {
				c = COL8_848484;
			} else if (c == 'Q') {
				c = COL8_C6C6C6;
			} else {
				c = COL8_FFFFFF;
			}
			buf[(5 + y) * xsize + (xsize - 21 + x)] = c;
		}
	}
}
