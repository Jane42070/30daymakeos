#include "bootpack.h"

extern struct KEYBUF keybuf;

void HariMain(void)
{
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
	extern char hankaku[4096];
	char *mcursor;
	int mx = (binfo->scrnx - 16) / 2; /* ��Ļ */
	int my = (binfo->scrny - 28 - 16) / 2;
	int i,j;
	unsigned char *s;

	// ��ʼ��16λ��ɫ
	init_palette();
	// ��ʼ�� ȫ�ֶμ�¼���жϼ�¼��
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

	// �����������
	for (;;) {
		// ���������ж�
		io_cli();
		if (keybuf.next == 0) {
			// �����жϲ�����ȴ�
			io_stihlt();
		} else {
			keybuf.next--;
			i = keybuf.data[0];
			for (j = 0; j < keybuf.next; j++) keybuf.data[j] = keybuf.data[j + 1];
			io_sti();
			sprintf((char *)s, "%02X", i);
			boxfill8((unsigned char *)binfo->vram, binfo->scrnx, COL8_008484, 0, 0, 16, 16);
			putfont8_str(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);
		}
	}
}
