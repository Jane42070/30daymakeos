#include "bootpack.h"

void HariMain(void)
{
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
	extern char hankaku[4096];
	char *mcursor;
	int mx = (binfo->scrnx - 16) / 2; /* ��Ļ */
	int my = (binfo->scrny - 28 - 16) / 2;
	unsigned char *s;

	// ��ʼ��16λ��ɫ
	init_palette();
	// ��ʼ�� ȫ�ֶμ�¼�����жϼ�¼��
	init_gdtidt();
	// ��ʼ�� PIC
	init_pic();
	// IDT/PIC �ĳ�ʼ���Ѿ���ɣ����� CPU ���ж�
	io_sti();


	init_screen((unsigned char *)binfo->vram, binfo->scrnx, binfo->scrny);
	init_mouse_cursor8(mcursor, COL8_008484);
	putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);

	putfont8_pos(binfo->vram, binfo->scrnx, 0, 8, COL8_FFFFFF, (unsigned char *)"hello world!");
	putfont8_pos(binfo->vram, binfo->scrnx, 0, 40, COL8_FFFFFF, (unsigned char *)"CLAY");
	putfont8_pos(binfo->vram, binfo->scrnx, 0, 92, COL8_FFFFFF, (unsigned char *)"HARIBOTE.SYS");

	// ����PIC1�ͼ����ж�(11111001)
	io_out8(PIC0_IMR, 0xf9);
	// ��������ж�(11101111)
	io_out8(PIC1_IMR, 0xef);

	for (;;) { io_hlt(); }
}