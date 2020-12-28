#include "bootpack.h"

void console_task(struct SHEET *sheet, unsigned int memtotal)
{
	struct TIMER *timer;
	struct TASK *task = task_now();
	char history[100] = {0};
	int i, fifobuf[128], cursor_x = 16, cursor_y = 28, cursor_c = -1, x, y;
	fifo32_init(&task->fifo, 128, fifobuf, task);
	struct FILEINFO *finfo = (struct FILEINFO *) (ADR_DISKIMG + 0x002600);
	struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) ADR_GDT;

	timer  = timer_alloc();
	timer_init(timer, &task->fifo, 1);
	timer_settime(timer, 50);
	char s[30] = {0}, cmdline[30] = {0}, *p;
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	int *fat = (int *) memman_alloc_4k(memman, 4 * 2880);
	file_readfat(fat, (unsigned char *) (ADR_DISKIMG + 0x000200));
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
							cursor_y -= 16;
						} else if (strncmp(cmdline, "cat ", 4) == 0) {// cat 命令
							for (y = 0; y < 11; y++) s[y] = ' ';
							y = 0;
							for (x = 4; y < 11 && cmdline[x] != 0; x++) {
								if (cmdline[x] == '.' && y <= 8) y = 8;
								else {
									s[y] = cmdline[x];
									if ('a' <= s[y] && s[y] <= 'z') s[y] -= 0x20;
									y++;
								}
							}
							for (x = 0; x < 224; ) {// 寻找文件
								if (finfo[x].name[0] == 0x00) break;
								if ((finfo[x].type & 0x18) == 0) {
									for (y = 0; y < 11; y++) {
										if (finfo[x].name[y] != s[y]) goto cat_next_file;
									}
									break;
								}
cat_next_file:
								x++;
							}
							if (x < 224 && finfo[x].name[0] != 0x00) {// 找到文件
								y = finfo[x].size;
								p = (char *) memman_alloc_4k(memman, finfo[x].size);
								file_loadfile(finfo[x].clustno, finfo[x].size, p, fat, (char *) (ADR_DISKIMG + 0x003e00));
								cursor_x = 8;
								for (y = 0; y < finfo[x].size; y++) {// 逐字输出
									s[0] = p[y];
									s[1] = 0;
									switch (s[0]) {
										case 0x09:// 制表符
											for (;;) {
												putfonts8_str_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, " ");
												cursor_x += 8;
												if (cursor_x == 8 + 240) {
													cursor_x = 8;
													cursor_y = cons_newline(cursor_y, sheet);
												}
												// 被 32 整除则 break
												// cursor_x - 8 因为命令行边框有 8 个像素
												// 制表符为 4 个空格，0x1f = 32
												if (((cursor_x - 8) & 0x1f) == 0) break;
											}
											break;
										case 0x0a:// 换行
											cursor_x = 8;
											cursor_y = cons_newline(cursor_y, sheet);
											break;
										case 0x0d:// 回车不做处理
											break;
										default:  // 一般字符
											putfonts8_str_sht(sheet, cursor_x, cursor_y, COL8_FFFFFF, COL8_000000, s);
											cursor_x += 8;
											if (cursor_x == 8 + 240) {// 达到右边界换行
												cursor_x = 8;
												cursor_y = cons_newline(cursor_y, sheet);
											}
									}
								}
								memman_free_4k(memman, (int) p, finfo[x].size);
							} else putfonts8_str_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, "File not found"); // 没有找到文件
						} else if (strcmp(cmdline, "halt") == 0) {
							for (y = 0; y < 11; y++) s[y] = ' ';
							s[0]  = 'H';
							s[1]  = 'A';
							s[2]  = 'L';
							s[3]  = 'T';
							s[8]  = 'H';
							s[9]  = 'R';
							s[10] = 'B';
							for (x = 0; x < 224; ) {
								if (finfo[x].name[0] == 0x00) break;
								if ((finfo[x].type & 0x18) == 0) {
									for (y = 0; y < 11; y++) {
										if (finfo[x].name[y] != s[y]) goto halt_next_file;
									}
									break; // 找到文件
								}
halt_next_file:
								x++;
							}
							if (x < 224 & finfo[x].name[0] != 0x00) {// 找到文件
								p = (char *) memman_alloc_4k(memman, finfo[x].size);
								file_loadfile(finfo[x].clustno, finfo[x].size, p, fat, (char *) (ADR_DISKIMG + 0x003e00));
								set_segmdesc(gdt + 1003, finfo[x].size - 1, (int) p, AR_CODE32_ER);
								farjmp(0, 1003 * 8);
								memman_free_4k(memman, (int) p, finfo[x].size);
								// 没有找到文件
							} else putfonts8_str_sht(sheet, 8, cursor_y, COL8_FFFFFF, COL8_000000, "File not found");
								
							
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
