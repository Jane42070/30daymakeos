#include "bootpack.h"
/* sys info */
static char *kernel_release = "0.2.0";
static char *kernel_version = "2020 1-10 22:26";
static char *kernel_name    = "Spark";
static char *architecture   = "i386";
static char *operating_sys  = "CLAY/Spark";

void term_task(struct SHEET *sheet, unsigned int memtotal)
{
	struct TIMER *timer;
	struct TASK *task = task_now();
	// char history[128] = {0};
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	int i, fifobuf[128], *fat = (int *) memman_alloc_4k(memman, 4 * 2880);
	struct TERM term;
	char cmdline[30] = {0};
	term.sht   = sheet;
	term.cur_x = 8;
	term.cur_y = 28;
	term.cur_c = -1;

	fifo32_init(&task->fifo, 128, fifobuf, task);
	timer = timer_alloc();
	timer_init(timer, &task->fifo, 1);
	timer_settime(timer, 50);
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
				case 3:// 关闭光标
					boxfill8(sheet->buf, sheet->bxsize, COL8_000000, term.cur_x, 28, term.cur_x + 7, 43);
					term.cur_c = -1;
					break;
				case 2:// 开启光标
					term.cur_c = COL8_FFFFFF;
					break;
				case 1:// 光标显示
					timer_init(timer, &task->fifo, 0);
					if (term.cur_c >= 0) term.cur_c = COL8_FFFFFF;
					// 刷新光标
					timer_settime(timer, 50);
					boxfill8(sheet->buf, sheet->bxsize, term.cur_c, term.cur_x, term.cur_y, term.cur_x + 7, term.cur_y + 15);
					sheet_refresh(sheet, term.cur_x, term.cur_y, term.cur_x + 8, term.cur_y + 16);
					break;
				case 0:// 光标隐藏
					timer_init(timer, &task->fifo, 1);
					if (term.cur_c >= 0) term.cur_c = COL8_000000;
					// 刷新光标
					timer_settime(timer, 50);
					boxfill8(sheet->buf, sheet->bxsize, term.cur_c, term.cur_x, term.cur_y, term.cur_x + 7, term.cur_y + 15);
					sheet_refresh(sheet, term.cur_x, term.cur_y, term.cur_x + 8, term.cur_y + 16);
					break;
			}
			if (256 <= i && i <= 511) {	// 通过任务A接收键盘数据
				switch (i) {
					case 8 + 256:// 退格键
						if (term.cur_x > 16) {// 如果存在一个以上字符，可以退格
							term_putchar(&term, ' ', 0);
							term.cur_x -= 8;
						}
						break;
					case 10 + 256:// 回车键
						// 将光标用空格擦除后换行
						term_putchar(&term, ' ', 0);
						cmdline[term.cur_x / 8 - 2] = 0;
						term_newline(&term);
						term_runcmd(cmdline, &term, fat, memtotal); // 运行命令
						term_putchar(&term, '>', 1); // 显示提示符
						break;
					default:
						if (term.cur_x < 240) {// 如果字符未满一行，继续追加
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
	term->cur_x = 8;
}

// 输出字符
void term_putchar(struct TERM *term, int c, char mv)
{
	char s[2] = {c, 0};
	switch (s[0]) {
		case 0x09:// 制表符
			for (;;) {
				putfonts8_str_sht(term->sht, term->cur_x, term->cur_y, term->cur_c, COL8_FFFFFF, " ");
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
			putfonts8_str_sht(term->sht, term->cur_x, term->cur_y, COL8_FFFFFF, COL8_000000, s);
			if (mv != 0) {// mv 为 0 时光标不后移
				term->cur_x += 8;
				if (term->cur_x == 8 + 240) term_newline(term);
			}
	}
}

// 输出字符串
// TODO: 不使用term_putchar(), 感觉太费性能？
void term_putstr(struct TERM *term, char *s)
{
	char *p = s;
	int i, len = strlen(s);
	for (i = 0; i < len; i++) term_putchar(term, p[i], 1);
}

// 运行命令
void term_runcmd(char *cmdline, struct TERM *term, int *fat, unsigned int memtotal)
{
	if (strcmp(cmdline, "mem") == 0) {
		cmd_mem(term, memtotal);
	} else if (strcmp(cmdline, "ls") == 0) {
		cmd_ls(term);
	} else if (strcmp(cmdline, "halt") == 0) {
		cmd_halt(term, fat);
	} else if (strcmp(cmdline, "clear") == 0) {
		cmd_clear(term);
	} else if (strcmp(cmdline, "uname") == 0) {
		cmd_uname(term, cmdline);
	} else if (strncmp(cmdline, "cat", 3) == 0) {
		cmd_cat(term, fat, cmdline);
	} else if (strncmp(cmdline, "uname ", 6) == 0) {
		cmd_uname(term, cmdline);
	} else if (cmdline[0] != 0) {// 不是命令，也不是空行
		char ret[80] = {0};
		sprintf(ret, "Unknown Command: %s", cmdline);
		term_putstr(term, ret);
		term_newline(term);
	}
}

// mem 命令
void cmd_mem(struct TERM *term, unsigned int memtotal)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	char s[30] = {0};
	sprintf(s, "Total %dMB", memtotal / (1024 * 1024));
	putfonts8_str_sht(term->sht, 8, term->cur_y, COL8_FFFFFF, COL8_000000, s);
	term_newline(term);
	sprintf(s, "Free %dKB", memman_total(memman) / 1024);
	putfonts8_str_sht(term->sht, 8, term->cur_y, COL8_FFFFFF, COL8_000000, s);
	term_newline(term);
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
				sprintf(s, "filename.ext    %7d", finfo[x].size);
				for (y = 0; y < 8; y++) s[y] = finfo[x].name[y];
				s[9]  = finfo[x].ext[0];
				s[10] = finfo[x].ext[1];
				s[11] = finfo[x].ext[2];
				putfonts8_str_sht(term->sht, 8, term->cur_y, COL8_FFFFFF, COL8_000000, s);
				term_newline(term);
			}
		}
	}
	term_newline(term);
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
	} else {// 没有找到文件
		putfonts8_str_sht(term->sht, 8, term->cur_y, COL8_FFFFFF, COL8_000000, "File not found");
		term_newline(term);
	}
}

// uname 命令
// TODO: 做成应用而不是命令
void cmd_uname(struct TERM *term, char *cmdline)
{
	char s[30] = {0};
	if (cmdline[6] == '-') {
		switch (cmdline[7]) {
			case 'h':
				term_putstr(term, "Usage: uname [OPTION]...");
				term_newline(term);
				term_putstr(term, "-a, print all information");
				term_newline(term);
				term_putstr(term, "-r, print kernel release");
				term_newline(term);
				term_putstr(term, "-v, print kernel version");
				term_newline(term);
				term_putstr(term, "-m, print system architecture");
				term_newline(term);
				term_putstr(term, "-o, print operating system");
				term_newline(term);
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
				term_putstr(term, "uname: extra operand");
				term_newline(term);
				term_putstr(term, "Try 'uname -h' for more information");
		}
	} else if (strcmp(cmdline, "uname") == 0) {
		term_putstr(term, kernel_name);
		term_newline(term);
	}
}

// halt 应用
void cmd_halt(struct TERM *term, int *fat)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct FILEINFO *finfo = file_search("halt.hrb", (struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);
	struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) ADR_GDT;
	char *p;
	if (finfo != 0) {// 找到文件
		p = (char *) memman_alloc_4k(memman, finfo->size);
		file_loadfile(finfo->clustno, finfo->size, p, fat, (char *) (ADR_DISKIMG + 0x003e00));
		set_segmdesc(gdt + 1003, finfo->size - 1, (int) p, AR_CODE32_ER);
		farjmp(0, 1003 * 8);
		memman_free_4k(memman, (int) p, finfo->size);
	} else {// 没有找到文件
		putfonts8_str_sht(term->sht, 8, term->cur_y, COL8_FFFFFF, COL8_000000, "File not found");
		term_newline(term);
	}
}
