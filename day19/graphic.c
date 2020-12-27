#include "bootpack.h"

/* 画面设定 */
void init_palette(void)
{
	static unsigned char table_rgb[16 * 3] = {
		0x00, 0x00, 0x00,	/*  0: 黑 */
		0xff, 0x00, 0x00,	/*  1: 亮红 */
		0x00, 0xff, 0x00,	/*  2: 亮绿 */
		0xff, 0xff, 0x00,	/*  3: 亮黄 */
		0x00, 0x00, 0xff,	/*  4: 亮蓝 */
		0xff, 0x00, 0xff,	/*  5: 亮紫 */
		0x00, 0xff, 0xff,	/*  6: 浅亮蓝 */
		0xff, 0xff, 0xff,	/*  7: 白 */
		0xc6, 0xc6, 0xc6,	/*  8: 亮灰 */
		0x84, 0x00, 0x00,	/*  9: 暗红 */
		0x00, 0x84, 0x00,	/* 10: 暗绿 */
		0x84, 0x84, 0x00,	/* 11: 暗黄 */
		0x00, 0x00, 0x84,	/* 12: 暗青 */
		0x84, 0x00, 0x84,	/* 13: 暗紫 */
		0x00, 0x84, 0x84,	/* 14: 浅暗蓝 */
		0x84, 0x84, 0x84,	/* 15: 暗灰 */
	};
	set_palette(0, 15, table_rgb);
	return;
}

void set_palette(int start, int end, unsigned char *rgb)
{
	int i, eflags;
	eflags = io_load_eflags();	/* 记录中断许可标志的值 */
	io_cli(); 					/* 将中断许可标志置为 0，禁止中断 */
	io_out8(0x03c8, start);
	for (i = start; i <= end; i++) {
		io_out8(0x03c9, rgb[0] / 4);
		io_out8(0x03c9, rgb[1] / 4);
		io_out8(0x03c9, rgb[2] / 4);
		rgb += 3;
	}
	io_store_eflags(eflags);	/* 复原中断许可标志 */
	return;
}

void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1)
{
	int x, y;
	for (y = y0; y <= y1; y++) {
		for (x = x0; x <= x1; x++)
			vram[y * xsize + x] = c;
	}
	return;
}

/** 初始化屏幕
 *	vram 显存起始地址
 *	x 横向屏幕像素
 *	y 纵向屏幕像素
 * */
void init_screen(unsigned char *vram, int x, int y)
{
	boxfill8(vram, x, COL8_008484,  0,     0,      x -  1, y - 29);
	boxfill8(vram, x, COL8_C6C6C6,  0,     y - 28, x -  1, y - 28);
	boxfill8(vram, x, COL8_FFFFFF,  0,     y - 27, x -  1, y - 27);
	boxfill8(vram, x, COL8_C6C6C6,  0,     y - 26, x -  1, y -  1);

	boxfill8(vram, x, COL8_FFFFFF,  3,     y - 24, 59,     y - 24);
	boxfill8(vram, x, COL8_FFFFFF,  2,     y - 24,  2,     y -  4);
	boxfill8(vram, x, COL8_848484,  3,     y -  4, 59,     y -  4);
	boxfill8(vram, x, COL8_848484, 59,     y - 23, 59,     y -  5);
	boxfill8(vram, x, COL8_000000,  2,     y -  3, 59,     y -  3);
	boxfill8(vram, x, COL8_000000, 60,     y - 24, 60,     y -  3);

	boxfill8(vram, x, COL8_848484, x - 47, y - 24, x -  4, y - 24);
	boxfill8(vram, x, COL8_848484, x - 47, y - 23, x - 47, y -  4);
	boxfill8(vram, x, COL8_FFFFFF, x - 47, y -  3, x -  4, y -  3);
	boxfill8(vram, x, COL8_FFFFFF, x -  3, y - 24, x -  3, y -  3);
	return;
}

/** 绘制字符
 * vram 显存起始地址
 * xsize 屏幕像素宽度
 * x 起始 x 坐标
 * y 起始 y 坐标
 * font 字符
 * */
void putfont8(unsigned char *vram, int xsize, int x, int y, char c, char *font)
{
	int i;
	unsigned char *p, d /* data */;
	for (i = 0; i < 16; i++) {
		p = vram + (y + i) * xsize + x;
		d = font[i];
		if ((d & 0x80) != 0) { p[0] = c; }
		if ((d & 0x40) != 0) { p[1] = c; }
		if ((d & 0x20) != 0) { p[2] = c; }
		if ((d & 0x10) != 0) { p[3] = c; }
		if ((d & 0x08) != 0) { p[4] = c; }
		if ((d & 0x04) != 0) { p[5] = c; }
		if ((d & 0x02) != 0) { p[6] = c; }
		if ((d & 0x01) != 0) { p[7] = c; }
	}
}
/** 显示字符串函数
 * vram 显存起始地址
 * xsize 横向屏幕像素
 * x 起始 x 坐标
 * y 起始 y 坐标
 * c 宏定义颜色
 * s 目标字符串*/
void putfonts8_str(unsigned char *vram, int xsize, int x, int y, char c, char *s)
{
	extern char hankaku[4096];
	for (; *s != 0 ; s++) { putfont8(vram, xsize, x, y, c, hankaku + *s * 16); x+=8; }
}

/** 叠加处理显示字符串函数
 * x, y 显示位置坐标
 * c 字符颜色
 * b 背景颜色
 * s 字符串
 * */
void putfonts8_str_sht(struct SHEET *sht, int x, int y, int c, char b, char *s)
{
	int l = strlen(s);
	boxfill8(sht->buf, sht->bxsize, b, x, y, x + l * 8 - 1, y + 15);
	putfonts8_str(sht->buf, sht->bxsize, x, y, c, s);
	sheet_refresh(sht, x, y, x + l * 8, y + 16);
}

/** 鼠标指针绘制 */
void init_mouse_cursor8(unsigned char *mouse, char bc)
{
	static char cursor[16][16] = {
		"**************.",
		"*OOOOOOOOOOO*...",
		"*OOOOOOOOOO*....",
		"*OOOOOOOOO*.....",
		"*OOOOOOOO*......",
		"*OOOOOOO*.......",
		"*OOOOOOO*.......",
		"*OOOOOOOO*......",
		"*OOOO**OOO*.....",
		"*OOO*..*OOO*....",
		"*OO*....*OOO*...",
		"*O*......*OOO*..",
		"**........*OOO*.",
		"*..........*OOO*",
		"............*OO*",
		".............***"
	};
	// static char cursor[16][16] = {
	//     "1111............",
	//     "1001111.........",
	//     "100000111.......",
	//     "11000000011.....",
	//     ".100000000011...",
	//     ".100000000000111",
	//     ".110000000000001",
	//     "..10000000111111",
	//     "...10000001.....",
	//     "...100000001....",
	//     "...1100010001...",
	//     ".....100110001..",
	//     ".....1001.10001.",
	//     ".....1101..10001",
	//     "......101...1001",
	//     "......111....111"
	// };
	int x, y;

	for (y = 0; y < 16; y++) {
		for (x = 0; x < 16; x++) {
			switch (cursor[y][x]) {
				case '0':
					mouse[y * 16 + x] = COL8_000000;
					break;
				case '1':
					mouse[y * 16 + x] = COL8_FFFFFF;
					break;
				case '.':
					mouse[y * 16 + x] = bc;
					break;
			}
		}
	}
	return;
}

/** 放置鼠标函数 */
void putblock8_8(unsigned char *vram, int vxsize, int pxsize, int pysize, int px0, int py0, unsigned char *buf, int bxsize)
{
	int x, y;
	for (y = 0; y < pysize; y++) {
		for (x = 0; x < pxsize; x++) { vram[(py0 + y) * vxsize + (px0 + x)] = buf[y * bxsize + x]; }
	}
	return;
}
