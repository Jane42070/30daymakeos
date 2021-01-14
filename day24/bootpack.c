#include "bootpack.h"
#define KEYCMD_LED 0xed
int key_to = 0, key_shift = 0, key_ctrl = 0, key_alt = 0, keycmd_wait = -1, keycmd_time = 0, key_esc = 0;

void HariMain(void)
{
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;	// 结构体指针指向储存系统信息的地址
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct FIFO32 fifo, keycmd;
	struct TERM *term;
	int fifobuf[128], keycmd_buf[32];
	int j, x, y, mmx = -1, mmy = -1;
	struct SHEET *sht = 0;
	struct SHTCTL *shtctl;
	// 图层背景，鼠标
	struct SHEET *sht_back, *sht_mouse, *sht_win, *sht_term;
	// 定义背景缓冲区、鼠标缓冲区
	unsigned char *buf_back, buf_mouse[256], *buf_win, *buf_term;
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
	int key_leds = (binfo->leds >> 4) & 7;

	unsigned int memtotal;
	char s[40];
	struct TIMER *timer;
	int i, mx, my, cursor_x, cursor_c;
	struct TASK *task_a, *task_term;

	init_gdtidt();							// 初始化 全局段记录表，中断记录表
	init_pic();								// 初始化 PIC
	io_sti();								// IDT/PIC 的初始化已经完成，开放 CPU 的中断
	init_pit();								// 设定定时器频率
	io_out8(PIC0_IMR, 0xf8);				// 开放 PIC1 和键盘中断 (11111001)
	io_out8(PIC1_IMR, 0xef);				// 开放鼠标中断 (11101111)
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
	*((int *) 0x0fe4) = (int) shtctl;

	// sht_back
	sht_back = sheet_alloc(shtctl);
	buf_back = (unsigned char *) memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);
	sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1);	// 没有透明色
	init_screen(buf_back, binfo->scrnx, binfo->scrny);

	// sht_term
	sht_term = sheet_alloc(shtctl);
	buf_term = (unsigned char *) memman_alloc_4k(memman, 256 * 165);
	sheet_setbuf(sht_term, buf_term, 256, 165, -1);
	make_window8(buf_term, 256, 165, "terminal", 0);
	make_textbox8(sht_term, 8, 28, 240, 128, COL8_000000);
	task_term = task_alloc();
	task_term->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024 - 12;
	task_term->tss.eip = (int) &term_task;
	task_term->tss.es = 1 * 8;
	task_term->tss.cs = 2 * 8;
	task_term->tss.ss = 1 * 8;
	task_term->tss.ds = 1 * 8;
	task_term->tss.fs = 1 * 8;
	task_term->tss.gs = 1 * 8;
	*((int *) (task_term->tss.esp + 4)) = (int) sht_term;
	*((int *) (task_term->tss.esp + 8)) = memtotal;
	task_run(task_term, 2, 2);	// 第二等级，0.02 秒

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
	sheet_slide(sht_term,  32,  4);
	sheet_slide(sht_win,   8,  56);
	sheet_slide(sht_mouse, mx, my);
	// 设置叠加显示优先级
	sheet_updown(sht_back, 0);
	sheet_updown(sht_term, 1);
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
				if (s[0] != 0 && key_ctrl != 1 && key_alt != 1) {// 一般字符在没有按下组合键 ctrl，alt 的时候
					// 显示一个字符就后移一次光标
					if (key_to == 0) {// 发送给任务 A
						if (cursor_x < 128) {
							s[1] = 0;
							putfonts8_str_sht(sht_win, cursor_x, 28, COL8_000000, COL8_FFFFFF, s);
							cursor_x += 8;
						}
					}
					// 发送给任务终端
					else fifo32_put(&task_term->fifo, s[0] + 256);
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
						else fifo32_put(&task_term->fifo, 8 + 256);
						break;
					case 256 + 0x0f:// TAB 键处理
						if (key_alt != 0) {// alt + tab
							sheet_updown(shtctl->sheets[1], shtctl->top - 1);
							if (key_to == 0) {// 切换至终端
								key_to = 1;
								cursor_c = -1; // 不显示光标
								make_wtitle8(buf_win, sht_win->bxsize, "task_a", 0);
								make_wtitle8(buf_term, sht_term->bxsize, "terminal", 1);
								boxfill8(sht_win->buf, sht_win->bxsize, COL8_FFFFFF, cursor_x, 28, cursor_x + 7, 43);
								fifo32_put(&task_term->fifo, 2);	// 命令行窗口光标 ON
							} else {// 切换至任务 A
								key_to = 0;
								cursor_c = COL8_000000;	// 显示光标
								make_wtitle8(buf_win, sht_win->bxsize, "task_a", 1);
								make_wtitle8(buf_term, sht_term->bxsize, "terminal", 0);
								fifo32_put(&task_term->fifo, 3);	// 命令行窗口光标 OFF
							}
						}
						sheet_refresh(sht_win, 0, 0, sht_win->bxsize, 21);
						sheet_refresh(sht_term, 0, 0, sht_term->bxsize, 21);
						if (cursor_c >= 0) boxfill8(sht_win->buf, sht_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
						sheet_refresh(sht_win, cursor_x, 28, cursor_x + 8, 44);
						break;
					case 256 + 0xae:
						if (key_ctrl != 0 && task_term->tss.ss0 != 0) {// ctrl + c
							term = (struct TERM *) *((int *) 0x0fec);
							term_putstr(term, "\nBreak(key):\nCtrl + c");
							io_cli();	// 不能在改变寄存器值时切换到其他任务
							task_term->tss.eax = (int) &(task_term->tss.esp0);
							task_term->tss.eip = (int) asm_end_app;
							io_sti();
						}
						break;
					case 256 + 0x1c:// 回车键
						if (key_to != 0) fifo32_put(&task_term->fifo, 10 + 256);	// 发送命令到终端
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
					case 256 + 0x1d:// lCtrl ON
						key_ctrl |=1;
						break;
					case 256 + 0x9d:// lCtrl OFF
						key_ctrl &= ~1;
						break;
					case 256 + 0x38:// lAlt ON
						key_alt |= 1;
						break;
					case 256 + 0xb8:// lAlt OFF
						key_alt &= ~1;
						break;
					case 256 + 0x01:// ESC
						key_esc ^= 1;
						if (key_esc == 0) putfonts8_str_sht(sht_back, 500, 500, COL8_FFFFFF, COL8_008484, "esc");
						else putfonts8_str_sht(sht_back, 500, 500, COL8_FFFFFF, COL8_008484, "ESC");
						sheet_refresh(sht_back, 500, 516, 524, 516);
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
					// TODO: 实现功能键按下的功能
					// case 256 + 0xe0:// 功能键
					//     switch (fifo32_get(&fifo)) {
					//         case 256 + 0x48:// 箭头上
					//             break;
					//         case 256 + 0x4b:// 箭头左
					//             break;
					//         case 256 + 0x4d:// 箭头右
					//             break;
					//         case 256 + 0x50:// 箭头下
					//             break;
					//     }
					//     break;
					case 256 + 0xfa:// 键盘成功接收到数据
						keycmd_wait = -1;
						break;
					case 256 + 0xfe:// 键盘没有成功接收到数据
						if (keycmd_time == 5) break;
						wait_KBC_sendready();
						io_out8(PORT_KEYDAT, keycmd_wait);
						keycmd_time += 1;
						break;
				}
			} else if (512 <= i && i <= 767) {
				if (mouse_decode(&mdec, i - 512) != 0) {
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
					if ((mdec.btn & 0x01) != 0) {
						if (mmx < 0) {
							// 如果处于通常模式
							// 按照从上到下的顺序寻找鼠标所指向的图层
							for (j = shtctl->top - 1; j > 0; j--) {
								sht = shtctl->sheets[j];
								x = mx - sht->vx0;
								y = my - sht->vy0;
								if (0 <= x && x < sht->bxsize && 0 <= y && y < sht->bysize) {
									if (sht->buf[y * sht->bxsize + x] != sht->col_inv) {
										sheet_updown(sht, shtctl->top - 1);
										// 点击的区域是标题栏的区域
										if (3 <= x && x < sht->bxsize - 3 && 3 <= y && y < 21) {
											// 进入窗口移动模式
											mmx = mx;
											mmy = my;
										}
										if (sht->bxsize - 21 <= x && x < sht->bxsize - 5 && 5 <= y && y < 19) {
											// 点击 x 按钮
											if (sht->task != 0) {
												// 该窗口是否为应用程序
												term = (struct TERM *) *((int *) 0x0fec);
												term_putstr(term, "\nBreak(Mouse)");
												io_cli();// 强制结束处理中，禁止切换任务
												task_term->tss.eax = (int) &(task_term->tss.esp0);
												task_term->tss.eip = (int) asm_end_app;
												io_sti();
											}
										}
										break;
									}
								}
							}
						} else {
							// 如果处于窗口移动模式
							// 计算鼠标的移动距离
							x = mx - mmx;
							y = my - mmy;
							sheet_slide(sht, sht->vx0 + x, sht->vy0 + y);
							// 更新为移动后的坐标
							mmx = mx;
							mmy = my;
						}
					} else {
						// 没有按下左键
						// 返回普通模式
						mmx = -1;
					}
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
