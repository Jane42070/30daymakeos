#include "bootpack.h"
#define KEYCMD_LED 0xed
// 文件信息
struct FILEINFO {
	unsigned char name[8], ext[3], type;
	char reserve[10];
	unsigned short time, date, clustno;
	unsigned int size;
};

void make_window8(unsigned char *buf, int xsize, int ysize, char *title, char act);
void make_wtitle8(unsigned char *buf, int xsize, char *title, char act);
void make_textbox8(struct SHEET *sht, int x0, int y0, int sx, int sy, int c);
void console_task(struct SHEET *sheet, unsigned int memtotal);
int cons_newline(int cursor_y, struct SHEET *sheet);

void HariMain(void)
{
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;	// 结构体指针指向储存系统信息的地址
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct FIFO32 fifo, keycmd;
	int fifobuf[128], keycmd_buf[32];
	struct SHTCTL *shtctl;
	// 图层背景，鼠标
	struct SHEET *sht_back, *sht_mouse, *sht_win, *sht_cons;
	// 定义背景缓冲区、鼠标缓冲区
	unsigned char *buf_back, buf_mouse[256], *buf_win, *buf_cons;
	// 键盘按键字符表
	static char keytable0[0x80] = {
		0,   0,   '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0,   0,
		'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '[', ']', 0,   0,   'A', 'S',
		'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', '\'', 0,   0,   '\\', 'Z', 'X', 'C', 'V',
		'B', 'N', 'M', ',', '.', '/', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
		'2', '3', '0', '.', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   0x5c, 0,  0,   0,   0,   0,   0,   0,   0,   0,   0x5c, 0,  0
	};
	static char keytable1[0x80] = {
		0,   0,   '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 0,   0,
		'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '|',   0,   'A', 'S',
		'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', 0,   0,   '}', 'Z', 'X', 'C', 'V',
		'B', 'N', 'M', '<', '>', '?', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
		'2', '3', '0', '.', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   '_', 0,   0,   0,   0,   0,   0,   0,   0,   0,   '|', 0,   0
	};
	int key_to = 0, key_shift = 0, key_leds = (binfo->leds >> 4) & 7, keycmd_wait = -1;

	unsigned int memtotal;
	char s[40];
	struct TIMER *timer;
	int i, mx, my, cursor_x, cursor_c;
	struct TASK *task_a, *task_cons;

	init_gdtidt();							// 初始化 全局段记录表，中断记录表
	init_pic();								// 初始化 PIC
	io_sti();								// IDT/PIC 的初始化已经完成，开放 CPU 的中断
	init_pit();								// 设定定时器频率
	io_out8(PIC0_IMR, 0xf8);				// 开放PIC1和键盘中断(11111001)
	io_out8(PIC1_IMR, 0xef);				// 开放鼠标中断(11101111)
	fifo32_init(&fifo, 128, fifobuf, 0);
	fifo32_init(&keycmd, 32, keycmd_buf, 0);
	timer   = timer_alloc();
	timer_init(timer, &fifo, 1);
	timer_settime(timer, 50);
	init_keyboard(&fifo, 256);
	enable_mouse(&fifo, 512, &mdec);
	memtotal = memtest(0x00400000, 0xbfffffff);
	memman_init(memman);
	memman_free(memman, 0x00001000, 0x0009e000); // 0x00001000 - 0x0009efff
	memman_free(memman, 0x00400000, memtotal - 0x00400000);

	init_palette();
	// 分配图层、内存
	shtctl    = shtctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);
	task_a    = task_init(memman);
	fifo.task = task_a;
	task_run(task_a, 1, 0);

	// sht_back
	sht_back = sheet_alloc(shtctl);
	buf_back = (unsigned char *) memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);
	sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1);	// 没有透明色
	init_screen(buf_back, binfo->scrnx, binfo->scrny);

	// sht_cons
	sht_cons = sheet_alloc(shtctl);
	buf_cons = (unsigned char *) memman_alloc_4k(memman, 256 * 165);
	sheet_setbuf(sht_cons, buf_cons, 256, 165, -1);
	make_window8(buf_cons, 256, 165, "terminal", 0);
	make_textbox8(sht_cons, 8, 28, 240, 128, COL8_000000);
	task_cons = task_alloc();
	task_cons->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024 - 12;
	task_cons->tss.eip = (int) &console_task;
	task_cons->tss.es = 1 * 8;
	task_cons->tss.cs = 2 * 8;
	task_cons->tss.ss = 1 * 8;
	task_cons->tss.ds = 1 * 8;
	task_cons->tss.fs = 1 * 8;
	task_cons->tss.gs = 1 * 8;
	*((int *) (task_cons->tss.esp + 4)) = (int) sht_cons;
	*((int *) (task_cons->tss.esp + 8)) = memtotal;
	task_run(task_cons, 2, 2);	// 第二等级，0.02秒

	// sht_win
	sht_win   = sheet_alloc(shtctl);
	buf_win   = (unsigned char *) memman_alloc_4k(memman, 160 * 52);
	sheet_setbuf(sht_win, buf_win, 144, 52, -1);			// 没有透明色
	make_window8(buf_win, 144, 52, "task_a", 1);			// 创建窗口
	make_textbox8(sht_win, 8, 28, 128, 16, COL8_FFFFFF);	// 创建输入盒子
	cursor_x = 8;

	// sht_mouse
	sht_mouse = sheet_alloc(shtctl);
	sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99);	// 透明色号 99
	init_mouse_cursor8(buf_mouse, 99);	// 背景色号 99
	cursor_c = COL8_FFFFFF;
	mx = (binfo->scrnx - 16) / 2;
	my = (binfo->scrny - 28 - 16) / 2;

	// 设置在移动图层时进行局部画面刷新
	sheet_slide(sht_back,  0,   0);
	sheet_slide(sht_cons,  32,  4);
	sheet_slide(sht_win,   8,  56);
	sheet_slide(sht_mouse, mx, my);
	// 设置叠加显示优先级
	sheet_updown(sht_back, 0);
	sheet_updown(sht_cons, 1);
	sheet_updown(sht_win,  2);
	sheet_updown(sht_mouse,3);

	// 为了避免和键盘当前状态起冲突，在一开始先进行设置
	fifo32_put(&keycmd, KEYCMD_LED);
	fifo32_put(&keycmd, key_leds);
	for (;;) {
		if (fifo32_status(&keycmd) > 0 && keycmd_wait < 0) {
			// 如果存在向键盘控制器发送的数据，则发送它
			keycmd_wait = fifo32_get(&keycmd);
			wait_KBC_sendready();
			io_out8(PORT_KEYDAT, keycmd_wait);
		}
		// 屏蔽其他中断
		io_cli();
		// 接收中断并进入等待
		if (fifo32_status(&fifo) == 0) {
			task_sleep(task_a);
			io_sti();
		} else {
			i = fifo32_get(&fifo);
			io_sti();
			// 处理键盘数据
			if (256 <= i && i <= 511) {
				if (i < 0x80 + 256) {// 将按键编码转化为字符编码
					if (key_shift == 0) s[0] = keytable0[i - 256];
					else s[0] = keytable1[i - 256];
				}
				else s[0] = 0;
				if ('A' <= s[0] && s[0] <= 'Z') {// 当输入的字符为英文字符时
					// 当 CapsLock 和 Shift 没有打开，或者都打开时
					if (((key_leds & 4) == 0 && key_shift == 0) || ((key_leds & 4) != 0 && key_shift != 0)) {
						s[0] += 0x20;	// 将大写字母转化为小写字母
					}
				}
				if (s[0] != 0) {// 一般字符
					// 显示一个字符就后移一次光标
					if (key_to == 0) {// 发送给任务 A
						if (cursor_x < 128) {
							s[1] = 0;
							putfonts8_str_sht(sht_win, cursor_x, 28, COL8_000000, COL8_FFFFFF, s);
							cursor_x += 8;
						}
					}
					// 发送给任务终端
					else fifo32_put(&task_cons->fifo, s[0] + 256);
				}
				switch (i) {
					case 256 + 0x0e:// 退格键
						if (key_to == 0) {
							if (cursor_x > 8) {
								// 用空白擦除光标后光标前移一位
								putfonts8_str_sht(sht_win, cursor_x, 28, COL8_000000, COL8_FFFFFF, " ");
								cursor_x -= 8;
							}
						}
						else fifo32_put(&task_cons->fifo, 8 + 256);
						break;
					case 256 + 0x0f:// TAB键处理
						if (key_to == 0) {// 切换至终端
							key_to = 1;
							sheet_updown(sht_cons, 2);
							sheet_updown(sht_win,  1);
							make_wtitle8(buf_win, sht_win->bxsize, "task_a", 0);
							make_wtitle8(buf_cons, sht_cons->bxsize, "terminal", 1);
							cursor_c = -1; // 不显示光标
							boxfill8(sht_win->buf, sht_win->bxsize, COL8_FFFFFF, cursor_x, 28, cursor_x + 7, 43);
							fifo32_put(&task_cons->fifo, 2);	// 命令行窗口光标 ON
						} else {// 切换至任务 A
							key_to = 0;
							sheet_updown(sht_cons, 1);
							sheet_updown(sht_win,  2);
							make_wtitle8(buf_win, sht_win->bxsize, "task_a", 1);
							make_wtitle8(buf_cons, sht_cons->bxsize, "terminal", 0);
							cursor_c = COL8_000000;	// 显示光标
							fifo32_put(&task_cons->fifo, 3);	// 命令行窗口光标 OFF
						}
						sheet_refresh(sht_win, 0, 0, sht_win->bxsize, 21);
						sheet_refresh(sht_cons, 0, 0, sht_cons->bxsize, 21);
						if (cursor_c >= 0) boxfill8(sht_win->buf, sht_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
						sheet_refresh(sht_win, cursor_x, 28, cursor_x + 8, 44);
						break;
					case 256 + 0x1c:// 回车键
						if (key_to != 0) fifo32_put(&task_cons->fifo, 10 + 256);	// 发送命令到终端
						break;
					case 256 + 0x2a:// lShift ON
						key_shift |= 1;
						break;
					case 256 + 0x36:// rShift ON
						key_shift |= 2;
						break;
					case 256 + 0xaa:// lShift OFF
						key_shift &= ~1;
						break;
					case 256 + 0xb6:// rShift OFF
						key_shift &= ~2;
						break;
					case 256 + 0x3a:// CapsLock
						key_leds ^= 4;
						fifo32_put(&keycmd, KEYCMD_LED);
						fifo32_put(&keycmd, key_leds);
						break;
					case 256 + 0x45:// NumLock
						key_leds ^= 2;
						fifo32_put(&keycmd, KEYCMD_LED);
						fifo32_put(&keycmd, key_leds);
						break;
					case 256 + 0x46:// ScrollLock
						key_leds ^= 1;
						fifo32_put(&keycmd, KEYCMD_LED);
						fifo32_put(&keycmd, key_leds);
						break;
					case 256 + 0xfa:// 键盘成功接收到数据
						keycmd_wait = -1;
						break;
					case 256 + 0xfe:// 键盘没有成功接收到数据
						wait_KBC_sendready();
						io_out8(PORT_KEYDAT, keycmd_wait);
						break;
				}
			} else if (512 <= i && i <= 767) {
				if (mouse_decode(&mdec, i - 512) != 0) {
					sprintf(s, "[lcr %4d %4d]", mdec.x, mdec.y);
					if ((mdec.btn & 0x01) != 0) sheet_slide(sht_win, mx - 80, my - 8);
					// s[1] = 'L';
					// if ((mdec.btn & 0x02) != 0) s[3] = 'R';
					// if ((mdec.btn & 0x04) != 0) s[2] = 'C';
					// 鼠标移动
					mx += mdec.x;
					my += mdec.y;
					// 防止超出屏幕
					if (mx < 0) mx = 0;
					if (my < 0) my = 0;
					if (mx > binfo->scrnx - 1) mx = binfo->scrnx - 1;
					if (my > binfo->scrny - 1) my = binfo->scrny - 1;
					// 描绘鼠标
					sheet_slide(sht_mouse, mx, my);
				}
			}
			switch (i) {
				// 模拟光标
				case 1:
					timer_init(timer, &fifo, 0);
					timer_settime(timer, 50);
					if (cursor_c >= 0) {
						cursor_c = COL8_000000;
						boxfill8(sht_win->buf, sht_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
						sheet_refresh(sht_win, cursor_x, 28, cursor_x + 8, 44);
					}
					break;
				case 0:
					timer_init(timer, &fifo, 1);
					timer_settime(timer, 50);
					if (cursor_c >= 0) {
						cursor_c = COL8_FFFFFF;
						boxfill8(sht_win->buf, sht_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
						sheet_refresh(sht_win, cursor_x, 28, cursor_x + 8, 44);
					}
					break;
			}
		}
	}
}

void make_window8(unsigned char *buf, int xsize, int ysize, char *title, char act)
{
	boxfill8(buf, xsize, COL8_C6C6C6, 0,         0,         xsize - 1, 0        );
	boxfill8(buf, xsize, COL8_FFFFFF, 1,         1,         xsize - 2, 1        );
	boxfill8(buf, xsize, COL8_C6C6C6, 0,         0,         0,         ysize - 1);
	boxfill8(buf, xsize, COL8_FFFFFF, 1,         1,         1,         ysize - 2);
	boxfill8(buf, xsize, COL8_848484, xsize - 2, 1,         xsize - 2, ysize - 2);
	boxfill8(buf, xsize, COL8_000000, xsize - 1, 0,         xsize - 1, ysize - 1);
	boxfill8(buf, xsize, COL8_C6C6C6, 2,         2,         xsize - 3, ysize - 3);
	boxfill8(buf, xsize, COL8_848484, 1,         ysize - 2, xsize - 2, ysize - 2);
	boxfill8(buf, xsize, COL8_000000, 0,         ysize - 1, xsize - 1, ysize - 1);
	make_wtitle8(buf, xsize, title, act);
}

void make_wtitle8(unsigned char *buf, int xsize, char *title, char act)
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
	char c, tc, tbc;
	if (act != 0) {
		tc = COL8_FFFFFF;
		tbc = COL8_000084;
	} else {
		tc = COL8_C6C6C6;
		tbc = COL8_848484;
	}
	boxfill8(buf, xsize, tbc, 3, 3, xsize - 4, 20);
	putfonts8_str(buf, xsize, 24, 4, tc, title);
	for (y = 0; y < 14; y++) {
		for (x = 0; x < 16; x++) {
			c = closebtn[y][x];
			switch (c) {
				case '@':
					c = COL8_000000;
					break;
				case '$':
					c = COL8_848484;
					break;
				case 'Q':
					c = COL8_C6C6C6;
					break;
				default:
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

void console_task(struct SHEET *sheet, unsigned int memtotal)
{
	struct TIMER *timer;
	struct TASK *task = task_now();
	int i, fifobuf[128], cursor_x = 16, cursor_y = 28, cursor_c = -1, x, y;
	fifo32_init(&task->fifo, 128, fifobuf, task);
	struct FILEINFO *finfo = (struct FILEINFO *) (ADR_DISKING + 0x002600);

	timer  = timer_alloc();
	timer_init(timer, &task->fifo, 1);
	timer_settime(timer, 50);
	char s[30] = {0}, cmdline[30] = {0};
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	// 显示提示符
	putfonts8_str_sht(sheet, 8, 28, COL8_FFFFFF, COL8_000000, ">");

	for (;;) {
		io_cli();
		if (fifo32_status(&task->fifo) == 0) {
			task_sleep(task);
			io_sti();
		} else {
			io_sti();
			i = fifo32_get(&task->fifo);
			switch (i) {
				case 3:// 开启光标
					boxfill8(sheet->buf, sheet->bxsize, COL8_000000, cursor_x, 28, cursor_x + 7, 43);
					cursor_c = -1;
					break;
				case 2:// 关闭光标
					cursor_c = COL8_FFFFFF;
					break;
				case 1:// 光标显示
					timer_init(timer, &task->fifo, 0);
					timer_settime(timer, 50);
					if (cursor_c >= 0) cursor_c = COL8_FFFFFF;
					// 刷新光标
					boxfill8(sheet->buf, sheet->bxsize, cursor_c, cursor_x, cursor_y, cursor_x + 7, cursor_y + 15);
					sheet_refresh(sheet, cursor_x, cursor_y, cursor_x + 8, cursor_y + 16);
					break;
				case 0:// 光标隐藏
					timer_init(timer, &task->fifo, 1);
					timer_settime(timer, 50);
					if (cursor_c >= 0) cursor_c = COL8_000000;
					// 刷新光标
					boxfill8(sheet->buf, sheet->bxsize, cursor_c, cursor_x, cursor_y, cursor_x + 7, cursor_y + 15);
					sheet_refresh(sheet, cursor_x, cursor_y, cursor_x + 8, cursor_y + 16);
					break;
			}
			if (256 <= i && i <= 511) {	// 通过任务A接收键盘数据
				switch (i) {
					case 8 + 256:// 退格键
						if (cursor_x > 16) {// 如果存在一个以上字符，可以退格
							putfonts8_str_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, " ");
							cursor_x -= 8;
						}
						break;
					case 10 + 256:// 回车键
						putfonts8_str_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, " ");
						cmdline[cursor_x / 8 - 2] = 0;
						cursor_y = cons_newline(cursor_y, sheet);
						// 执行命令
						if (strcmp(cmdline, "mem") == 0) {// mem 命令
							sprintf(s, "Total %dMB", memtotal / (1024 * 1024));
							putfonts8_str_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, s);
							cursor_y = cons_newline(cursor_y, sheet);
							sprintf(s, "Free %dKB", memman_total(memman) / 1024);
							putfonts8_str_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, s);
							// 不是命令，也不是空行
						} else if (strcmp(cmdline, "clear") == 0) {// clear 命令
							for (y = 28; y < 28 + 128; y++) {
								for (x = 8; x < 8 + 240; x++) {
									sheet->buf[x + y * sheet->bxsize] = COL8_000000;
								}
							}
							sheet_refresh(sheet, 8, 28, 8 + 240, 28 + 128);
							cursor_y = 12;
						} else if (strcmp(cmdline, "uname") == 0) {
							putfonts8_str_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, "Spark");
						} else if (strcmp(cmdline, "ls") == 0) {
							for (x = 0; x < 224; x++) {
								if (finfo[x].name[0] == 0x00) break;
								if (finfo[x].name[0] != 0xe5) {
									if ((finfo[x].type & 0x18) == 0) {
										sprintf(s, "filename.ext    %7d", finfo[x].size);
										for (y = 0; y < 8; y++) s[y] = finfo[x].name[y];
										s[9]  = finfo[x].ext[0];
										s[10] = finfo[x].ext[1];
										s[11] = finfo[x].ext[2];
										putfonts8_str_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, s);
										cursor_y = cons_newline(cursor_y, sheet);
									}
								}
							}
						} else if (cmdline[0] != 0) putfonts8_str_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, "Unkown Command");
						if (cursor_y < 28 + 112) cursor_y += 16;
						else {// 滚动
							for (y = 28; y < 28 + 112; y++) {// 将终端中的像素上移一行
								for (x = 8; x < 8 + 240; x++) {
									sheet->buf[x + y * sheet->bxsize] = sheet->buf[x + (y + 16) * sheet->bxsize];
								}
							}
							for (y = 28 + 112; y < 28 + 128; y++) {// 刷新最下面的一行，用黑色填充
								for (x = 8; x < 8 + 240; x++) {
									sheet->buf[x + y * sheet->bxsize] = COL8_000000;
								}
							}
							sheet_refresh(sheet, 8, 28, 8 + 240, 28 + 128);
						}
						putfonts8_str_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, ">");
						cursor_x = 16;
						break;
					default:
						if (cursor_x < 240) {// 如果字符未满一行，继续追加
							s[0] = i - 256;
							s[1] = 0;
							cmdline[cursor_x / 8 - 2] = i - 256;
							putfonts8_str_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, s);
							cursor_x += 8;
						}
				}
			}
		}
	}
}

int cons_newline(int cursor_y, struct SHEET *sheet)
{
	int x, y;
	if (cursor_y < 28 + 112) {
		cursor_y += 16;
	} else {
		for (y = 28; y < 28 + 112; y++) {
			for (x = 8; x < 8 + 240; x++) {
				sheet->buf[x + y * sheet->bxsize] = sheet->buf[x + (y + 16) * sheet->bxsize];
			}
		}
		for (y = 28 + 112; y < 28 + 128; y++) {
			for (x = 8; x < 8 + 240; x++) {
				sheet->buf[x + y * sheet->bxsize] = COL8_000000;
			}
		}
		sheet_refresh(sheet, 8, 28, 8 + 240, 28 + 128);
	}
	return cursor_y;
}
