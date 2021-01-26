#include "bootpack.h"
extern int key_ctrl, key_alt, key_esc, key_shift;
extern struct TASKCTL *taskctl;
/* sys info */
static char *kernel_release = "0.2.1";
static char *kernel_version = "2020 1-11 01:26";
static char *kernel_name    = "Spark";
static char *architecture   = "i486";
static char *operating_sys  = "CLAY/Spark";

void term_task(struct SHEET *sheet, unsigned int memtotal)
{
	struct TASK *task = task_now();
	char history[30] = {0};// 上一次命令
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	int i, *fat = (int *) memman_alloc_4k(memman, 4 * 2880);
	struct TERM term;
	char cmdline[30] = {0};
	term.sht   = sheet;
	term.cur_x = 8;
	term.cur_y = 28;
	term.cur_c = -1;
	task->term = &term;

	if (term.sht != 0) {
		term.timer = timer_alloc();
		timer_init(term.timer, &task->fifo, 1);
		timer_settime(term.timer, 50);
	}

	file_readfat(fat, (unsigned char *) (ADR_DISKIMG + 0x000200));
	// 显示提示符
	term_putchar(&term, '>', 1);

	for (;;) {
		io_cli();
		if (fifo32_status(&task->fifo) == 0) {
			task_sleep(task);
			io_sti();
		} else {
			io_sti();
			i = fifo32_get(&task->fifo);
			switch (i) {
				case 4:// x 关闭终端
					cmd_exit(&term, fat);
					break;
				case 3:// 关闭光标
					if (term.sht != 0) {
						boxfill8(term.sht->buf, term.sht->bxsize, COL8_000000, term.cur_x, term.cur_y, term.cur_x + 7, term.cur_y + 15);
						term.cur_c = -1;
					}
					break;
				case 2:// 开启光标
					term.cur_c = COL8_FFFFFF;
					break;
				case 1:// 光标显示
					timer_init(term.timer, &task->fifo, 0);
					if (term.sht != 0) {
						if (term.cur_c >= 0) {
							term.cur_c = COL8_FFFFFF;
							boxfill8(term.sht->buf, term.sht->bxsize, term.cur_c, term.cur_x, term.cur_y, term.cur_x + 7, term.cur_y + 15);
						}
						// 刷新光标
						timer_settime(term.timer, 50);
						sheet_refresh(term.sht, term.cur_x, term.cur_y, term.cur_x + 8, term.cur_y + 16);
					
					}
					break;
				case 0:// 光标隐藏
					timer_init(term.timer, &task->fifo, 1);
					if (term.sht != 0) {
						if (term.cur_c >= 0) {
							term.cur_c = COL8_000000;
							boxfill8(term.sht->buf, term.sht->bxsize, term.cur_c, term.cur_x, term.cur_y, term.cur_x + 7, term.cur_y + 15);
						}
					}
					// 刷新光标
					timer_settime(term.timer, 50);
					sheet_refresh(term.sht, term.cur_x, term.cur_y, term.cur_x + 8, term.cur_y + 16);
					break;
			}
			if (256 <= i && i <= 511) {	// 通过任务 A 接收键盘数据
				switch (i) {
					case 8 + 256:// 退格键
					// case 14 + 256:
						if (term.cur_x > 16) {// 如果存在一个以上字符，可以退格
							term_putchar(&term, ' ', 0);
							term.cur_x -= 8;
						}
						break;
					case 0x1c + 256:// 回车键
						strcpy0(history, cmdline);
						// 将光标用空格擦除后换行
						term_putchar(&term, ' ', 0);
						cmdline[term.cur_x / 8 - 2] = 0;
						term_newline(&term);
						term_runcmd(cmdline, &term, fat, memtotal); // 运行命令
						if (sheet == 0) cmd_exit(&term, fat);
						term_putchar(&term, '>', 1); // 显示提示符
						break;
					case 'C' + 256:// 关闭终端
						if (key_shift != 0 && key_alt != 0) {// shift + alt + c
							cmd_exit(&term, fat);
						}
						break;
					case 256 + 0x48:// 箭头上
						if (term.cur_x == 16) {// 如果没有字符
							strcpy(cmdline, history);
							term_putstr(&term, cmdline);
						}
						break;
					case 'l' + 256:
						if (key_ctrl == 1 && key_alt == 0) {
							cmd_clear(&term);
							putfonts8_str_sht(term.sht, 8, 28, COL8_FFFFFF, COL8_000000, ">");
						}
					default:
						if (term.cur_x < 240 && key_ctrl != 1 && key_alt != 1) {// 如果字符未满一行，继续追加
							cmdline[term.cur_x / 8 - 2] = i - 256;
							term_putchar(&term, i - 256, 1);
						}
				}
			}
		}
	}
}

// 换行
void term_newline(struct TERM *term)
{
	int x, y;
	struct SHEET *sht = term->sht;
	if (term->cur_y < 28 + 112) {
		term->cur_y += 16;// 到下一行
	} else {// 滚动
		if (sht != 0) {
			for (y = 28; y < 28 + 112; y++) {
				for (x = 8; x < 8 + 240; x++) {
					sht->buf[x + y * sht->bxsize] = sht->buf[x + (y + 16) * sht->bxsize];
				}
			}
			for (y = 28 + 112; y < 28 + 128; y++) {
				for (x = 8; x < 8 + 240; x++) {
					sht->buf[x + y * sht->bxsize] = COL8_000000;
				}
			}
			sheet_refresh(sht, 8, 28, 8 + 240, 28 + 128);
		}
	}
	term->cur_x = 8;
}

// 输出字符
void term_putchar(struct TERM *term, int c, char mv)
{
	char s[2] = {c, 0};
	switch (s[0]) {
		case 0x09:// 制表符
			for (;;) {
				if (term->sht != 0) {
					putfonts8_str_sht(term->sht, term->cur_x, term->cur_y, term->cur_c, COL8_000000, " ");
				}
				term->cur_x += 8;
				if (term->cur_x == 8 + 240) term_newline(term);
				if ((term->cur_x - 8) & 0x1f) break;
			}
			break;
		case 0x0a:// 换行
			term_newline(term);
			break;
		case 0x0d:// 回车，先不做任何操作
			break;
		default:// 一般字符
			if (term->sht != 0) {
				putfonts8_str_sht(term->sht, term->cur_x, term->cur_y, COL8_FFFFFF, COL8_000000, s);
			}
			if (mv != 0) {// mv 为 0 时光标不后移
				term->cur_x += 8;
				if (term->cur_x == 8 + 240) term_newline(term);
			}
	}
}

// 输出字符串 0
void term_putstr(struct TERM *term, char *s)
{
	int i, len = strlen(s);
	for (i = 0; i < len; i++) term_putchar(term, s[i], 1);
}

// 输出字符串 1
void term_putnstr(struct TERM *term, char *s, int l)
{
	int i;
	for (i = 0; i < l; i++) term_putchar(term, s[i], 1);
}

// 运行命令
void term_runcmd(char *cmdline, struct TERM *term, int *fat, unsigned int memtotal)
{
	if (strcmp(cmdline, "mem") == 0) {
		cmd_mem(term, memtotal);
	} else if (strcmp(cmdline, "ls") == 0) {
		cmd_ls(term);
	} else if (strcmp(cmdline, "clear") == 0) {
		cmd_clear(term);
	} else if (strcmp(cmdline, "uname") == 0) {
		cmd_uname(term, cmdline);
	} else if (strcmp(cmdline, "exit") == 0) {
		cmd_exit(term, fat);
	} else if (strncmp(cmdline, "cat", 3) == 0) {
		cmd_cat(term, fat, cmdline);
	} else if (strncmp(cmdline, "echo ", 5) == 0) {
		cmd_echo(term, cmdline + 5);
	} else if (strncmp(cmdline, "start ", 6) == 0) {
		cmd_start(term, cmdline, memtotal);
	} else if (strncmp(cmdline, "exec ", 5) == 0) {
		cmd_exec(term, cmdline, memtotal);
	} else if (strncmp(cmdline, "uname ", 6) == 0) {
		cmd_uname(term, cmdline);
	} else if (cmdline[0] != 0) {
		if (cmd_app(term, fat, cmdline) == 0) {// 不是命令，也不是空行
			char ret[80] = {0};
			sprintf(ret, "Unknown Command: %s", cmdline);
			term_putstr(term, ret);
			term_newline(term);
		}
	}
}

void cmd_exit(struct TERM *term, int *fat)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct TASK *task = task_now();
	struct SHTCTL *shtctl = (struct SHTCTL *) *((int *) 0x0fe4);
	struct FIFO32 *fifo = (struct FIFO32 *) *((int *) 0x0fec);
	timer_cancel(term->timer);
	memman_free_4k(memman, (int) fat, 4 * 2880);
	io_cli();
	if (term->sht != 0) {
		fifo32_put(fifo, term->sht - shtctl->sheets0 + 768);
	} else {
		fifo32_put(fifo, task - taskctl->tasks0 + 1024);
	}
	io_sti();
	for (;;) {
		task_sleep(task);
	}
}

// mem 命令
void cmd_mem(struct TERM *term, unsigned int memtotal)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	char s[30] = {0};
	sprintf(s, "Total %dMB\nFree %dKB\n", memtotal / (1024 * 1024), memman_total(memman) / 1024);
	term_putstr(term, s);
}

// clear 命令
void cmd_clear(struct TERM *term)
{
	int x, y;
	struct SHEET *sht = term->sht;
	for (y = 28; y < 28 + 128; y++) {
		for (x = 8; x < 8 + 240; x++) {
			sht->buf[x + y * sht->bxsize] = COL8_000000;
		}
	}
	sheet_refresh(sht, 8, 28, 8 + 240, 28 + 128);
	term->cur_y = 28;
}

// ls 命令
void cmd_ls(struct TERM *term)
{
	struct FILEINFO *finfo = (struct FILEINFO *) (ADR_DISKIMG + 0x002600);
	int x, y;
	char s[30] = {0};
	for (x = 0; x < 224; x++) {
		if (finfo[x].name[0] == 0x00) break;
		if (finfo[x].name[0] != 0xe5) {
			if ((finfo[x].type & 0x18) == 0) {
				sprintf(s, "filename.ext    %7d\n", finfo[x].size);
				for (y = 0; y < 8; y++) s[y] = finfo[x].name[y];
				s[9]  = finfo[x].ext[0];
				s[10] = finfo[x].ext[1];
				s[11] = finfo[x].ext[2];
				term_putstr(term, s);
			}
		}
	}
}

// cat 命令
void cmd_cat(struct TERM *term, int *fat, char *cmdline)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct FILEINFO *finfo = file_search(cmdline + 4, (struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);
	char *p;
	int i;
	if (finfo != 0) {// 找到文件
		p = (char *) memman_alloc_4k(memman, finfo->size);
		file_loadfile(finfo->clustno, finfo->size, p, fat, (char *) (ADR_DISKIMG + 0x003e00));
		for (i = 0; i < finfo->size; i++) term_putchar(term, p[i], 1);
		memman_free_4k(memman, (int) p, finfo->size);
		term_newline(term);
	} else {// 没有找到文件
		char *s = 0;
		sprintf(s, "No such file or directory: %s\n", cmdline + 4);
		term_putstr(term, s);
	}
}

// echo 命令
void cmd_echo(struct TERM *term, char *cmdline)
{
	char s[30] = {0};
	sprintf(s, "%s", cmdline);
	term_putstr(term, s);
	term_newline(term);
}

// uname 命令
// TODO: 做成应用而不是命令
void cmd_uname(struct TERM *term, char *cmdline)
{
	char s[30] = {0};
	if (cmdline[6] == '-') {
		switch (cmdline[7]) {
			case 'h':
				term_putstr(term, "Usage: uname [OPTION]...\n");
				term_putstr(term, "-a, print all information\n");
				term_putstr(term, "-r, print kernel release\n");
				term_putstr(term, "-v, print kernel version\n");
				term_putstr(term, "-m, print system architecture\n");
				term_putstr(term, "-o, print operating system\n");
				term_putstr(term, "-h, display this help and exit");
				break;
			case 'a':
				sprintf(s, "%s %s %s %s", kernel_name, kernel_release, architecture, operating_sys);
				term_putstr(term, s);
				term_newline(term);
				break;
			case 'r':
				term_putstr(term, kernel_release);
				term_newline(term);
				break;
			case 'v':
				term_putstr(term, kernel_version);
				term_newline(term);
				break;
			case 'm':
				term_putstr(term, architecture);
				term_newline(term);
				break;
			case 'o':
				term_putstr(term, operating_sys);
				term_newline(term);
				break;
			default:
				term_putstr(term, "uname: extra operand\n");
				term_putstr(term, "Try 'uname -h' for more information\n");
		}
	} else if (strcmp(cmdline, "uname") == 0) {
		term_putstr(term, kernel_name);
		term_newline(term);
	}
}

void cmd_start(struct TERM *term, char *cmdline, int memtotal)
{
	struct SHTCTL *shtctl = (struct SHTCTL *) *((int *) 0x0fe4);
	struct SHEET *sht = open_terminal(shtctl, memtotal);
	struct FIFO32 *fifo = &sht->task->fifo;
	int i;
	sheet_slide(sht, 32, 4);
	sheet_updown(sht, shtctl->top);
	// 将命令输入的字符串复制到新的命令行中
	for (i = 6; cmdline[i] != 0; i++) {
		fifo32_put(fifo, cmdline[i] + 256);
	}
	fifo32_put(fifo, 0x1c + 256);
}

void cmd_exec(struct TERM *term, char *cmdline, int memtotal)
{
	struct TASK *task = open_termtask(0, memtotal);
	struct FIFO32 *fifo = &task->fifo;
	int i;
	// 将命令行输入的字符串复制到新的命令行窗口中
	for (i = 5; cmdline[i] != 0; ++i) {
		fifo32_put(fifo, cmdline[i] + 256);
	}
	fifo32_put(fifo, 0x1c + 256);
}

// 启动应用
int cmd_app(struct TERM *term, int *fat, char *cmdline)
{
	struct SHTCTL *shtctl;
	struct SHEET *sht;
	int segsiz, datsiz, esp, dathrb;
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct FILEINFO *finfo;
	char name[18], *p, *q;
	struct TASK *task = task_now();
	int i;
	for (i = 0; i < 13; i++) {// 根据命令生成文件名
		if (cmdline[i] <= ' ') break;
		name[i] = cmdline[i];
	}
	name[i] = 0;	// 将文件名后置为 0
	finfo = file_search(name, (struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);
	if (finfo == 0 && name[i - 1] != '.') {// 找不到文件通过加上文件后缀 '.hrb' 查找
		name[i]     = '.';
		name[i + 1] = 'H';
		name[i + 2] = 'R';
		name[i + 3] = 'B';
		name[i + 4] = 0;
		finfo = file_search(name, (struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);
	}

	if (finfo != 0) {
		// 找到文件
		p = (char *) memman_alloc_4k(memman, finfo->size);
		file_loadfile(finfo->clustno, finfo->size, p, fat, (char *) (ADR_DISKIMG + 0x003e00));
		if (finfo->size >= 36 && strncmp(p + 4, "Hari", 4) == 0 && *p == 0x00) {
			segsiz = *((int *) (p + 0x0000));
			esp    = *((int *) (p + 0x000c));
			datsiz = *((int *) (p + 0x0010));
			dathrb = *((int *) (p + 0x0014));
			q = (char *) memman_alloc_4k(memman, segsiz);
			task->ds_base = (int) q;
			set_segmdesc(task->ldt + 0, finfo->size - 1, (int) p, AR_CODE32_ER + 0x60);
			set_segmdesc(task->ldt + 1, segsiz - 1,      (int) q, AR_DATA32_RW + 0x60);
			for (i = 0; i < datsiz; i++) {
				q[esp + i] = p[dathrb + i];
			}
			start_app(0x1b, 0 * 8 + 4, esp, 1 * 8 + 4, &(task->tss.esp0));
			shtctl = (struct SHTCTL *) *((int *) 0x0fe4);
			for (i = 0; i < MAX_SHEETS; i++) {
				sht = &(shtctl->sheets0[i]);
				// 如果是应用程序遗留的窗口
				if ((sht->flags & 0x11) == 0x11 && sht->task == task) sheet_free(sht);
			}
			timer_cancelall(&task->fifo);
			memman_free_4k(memman, (int) q, segsiz);
		}
		else term_putstr(term, ".hrb file format error.");
		memman_free_4k(memman, (int) p, finfo->size);
		term_newline(term);
		return 1;
	}
	return 0;
}

// 应用程序 API
int *hrb_api(int edi, int esi, int ebp, int esp, int ebx, int edx, int ecx, int eax)
{
	struct TASK *task     = task_now();
	int ds_base = task->ds_base;
	struct TERM *term     = task->term;
	struct SHTCTL *shtctl = (struct SHTCTL *) *((int *) 0x0fe4);
	struct FIFO32 *sys_info = (struct FIFO32 *) *((int *) 0x0fec);
	struct SHEET *sht;
	int *reg = &eax + 1, i;	// eax 后面的地址
	/* 强行改写通过 PUSHAD 保存的值
	 * reg[0] : EDI,	reg[1] : ESI,	reg[2] : EBP,	reg[3] : ESP
	 * reg[4] : EBX,	reg[5] : EDX,	reg[6] : ECX,	reg[7] : EAX
	 * */
	switch (edx) {
		case 1:
			term_putchar(term, eax & 0xff, 1);
			break;
		case 2:
			term_putstr(term, (char *) ebx + ds_base);
			break;
		case 3:
			term_putnstr(term, (char *) ebx + ds_base, ecx);
			break;
		case 4:
			return &(task->tss.esp0);
		case 5:
			sht = sheet_alloc(shtctl);
			sht->task = task;
			sht->flags |= 0x10;
			sheet_setbuf(sht, (unsigned char *) ebx + ds_base, esi, edi, eax);
			make_window8((unsigned char *) ebx + ds_base, esi, edi, (char *) ecx + ds_base, 0);
			// 窗口出现在屏幕中间
			sheet_slide(sht, ((shtctl->xsize - esi) / 2) & ~3, (shtctl->ysize - edi) / 2);
			sheet_updown(sht, shtctl->top);	// 新出现的窗口位于最顶层
			reg[7] = (int) sht;
			break;
		case 6:
			sht = (struct SHEET *) (ebx & 0xfffffffe);
			putfonts8_str(sht->buf, sht->bxsize, esi, edi, eax, (char *) ebp + ds_base);
			if ((ebx & 1) == 0) sheet_refresh(sht, esi, edi, esi + ecx * 8, edi + 16);
			break;
		case 7:
			sht = (struct SHEET *) (ebx & 0xfffffffe);
			boxfill8(sht->buf, sht->bxsize, ebp, eax, ecx, esi, edi);
			if ((ebx & 1) == 0) sheet_refresh(sht, eax, ecx, esi + 1, edi + 1);
			break;
		case 8:
			memman_init((struct MEMMAN *) (ebx + ds_base));
			ecx &= 0xfffffff0;	// 以 16 字节为单位
			memman_free((struct MEMMAN *) (ebx + ds_base), eax, ecx);
			break;
		case 9:
			ecx = (ecx + 0x0f) & 0xfffffff0;	// 以 16 字节为单位进位取整
			reg[7] = memman_alloc((struct MEMMAN *) (ebx + ds_base), ecx);
			break;
		case 10:
			ecx = (ecx + 0x0f) & 0xfffffff0;	// 以 16 字节为单位进位取整
			memman_free((struct MEMMAN *) (ebx + ds_base), eax, ecx);
			break;
		case 11:
			sht = (struct SHEET *) (ebx & 0xfffffffe);
			sht->buf[sht->bxsize * edi + esi] = eax;
			if ((ebx & 1) == 0) sheet_refresh(sht, esi, edi, esi + 1, edi + 1);
			break;
		case 12:
			sht = (struct SHEET *) ebx;
			sheet_refresh(sht, eax, ecx, esi, edi);
			break;
		case 13:
			sht = (struct SHEET *) (ebx & 0xfffffffe);
			hrb_api_linewin(sht, eax, ecx, esi, edi, ebp);
			if ((ebx & 1) == 0) sheet_refresh(sht, esi, edi, esi + 1, edi + 1);
			break;
		case 14:
			sheet_free((struct SHEET *) ebx);
			break;
		case 15:
			for (;;) {
				io_cli();
				if (fifo32_status(&task->fifo) == 0) {
					if (eax != 0) task_sleep(task);	// FIFO 为空，休眠并等待
					else {
						io_sti();
						reg[7] = -1;
						return 0;
					}
				}
				i = fifo32_get(&task->fifo);
				io_sti();
				switch (i) {
					case 0:
					case 1:
						timer_init(term->timer, &task->fifo, 1);
						timer_settime(term->timer, 50);
						break;
					case 2:// 光标显示
						term->cur_c = COL8_FFFFFF;
						break;
					case 3:// 光标隐藏
						term->cur_c = -1;
						break;
					case 4:// 只关闭终端窗口
						timer_cancel(term->timer);
						io_cli();
						fifo32_put(sys_info, term->sht - shtctl->sheets0 + 2024);	// 2024 ~ 2279
						io_sti();
						break;
					default:
						if (i >= 256) {// 如果是键盘输入
							reg[7] = i - 256;
							// if (key_ctrl != 0) reg[7] ^= 0x10000000;
							return 0;
						}
				}
			}
			break;
			case 16:
				reg[7] = (int) timer_alloc();
				((struct TIMER *) reg[7])->flags2 = 1;	// 允许自动取消
				break;
			case 17:
				timer_init((struct TIMER *) ebx, &task->fifo, eax + 256);
				break;
			case 18:
				timer_settime((struct TIMER *) ebx, eax);
				break;
			case 19:
				timer_free((struct TIMER *) ebx);
				break;
			case 20:
				if (eax == 0) {
					i = io_in8(0x61);
					io_out8(0x61, i & 0x0d);
				} else {
					i = 1193180000 / eax;
					io_out8(0x43, 0xb6);

					io_out8(0x42, i & 0xff);
					io_out8(0x42, i >> 8);
					i = io_in8(0x61);
					io_out8(0x61, (i | 0x03) & 0x0f);
				}
				break;
	}
	return 0;
}

void hrb_api_linewin(struct SHEET *sht, int x0, int y0, int x1, int y1, int col)
{
	int i, x, y, len, dx, dy;

	dx = x1 - x0;
	dy = y1 - y0;
	x = x0 << 10;
	y = y0 << 10;
	if (dx < 0) dx = - dx;
	if (dy < 0) dy = - dy;
	if (dx >= dy) {
		len = dx + 1;
		if (x0 > x1) dx = -1024;
		else dx = 1024;

		if (y0 <= y1) dy = ((y1 - y0 + 1) << 10) / len;
		else dy = ((y1 - y0 - 1) << 10) / len;
	} else {
		len = dy + 1;
		if (y0 > y1) dy = -1024;
		else dy = 1024;
		if (x0 <= x1) dx = ((x1 - x0 + 1) << 10) / len;
		else dx = ((x1 - x0 - 1) << 10) / len;
	}

	for (i = 0; i < len; i++) {
		sht->buf[(y >> 10) * sht->bxsize + (x >> 10)] = col;
		x += dx;
		y += dy;
	}

	return;
}

// 抛出系统保护异常
int *inthandler0d(int *esp)
{
	struct TASK *task = task_now();
	struct TERM *term = task->term;
	char s[30];
	term_putstr(term, "\nINT 0x0d:\n General Protected Exception.");
	sprintf(s, "\nEIP = %08X", esp[11]);
	term_putstr(term, s);
	return &(task->tss.esp0);// 强制结束程序
}

// 抛出栈异常
int *inthandler0c(int *esp)
{
	struct TASK *task = task_now();
	struct TERM *term = task->term;
	char s[30];
	term_putstr(term, "\nINT 0x0c:\n Stack Exception.");
	sprintf(s, "\nEIP = %08X", esp[11]);
	term_putstr(term, s);
	return &(task->tss.esp0);// 强制结束程序
}

/* Copy s to dest
 * write 0 at the end of dest
 * return s length
 * */
int strcpy0(char *dest, char *s)
{
	char *p = s;
	char *q = dest;
	int len = 0;
	for(;*p != 0;len++) {
		*q = *p;
		q++;
		p++;
	}
	*q = 0;
	return len;
}
