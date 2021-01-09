#include "bootpack.h"

void make_window8(unsigned char *buf, int xsize, int ysize, char *title);
void make_textbox8(struct SHEET *sht, int x0, int y0, int sx, int sy, int c);
void task_b_main();

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

void HariMain(void)
{
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;	// 结构体指针指向储存系统信息的地址
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct FIFO32 fifo;
	int fifobuf[128];
	struct SHTCTL *shtctl;
	// 图层背景，鼠标
	struct SHEET *sht_back, *sht_mouse, *sht_win;
	// 定义背景缓冲区、鼠标缓冲区
	unsigned char *buf_back, buf_mouse[256], *buf_win;
	// 键盘按键字符表
	static char keytable[0x54] = {
		0,   0,   '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '+', 0,   0,
		'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '[', ']', 0,   0,   'A', 'S',
		'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', '\'', 0,   0,   '\\', 'Z', 'X', 'C', 'V',
		'B', 'N', 'M', ',', '.', '/', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
		'2', '3', '0', '.'
	};
	unsigned int memtotal;
	char s[40];
	struct TIMER *timer, *timer2, *timer3;
	int i, mx, my, cursor_x, cursor_c;
	struct TSS32 tss_a, tss_b;
	struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) ADR_GDT;

	init_gdtidt();							// 初始化 全局段记录表，中断记录表
	init_pic();								// 初始化 PIC
	io_sti();								// IDT/PIC 的初始化已经完成，开放 CPU 的中断
	init_pit();								// 设定定时器频率
	io_out8(PIC0_IMR, 0xf8);				// 开放PIC1和键盘中断(11111001)
	io_out8(PIC1_IMR, 0xef);				// 开放鼠标中断(11101111)
	fifo32_init(&fifo, 128, fifobuf);
	timer    = timer_alloc();
	timer2   = timer_alloc();
	timer3   = timer_alloc();
	timer_init(timer, &fifo, 10);
	timer_init(timer2, &fifo, 3);
	timer_init(timer3, &fifo, 1);
	timer_settime(timer, 1000);
	timer_settime(timer2, 300);
	timer_settime(timer3, 50);
	init_keyboard(&fifo, 256);
	enable_mouse(&fifo, 512, &mdec);
	memtotal = memtest(0x00400000, 0xbfffffff);
	memman_init(memman);
	memman_free(memman, 0x00001000, 0x0009e000); // 0x00001000 - 0x0009efff
	memman_free(memman, 0x00400000, memtotal - 0x00400000);

	set_segmdesc(gdt + 3, 103, (int) &tss_a, AR_TSS32);
	set_segmdesc(gdt + 4, 103, (int) &tss_b, AR_TSS32);
	load_tr(3 * 8);

	tss_a.ldtr   = 0;
	tss_b.ldtr   = 0;
	tss_a.iomap  = 0x40000000;
	tss_b.iomap  = 0x40000000;

	int task_b_esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024 - 8;
	tss_b.eip    = (int) &task_b_main;
	tss_b.eflags = 0x00000202;	// IF = 1
	tss_b.eax    = 0;
	tss_b.ecx    = 0;
	tss_b.edx    = 0;
	tss_b.ebx    = 0;
	tss_b.esp    = task_b_esp;
	tss_b.ebp    = 0;
	tss_b.esi    = 0;
	tss_b.edi    = 0;
	tss_b.es     = 1 * 8;
	tss_b.cs     = 2 * 8;
	tss_b.ss     = 1 * 8;
	tss_b.ds     = 1 * 8;
	tss_b.fs     = 1 * 8;
	tss_b.gs     = 1 * 8;

	init_palette();
	// 分配图层、内存
	shtctl    = shtctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);
	sht_back   = sheet_alloc(shtctl);
	sht_mouse = sheet_alloc(shtctl);
	sht_win   = sheet_alloc(shtctl);
	buf_back  = (unsigned char *) memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);
	buf_win   = (unsigned char *) memman_alloc_4k(memman, 160 * 52);
	sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1);	// 没有透明色
	sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99);	// 透明色号 99
	sheet_setbuf(sht_win, buf_win, 160, 52, -1);	// 没有透明色
	init_screen(buf_back, binfo->scrnx, binfo->scrny);
	init_mouse_cursor8(buf_mouse, 99);	// 背景色号 99
	// 创建窗口
	make_window8(buf_win, 160, 52, "window");
	// 创建输入盒子
	make_textbox8(sht_win, 8, 28, 144, 16, COL8_FFFFFF);
	cursor_x = 8;
	cursor_c = COL8_FFFFFF;
	mx = (binfo->scrnx - 16) / 2;
	my = (binfo->scrny - 28 - 16) / 2;
	// 设置在移动图层时进行局部画面刷新
	sheet_slide(sht_back, 0, 0);
	sheet_slide(sht_mouse, mx, my);
	sheet_slide(sht_win, 80, 72);
	// 设置叠加显示优先级
	sheet_updown(sht_back, 0);
	sheet_updown(sht_win, 1);
	sheet_updown(sht_mouse, 2);
	putfont8_pos(buf_back, binfo->scrnx, 0, 30, COL8_FFFFFF, "SPARK.OS");
	sprintf(s, "memory %dMB free: %dKB", memtotal / (1024 * 1024), memman_total(memman) / 1024);
	putfonts8_str(buf_back, binfo->scrnx, 0, 50, COL8_FFFFFF, s);
	sheet_refresh(sht_back, 0, 0, sht_back->bxsize, sht_back->bysize);

	*((int *) (task_b_esp + 4)) = (int) sht_back;
	mt_init();

	for (;;) {
		// 计时器开始
		// sprintf(s, "%010d", timerctl.count);
		// putfonts8_str_sht(sht_win, 40, 28, COL8_000000, COL8_C6C6C6, s);
		// 屏蔽其他中断
		io_cli();
		// 接收中断并进入等待
		if (fifo32_status(&fifo) == 0) io_stihlt();
		else {
			i = fifo32_get(&fifo);
			io_sti();
			// 处理键盘数据
			if (256 <= i && i <= 511) {
				sprintf(s, "%02X", i - 256);
				putfonts8_str_sht(sht_back, 0, 0, COL8_FFFFFF, COL8_008484, s);
				if (i < 256 + 0x54) {
					if (keytable[i - 256] != 0 && cursor_x < 144) {
						// 显示一个字符就后移一次光标
						s[0] = keytable[i - 256];
						s[1] = 0;
						putfonts8_str_sht(sht_win, cursor_x, 28, COL8_000000, COL8_FFFFFF, s);
						cursor_x += 8;
					}
				}
				// 退格键处理，前移一次光标
				if (i == 256 + 0x0e && cursor_x > 8) {
					putfonts8_str_sht(sht_win, cursor_x, 28, COL8_000000, COL8_FFFFFF, " ");
					cursor_x -= 8;
				}
				boxfill8(sht_win->buf, sht_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
				sheet_refresh(sht_win, cursor_x, 28, cursor_x + 8, 44);
			} else if (512 <= i && i <= 767) {
				if (mouse_decode(&mdec, i - 512) != 0) {
					sprintf(s, "[lcr %4d %4d]", mdec.x, mdec.y);
					if ((mdec.btn & 0x01) != 0) {
						s[1] = 'L';
						sheet_slide(sht_win, mx - 80, my - 8);
					}
					if ((mdec.btn & 0x02) != 0) s[3] = 'R';
					if ((mdec.btn & 0x04) != 0) s[2] = 'C';
					putfonts8_str_sht(sht_back, 0, 32, COL8_FFFFFF, COL8_008484, s);
					// 鼠标移动
					mx += mdec.x;
					my += mdec.y;
					// 防止超出屏幕
					if (mx < 0) mx = 0;
					if (my < 0) my = 0;
					if (mx > binfo->scrnx - 1) mx = binfo->scrnx - 1;
					if (my > binfo->scrny - 1) my = binfo->scrny - 1;
					sprintf(s, "(%4d %4d)", mx, my);
					// 显示坐标
					putfonts8_str_sht(sht_back, 0, 16, COL8_FFFFFF, COL8_008484, s);
					// 描绘鼠标
					sheet_slide(sht_mouse, mx, my);
				}
			}
			switch (i) {
				case 10:
					putfonts8_str_sht(sht_back, 0, 80, COL8_FFFFFF, COL8_008484, "10 sec");
					break;
				case 3:
					putfonts8_str_sht(sht_back, 0, 64, COL8_FFFFFF, COL8_008484, "3 sec");
					break;
				// 模拟光标
				case 1:
					timer_init(timer3, &fifo, 0);
					cursor_c = COL8_000000;
					boxfill8(sht_win->buf, sht_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
					timer_settime(timer3, 50);
					sheet_refresh(sht_win, cursor_x, 28, cursor_x + 8, 44);
					break;
				case 0:
					timer_init(timer3, &fifo, 1);
					cursor_c = COL8_FFFFFF;
					boxfill8(sht_win->buf, sht_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
					timer_settime(timer3, 50);
					sheet_refresh(sht_win, cursor_x, 28, cursor_x + 8, 44);
					break;
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

void make_textbox8(struct SHEET *sht, int x0, int y0, int sx, int sy, int c)
{
	int x1 = x0 + sx, y1 = y0 + sy;
	boxfill8(sht->buf, sht->bxsize, COL8_848484, x0 - 2, y0 - 3, x1 + 1, y0 - 3);
	boxfill8(sht->buf, sht->bxsize, COL8_848484, x0 - 3, y0 - 3, x0 - 3, y1 + 1);
	boxfill8(sht->buf, sht->bxsize, COL8_FFFFFF, x0 - 3, y1 + 2, x1 + 1, y1 + 2);
	boxfill8(sht->buf, sht->bxsize, COL8_FFFFFF, x1 + 2, y0 - 3, x1 + 2, y1 + 2);
	boxfill8(sht->buf, sht->bxsize, COL8_000000, x0 - 1, y0 - 2, x1 + 0, y0 - 2);
	boxfill8(sht->buf, sht->bxsize, COL8_000000, x0 - 2, y0 - 2, x0 - 2, y1 + 0);
	boxfill8(sht->buf, sht->bxsize, COL8_C6C6C6, x0 - 2, y1 + 1, x1 + 0, y1 + 1);
	boxfill8(sht->buf, sht->bxsize, COL8_C6C6C6, x1 + 1, y0 - 2, x1 + 1, y1 + 1);
	boxfill8(sht->buf, sht->bxsize, c,           x0 - 1, y0 - 1, x1 + 0, y1 + 0);
}

void task_b_main(struct SHEET *sht_back)
{
	struct FIFO32 fifo;
	struct TIMER *timer_put, *timer_1s;
	int fifobuf[128], count = 0, count0 = 0;
	char s[12];
	
	fifo32_init(&fifo, 128, fifobuf);
	timer_1s  = timer_alloc();
	timer_put = timer_alloc();
	timer_init(timer_1s, &fifo, 100);
	timer_init(timer_put, &fifo, 1);
	timer_settime(timer_1s, 100);
	timer_settime(timer_put, 1);
	putfonts8_str_sht(sht_back, 0, 160, COL8_FFFFFF, COL8_008484, "HELLO MULTITASK");

	for (;;) {
		io_cli();
		count++;
		if (fifo32_status(&fifo) == 0) io_sti();
		else {
			io_sti();
			switch (fifo32_get(&fifo)) {
				case 1:
					sprintf(s, "%011d", count);
					putfonts8_str_sht(sht_back, 0, 144, COL8_FFFFFF, COL8_008484, s);
					break;
				case 100:
					sprintf(s, "%011d", count - count0);
					putfonts8_str_sht(sht_back, 0, 128, COL8_FFFFFF, COL8_008484, s);
					count0 = count;
					timer_settime(timer_1s, 100);
					break;
			}
		}
	}
}