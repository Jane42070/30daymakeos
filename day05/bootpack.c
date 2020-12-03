#include <string.h>

void io_hlt(void);
void io_cli(void);
void io_out8(int port, int data);
int io_load_eflags(void);
void io_store_eflags(int eflags);

void init_palette(void);
void set_palette(int start, int end, unsigned char *rgb);
void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1);
void init_screen(unsigned char *vram, int x, int y);
void putfont8(char *vram, int xsize, int x, int y, char c, char *font);
void putfont8_str(char *vram, int xsize, int x, int y, char c, unsigned char *s);
void putfont8_pos(char *vram, int xsize, int pos,int y, char c, unsigned char *s);
void init_mouse_cursor8(char *mouse, char bc);
void putblock8_8(char *vram, int vxsize, int pxsize, int pysize, int px0, int py0, char *buf, int bxsize);

#define COL8_000000		0  /*  0: �� */
#define COL8_FF0000		1  /*  1: ���� */
#define COL8_00FF00		2  /*  2: ���� */
#define COL8_FFFF00		3  /*  3: ���� */
#define COL8_0000FF		4  /*  4: ���� */
#define COL8_FF00FF		5  /*  5: ���� */
#define COL8_00FFFF		6  /*  6: ǳ���� */
#define COL8_FFFFFF		7  /*  7: �� */
#define COL8_C6C6C6		8  /*  8: ���� */
#define COL8_840000		9  /*  9: ���� */
#define COL8_008400		10 /* 10: ���� */
#define COL8_848400		11 /* 11: ���� */
#define COL8_000084		12 /* 12: ���� */
#define COL8_840084		13 /* 13: ���� */
#define COL8_008484		14 /* 14: ǳ���� */
#define COL8_848484		15 /* 15: ���� */

/** ����������Ϣ
 * cyls ����
 * leds ָʾ��״̬
 * vmode ��ʾģʽ
 * reserve ��ת
 * */
struct BOOTINFO {
	char cyls, leds, vmode, reserve;
	short scrnx, scrny;
	char *vram;
};
/** �κż�¼ */
struct SEGMENT_DESCRIPTOR {
	short limit_low, base_low;
	char base_mid, access_right;
	char limit_high, base_high;
};
/** �жϼ�¼�� */
struct GATE_DESCRIPTOR {
	short offset_low, selector;
	char dw_count, access_right;
	short offset_high;
};

/** �κ���Ϣ���� */
void set_segmdesc(struct SEGMENT_DESCRIPTOR *sd, unsigned int limit, int base, int ar);
/** ȫ���жϼ�¼������ */
void set_gatedesc(struct GATE_DESCRIPTOR *gd, int offset, int selector, int ar);
/** GDT �� IDT ��ʼ������ */
void init_gdtidt(void);

/** װ�ضκżĴ�������
 *  װ���жϼ�¼����
 *  C�����޷�ʵ�֣��� asmhead.nas ��*/
void load_gdtr(int limit, int addr);
void load_idtr(int limit, int addr);

void HariMain(void)
{
	struct BOOTINFO *binfo = (struct BOOTINFO *) 0x0ff0;
	extern char hankaku[4096];
	char *mcursor;
	int mx = (binfo->scrnx - 16) / 2; /* ��Ļ */
	int my = (binfo->scrny - 28 - 16) / 2;
	unsigned char *s;

	init_palette();

	init_screen((unsigned char *)binfo->vram, binfo->scrnx, binfo->scrny);
	init_mouse_cursor8(mcursor, COL8_008484);
	putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);

	putfont8_pos(binfo->vram, binfo->scrnx, 0, 8, COL8_FFFFFF, (unsigned char *)"hello world!");
	putfont8_pos(binfo->vram, binfo->scrnx, 0, 40, COL8_FFFFFF, (unsigned char *)"CLAY");
	putfont8_pos(binfo->vram, binfo->scrnx, 0, 92, COL8_FFFFFF, (unsigned char *)"HARIBOTE.SYS");


	/** sprintf(s, "scrnx = { %d }", binfo->scrnx); */
	/** putfont8_str(binfo->vram, binfo->scrnx, 100, 66, COL8_FFFFFF, s); */

	for (;;) {
		io_hlt();
	}
}

void init_palette(void)
{
	static unsigned char table_rgb[16 * 3] = {
		0x00, 0x00, 0x00,	/*  0: �� */
		0xff, 0x00, 0x00,	/*  1: ���� */
		0x00, 0xff, 0x00,	/*  2: ���� */
		0xff, 0xff, 0x00,	/*  3: ���� */
		0x00, 0x00, 0xff,	/*  4: ���� */
		0xff, 0x00, 0xff,	/*  5: ���� */
		0x00, 0xff, 0xff,	/*  6: ǳ���� */
		0xff, 0xff, 0xff,	/*  7: �� */
		0xc6, 0xc6, 0xc6,	/*  8: ���� */
		0x84, 0x00, 0x00,	/*  9: ���� */
		0x00, 0x84, 0x00,	/* 10: ���� */
		0x84, 0x84, 0x00,	/* 11: ���� */
		0x00, 0x00, 0x84,	/* 12: ���� */
		0x84, 0x00, 0x84,	/* 13: ���� */
		0x00, 0x84, 0x84,	/* 14: ǳ���� */
		0x84, 0x84, 0x84,	/* 15: ���� */
	};
	set_palette(0, 15, table_rgb);
	return;
}

void set_palette(int start, int end, unsigned char *rgb)
{
	int i, eflags;
	eflags = io_load_eflags();	/* ��¼�ж���ɱ�־��ֵ */
	io_cli(); 					/* ���ж���ɱ�־��Ϊ 0����ֹ�ж� */
	io_out8(0x03c8, start);
	for (i = start; i <= end; i++) {
		io_out8(0x03c9, rgb[0] / 4);
		io_out8(0x03c9, rgb[1] / 4);
		io_out8(0x03c9, rgb[2] / 4);
		rgb += 3;
	}
	io_store_eflags(eflags);	/* ��ԭ�ж���ɱ�־ */
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

/** ��ʼ����Ļ
 *	vram �Դ���ʼ��ַ
 *	x ������Ļ����
 *	y ������Ļ����
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

/** �����ַ�
 * vram �Դ���ʼ��ַ
 * xsize ��Ļ���ؿ��
 * x ��ʼ x ����
 * y ��ʼ y ����
 * font �ַ�
 * */
void putfont8(char *vram, int xsize, int x, int y, char c, char *font)
{
	int i;
	char *p, d /* data */;
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
	return;
}
/** ��ʾ�ַ�������
 * vram �Դ���ʼ��ַ
 * xsize ������Ļ����
 * x ��ʼ x ����
 * y ��ʼ y ����
 * c �궨����ɫ
 * s Ŀ���ַ���*/
void putfont8_str(char *vram, int xsize, int x, int y, char c, unsigned char *s)
{
	extern char hankaku[4096];
	for (; *s != 0 ; s++) {putfont8(vram, xsize, x, y, c, hankaku + *s * 16); x+=8;}
	return;
}

/* ����λ����ʾ�ַ��� */
void putfont8_pos(char *vram, int xsize, int pos,int y, char c, unsigned char *s)
{
	extern char hankaku[4096];
	unsigned char *start = s;
	int len = 0;

	for (; *start != 0 ; start++) { len+=8; }
	putfont8_str(vram, xsize, xsize/2 - len/2, y, c, s);
	return;
}

/** ���ָ����� */
void init_mouse_cursor8(char *mouse, char bc)
{
	static char cursor[16][16] = {
		"1111............",
		"1001111.........",
		"100000111.......",
		"11000000011.....",
		".100000000011...",
		".100000000000111",
		".110000000000001",
		"..10000000111111",
		"...10000001.....",
		"...100000001....",
		"...1100010001...",
		".....100110001..",
		".....1001.10001.",
		".....1101..10001",
		"......101...1001",
		"......111....111"
	};
	int x, y;

	for (y = 0; y < 16; y++) {
		for (x = 0; x < 16; x++) {
			if (cursor[y][x] == '0') { mouse[y * 16 + x] = COL8_000000; }
			if (cursor[y][x] == '1') { mouse[y * 16 + x] = COL8_FFFFFF; }
			if (cursor[y][x] == '.') { mouse[y * 16 + x] = bc; }
		}
	}
	return;
}

/** ������꺯�� */
void putblock8_8(char *vram, int vxsize, int pxsize, int pysize, int px0, int py0, char *buf, int bxsize)
{
	int x, y;
	for (y = 0; y < pysize; y++) {
		for (x = 0; x < pxsize; x++) { vram[(py0 + y) * vxsize + (px0 + x)] = buf[y * bxsize + x]; }
	}
	return;
}

void init_gdtidt(void)
{
	/* 0x00270000 ~ 0x0027ffff ��Ϊ GDT */
	struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) 0x00270000;
	/* 0x0026f800 ~ 0x0026ffff ��Ϊ IDT */
	struct GATE_DESCRIPTOR    *idt = (struct GATE_DESCRIPTOR    *) 0x0026f800;
	int i;

	/* GDT ��ʼ�� */
	for (i = 0; i < 8192; i++) {
		/* ���ö� ���ޡ���ַ������ */
		set_segmdesc(gdt + i, 0, 0, 0);
	}
	/* ���� CPU �ܹ�����ڴ� */
	set_segmdesc(gdt + 1, 0xffffffff, 0x00000000, 0x4092);
	/* Ϊ bootpack.hrb ���ö�2 */
	set_segmdesc(gdt + 2, 0x0007ffff, 0x00280000, 0x409a);
	load_gdtr(0xffff, 0x00270000);

	/* IDT ��ʼ�� */
	for (i = 0; i < 256; i++) {
		set_gatedesc(idt + i, 0, 0, 0);
	}
	load_idtr(0x7ff, 0x0026f800);

	return;
}

void set_segmdesc(struct SEGMENT_DESCRIPTOR *sd, unsigned int limit, int base, int ar)
{
	if (limit > 0xfffff) {
		ar |= 0x8000; /* G_bit = 1 */
		limit /= 0x1000;
	}
	sd->limit_low    = limit & 0xffff;
	sd->base_low     = base & 0xffff;
	sd->base_mid     = (base >> 16) & 0xff;
	sd->access_right = ar & 0xff;
	sd->limit_high   = ((limit >> 16) & 0x0f) | ((ar >> 8) & 0xf0);
	sd->base_high    = (base >> 24) & 0xff;
	return;
}

void set_gatedesc(struct GATE_DESCRIPTOR *gd, int offset, int selector, int ar)
{
	gd->offset_low   = offset & 0xffff;
	gd->selector     = selector;
	gd->dw_count     = (ar >> 8) & 0xff;
	gd->access_right = ar & 0xff;
	gd->offset_high  = (offset >> 16) & 0xffff;
	return;
}
