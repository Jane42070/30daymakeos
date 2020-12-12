#include "bootpack.h"
extern struct FIFO8 keyfifo;
extern struct FIFO8 mousefifo;

void wait_KBC_sendready(void);
void init_keyboard(void);
void enable_mouse(void);

void HariMain(void)
{
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
	extern char hankaku[4096];
	// Define FIFO buffer
	unsigned char s[40], mcursor[256], keybuf[32], mousebuf[128];
	int mx = (binfo->scrnx - 16) / 2; /* ��Ļ */
	int my = (binfo->scrny - 28 - 16) / 2;
	int i,j;

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
	putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
	sprintf((char *)s, "(%d, %d)", mx, my);
	putfont8_str(binfo->vram, binfo->scrnx, 0, 16, COL8_FFFFFF, s);



	putfont8_pos(binfo->vram, binfo->scrnx, 0, 30, COL8_FFFFFF, (unsigned char *)"hello world!");
	putfont8_pos(binfo->vram, binfo->scrnx, 0, 50, COL8_FFFFFF, (unsigned char *)"CLAY");
	putfont8_pos(binfo->vram, binfo->scrnx, 0, 92, COL8_FFFFFF, (unsigned char *)"HARIBOTE.SYS");

	// ����PIC1�ͼ����ж�(11111001)
	io_out8(PIC0_IMR, 0xf9);
	// ��������ж�(11101111)
	io_out8(PIC1_IMR, 0xef);
	// ��ʼ��keybuf������
	fifo8_init(&keyfifo, 32, keybuf);
	// ��ʼ��mousebuf������
	fifo8_init(&mousefifo, 128, mousebuf);

	init_keyboard();
	enable_mouse();

	// �����������
	for (;;) {
		// ���������ж�
		io_cli();
			// �����жϲ�����ȴ�
		if (fifo8_status(&keyfifo) + fifo8_status(&mousefifo) == 0) { io_stihlt(); }
		else {
			if (fifo8_status(&keyfifo) != 0) {
				i = fifo8_get(&keyfifo);
				io_sti();
				sprintf((char *)s, "%02X", i);
				boxfill8((unsigned char *)binfo->vram, binfo->scrnx, COL8_008484, 0, 0, 16, 16);
				putfont8_str(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);
			}
			else if (fifo8_status(&mousefifo) != 0)
			{
				i = fifo8_get(&mousefifo);
				io_sti();
				sprintf((char *)s, "%02X", i);
				boxfill8((unsigned char *)binfo->vram, binfo->scrnx, COL8_008484, 0, 32, 16, 48);
				putfont8_str(binfo->vram, binfo->scrnx, 0, 32, COL8_FFFFFF, s);
			}
		}
	}
}

void wait_KBC_sendready(void)
{
	/** �ȴ����̿��Ƶ�·׼����� */
	for(;;) { if ((io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0) { break; } }
}

void init_keyboard(void)
{
	/** ��ʼ�����̿��Ƶ�·
	 *  �����Ƶ�·�����ڼ��̿��Ƶ�·�� */
	wait_KBC_sendready();
	// ����귢��ģʽ�趨ָ�� 0x60
	io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
	// �ȴ���·��Ӧ
	wait_KBC_sendready();
	// �������ģʽ 0x47
	io_out8(PORT_KEYDAT, KBC_MODE);
}

void enable_mouse(void)
{
	/** ������� */
	wait_KBC_sendready();
	// �Լ��̵�·����ָ�� 0xd4
	io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
	// �ȴ���·��Ӧ
	wait_KBC_sendready();
	// �����������
	io_out8(PORT_KEYDAT, MOUSECMD_ENABLE); /** ����ɹ������̵�·�᷵��ACK(0xfa) */
}
