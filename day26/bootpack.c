#include "bootpack.h"
#define KEYCMD_LED 0xed

void keywin_off(struct SHEET *key_win);
void keywin_on(struct SHEET *key_win);
struct SHEET *open_terminal(struct SHTCTL *shtctl, unsigned int memtotal);
void close_termtask(struct TASK *task);
void close_term(struct SHEET *sht);

int key_to = 0, key_shift = 0, key_ctrl = 0, key_alt = 0, keycmd_wait = -1, keycmd_time = 0, key_esc = 0;

void HariMain(void)
{
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;	// 结构体指针指向储存系统信息的地址
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct FIFO32 fifo, keycmd;
	*((int *) 0x0fec) = (int) &fifo;
	int fifobuf[128], keycmd_buf[32];
	int j, x, y, mmx = -1, mmy = -1, mmx2 = 0;
	struct SHEET *sht = 0, *key_win;
	struct SHTCTL *shtctl;
	// 图层背景，鼠标
	struct SHEET *sht_back, *sht_mouse;
	// 定义背景缓冲区、鼠标缓冲区
	unsigned char *buf_back, buf_mouse[256];
	struct TASK *task_a, *task;
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
	int i, mx, my, new_mx = -1, new_my = 0, new_wx = 0x7fffffff, new_wy = 0;

	init_gdtidt();							// 初始化 全局段记录表，中断记录表
	init_pic();								// 初始化 PIC
	io_sti();								// IDT/PIC 的初始化已经完成，开放 CPU 的中断
	init_pit();								// 设定定时器频率
	io_out8(PIC0_IMR, 0xf8);				// 开放 PIC1 和键盘中断 (11111001)
	io_out8(PIC1_IMR, 0xef);				// 开放鼠标中断 (11101111)
	fifo32_init(&fifo, 128, fifobuf, 0);
	fifo32_init(&keycmd, 32, keycmd_buf, 0);
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
	key_win = open_terminal(shtctl, memtotal);

	// sht_mouse
	sht_mouse = sheet_alloc(shtctl);
	sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99);	// 透明色号 99
	init_mouse_cursor8(buf_mouse, 99);	// 背景色号 99
	mx = (binfo->scrnx - 16) / 2;
	my = (binfo->scrny - 28 - 16) / 2;

	// 设置在移动图层时进行局部画面刷新
	sheet_slide(sht_back,  0,   0);
	sheet_slide(key_win, 32, 4);
	sheet_slide(sht_mouse, mx, my);
	// 设置叠加显示优先级
	sheet_updown(sht_back, 0);
	sheet_updown(key_win, 1);
	sheet_updown(sht_mouse, 2);
	keywin_on(key_win);

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
			// FIFO 为空，当存在搁置的绘图操作时会立即执行
			if (new_mx >= 0) {
				io_sti();
				sheet_slide(sht_mouse, new_mx, new_my);
				new_mx = -1;
			} else if (new_wx != 0x7fffffff) {
				io_sti();
				sheet_slide(sht, new_wx, new_wy);
				new_wx = 0x7fffffff;
			} else {
				task_sleep(task_a);
				io_sti();
			}
		} else {
			i = fifo32_get(&fifo);
			io_sti();
			if (key_win != 0 && key_win->flags == 0) {// 输入窗口被关闭
				if (shtctl->top == 1) {
					key_win = 0;
				} else {
					key_win = shtctl->sheets[shtctl->top - 1];
					keywin_on(key_win);
				}
			}
			// 处理键盘数据
			if (256 <= i && i <= 511) {
				if (i < 0x80 + 256) {// 将按键编码转化为字符编码
					if (key_shift == 0) {
						s[0] = keytable0[i - 256];
					} else {
						s[0] = keytable1[i - 256];
					}
				} else {
					s[0] = 0;
				}
				if ('A' <= s[0] && s[0] <= 'Z') {// 当输入的字符为英文字符时
					// 当 CapsLock 和 Shift 没有打开，或者都打开时
					if (((key_leds & 4) == 0 && key_shift == 0) || ((key_leds & 4) != 0 && key_shift != 0)) {
						s[0] += 0x20;	// 将大写字母转化为小写字母
					}
				}
				if (s[0] != 0 && key_win != 0) {// 一般字符
					fifo32_put(&key_win->task->fifo, s[0] + 256);
				}
				switch (i) {
					case 256 + 0x1c:// 回车键
						if (key_alt != 0 && key_shift != 0) {// shift + alt + enter
							keywin_off(key_win);
							key_win = open_terminal(shtctl, memtotal);
							sheet_slide(key_win, 32, 4);
							sheet_updown(key_win, shtctl->top);
							// 自动将焦点切换到新打开的命令行窗口
							keywin_on(key_win);
						} else {
							fifo32_put(&key_win->task->fifo, 10 + 256);	// 发送命令到终端
						}
						break;
					case 256 + 0x0e:// 退格键
						fifo32_put(&key_win->task->fifo, 8 + 256);
						break;
					case 256 + 0x0f:// TAB 键处理
						if (key_alt != 0 && key_win != 0) {// alt + tab
							keywin_off(key_win);
							j = key_win->height - 1;
							if (j == 0) j = shtctl->top - 1;
							key_win = shtctl->sheets[j];
							keywin_on(key_win);
							sheet_updown(shtctl->sheets[1], shtctl->top - 1);
						} 
						break;
					case 256 + 0xae:
						if (key_ctrl == 1 && key_win != 0) {// ctrl + c
							task = key_win->task;
							if (task != 0 && task->tss.ss0 != 0) {
								term_putstr(task->term, "\nBreak(key):\nCtrl + c");
								io_cli();	// 不能在改变寄存器值时切换到其他任务
								task->tss.eax = (int) &(task->tss.esp0);
								task->tss.eip = (int) asm_end_app;
								io_sti();
							}
						}
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
					new_mx = mx;
					new_my = my;
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
										if (sht != key_win) {
											keywin_off(key_win);
											key_win = sht;
											keywin_on(key_win);
										}
										// 点击的区域是标题栏的区域
										if (3 <= x && x < sht->bxsize - 3 && 3 <= y && y < 21) {
											// 进入窗口移动模式
											mmx = mx;
											mmy = my;
											mmx2 = sht->vx0;
											new_wy = sht->vy0;
										}
										if (sht->bxsize - 21 <= x && x < sht->bxsize - 5 && 5 <= y && y < 19) {
											// 点击 x 按钮
											if ((sht->flags & 0x10) != 0) {
												// 该窗口是否为应用程序
												task = sht->task;
												term_putstr(task->term, "\nBreak(Mouse)");
												io_cli();// 强制结束处理中，禁止切换任务
												task->tss.eax = (int) &(task->tss.esp0);
												task->tss.eip = (int) asm_end_app;
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
							new_wx = (mmx2 + x + 2) & ~3;
							new_wy = new_wy + y;
							// 更新为移动后的坐标
							mmy = my;
						}
					} else {
						// 没有按下左键
						// 返回普通模式
						mmx = -1;
						if (new_wx != 0x7fffffff) {
							sheet_slide(sht, new_wx, new_wy);	// 固定图层位置
							new_wx = 0x7fffffff;
						}
					}
				}
			} else if (768 <= i && i <= 1023) {// 终端窗口关闭处理
				close_term(shtctl->sheets0 + (i - 768));
			}
		}
	}
}

void keywin_off(struct SHEET *key_win)
{
	change_wtitle8(key_win, 0);
	if ((key_win->flags & 0x20) != 0) fifo32_put(&key_win->task->fifo, 3);
}

void keywin_on(struct SHEET *key_win)
{
	change_wtitle8(key_win, 1);
	if ((key_win->flags & 0x20) != 0) fifo32_put(&key_win->task->fifo, 2);
}

struct SHEET *open_terminal(struct SHTCTL *shtctl, unsigned int memtotal)
{
		struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
		struct SHEET *sht = sheet_alloc(shtctl);
		unsigned char *buf = (unsigned char *) memman_alloc_4k(memman, 256 * 165);
		struct TASK *task = task_alloc();
		int *term_fifo = (int *) memman_alloc_4k(memman, 128 * 4);
		sheet_setbuf(sht, buf, 256, 165, -1);
		make_window8(buf, 256, 165, "terminal", 0);
		make_textbox8(sht, 8, 28, 240, 128, COL8_000000);
		task->term_stack = memman_alloc_4k(memman, 64 * 1024);
		task->tss.esp = task->term_stack + 64 * 1024 - 12;
		task->tss.eip = (int) &term_task;
		task->tss.es = 1 * 8;
		task->tss.cs = 2 * 8;
		task->tss.ss = 1 * 8;
		task->tss.ds = 1 * 8;
		task->tss.fs = 1 * 8;
		task->tss.gs = 1 * 8;
		*((int *) (task->tss.esp + 4)) = (int) sht;
		*((int *) (task->tss.esp + 8)) = memtotal;
		task_run(task, 2, 2);	// 第二等级，0.02 秒
		sht->task   = task;
		sht->flags |= 0x20;	// 有光标
		fifo32_init(&task->fifo, 128, term_fifo, task);
		return sht;
}

void close_termtask(struct TASK *task)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	task_sleep(task);
	memman_free_4k(memman, task->term_stack, 64 * 1024);
	memman_free_4k(memman, (int) task->fifo.buf, 128 * 4);
	task->flags = 0;// 用来替代 task_free(task);
}

void close_term(struct SHEET *sht)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct TASK *task = sht->task;
	memman_free_4k(memman, (int) sht->buf, 256 * 165);
	sheet_free(sht);
	close_termtask(task);
}
