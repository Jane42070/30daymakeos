#include "bootpack.h"
extern struct FIFO8 keyfifo;
extern struct FIFO8 mousefifo;

struct MOUSE_DEC {
	unsigned char buf[3], phase;
	// ����ƶ�������״̬
	int x, y, btn;
} mdec;

void wait_KBC_sendready(void);
void init_keyboard(void);
void enable_mouse(struct MOUSE_DEC *mdec);
int mouse_decode(struct MOUSE_DEC *mdec, unsigned char data);

void HariMain(void)
{
	// �ṹ��ָ��ָ�򴢴�ϵͳ��Ϣ�ĵ�ַ
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
	extern char hankaku[4096];
	// Define FIFO buffer
	unsigned char mcursor[256], keybuf[32], mousebuf[128];
	char s[40];
	int mx = (binfo->scrnx - 16) / 2; /* ��Ļ */
	int my = (binfo->scrny - 28 - 16) / 2;
	int i, j;

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
	putfont8_pos(binfo->vram, binfo->scrnx, 0, 50, COL8_FFFFFF, "CLAY");
	putfont8_pos(binfo->vram, binfo->scrnx, 0, 92, COL8_FFFFFF, "HARIBOTE.SYS");

	// ����PIC1�ͼ����ж�(11111001)
	io_out8(PIC0_IMR, 0xf9);
	// ��������ж�(11101111)
	io_out8(PIC1_IMR, 0xef);
	// ��ʼ��keybuf������
	fifo8_init(&keyfifo, 32, keybuf);
	// ��ʼ��mousebuf������
	fifo8_init(&mousefifo, 128, mousebuf);

	init_keyboard();
	enable_mouse(&mdec);

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
				sprintf(s, "%02X", i);
				boxfill8((unsigned char *)binfo->vram, binfo->scrnx, COL8_008484, 0, 0, 16, 16);
				putfont8_str(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);
			}
			else if (fifo8_status(&mousefifo) != 0)
			{
				i = fifo8_get(&mousefifo);
				io_sti();
				if (mouse_decode(&mdec, i) != 0) {
					sprintf(s, "[lcr %4d %4d]", mdec.x, mdec.y);
					if ((mdec.btn & 0x01) != 0) s[1] = 'L' ;
					if ((mdec.btn & 0x02) != 0) s[3] = 'R' ;
					if ((mdec.btn & 0x04) != 0) s[2] = 'C' ;
					boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 32, 16 + 15 * 8 - 1, 48);
					putfont8_str(binfo->vram, binfo->scrnx, 0, 32, COL8_FFFFFF, s);
					// ����ƶ�
					// �������
					boxfill8(binfo->vram, binfo->scrnx, COL8_008484, mx, my, mx + 15, my + 15);
					mx += mdec.x;
					my += mdec.y;
					// ��ֹ������Ļ
					if (mx < 0) mx = 0;
					if (my < 0) my = 0;
					if (mx > binfo->scrnx - 16) mx = binfo->scrnx - 16;
					if (my > binfo->scrny - 16) my = binfo->scrny - 16;
					sprintf(s, "(%3d %3d)", mx, my);
					// ��������
					boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 16, 100, 32);
					// ��ʾ����
					putfont8_str(binfo->vram, binfo->scrnx, 0, 16, COL8_FFFFFF, s);
					// ������
					putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
				}
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

void enable_mouse(struct MOUSE_DEC *mdec)
{
	/** ������� */
	wait_KBC_sendready();
	// �Լ��̵�·����ָ�� 0xd4
	io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
	// �ȴ���·��Ӧ
	wait_KBC_sendready();
	// �����������
	io_out8(PORT_KEYDAT, MOUSECMD_ENABLE); /** ����ɹ������̵�·�᷵��ACK(0xfa) */
	mdec->phase = 0;
}

int mouse_decode(struct MOUSE_DEC *mdec, unsigned char data)
{
	switch (mdec->phase) {
		/** �ȴ���귢�͵�һ���ֽ� */
		case 1:
			mdec->buf[0] = data;
			mdec->phase = 2;
			return 0;
		/** �ȴ���귢�͵ڶ����ֽ� */
		case 2:
			mdec->buf[1] = data;
			mdec->phase = 3;
			return 0;
		/** �ȴ���귢�͵������ֽ�
		 *	�������������� */
		case 3:
			mdec->buf[2] = data;
			mdec->phase = 1;
			mdec->btn = mdec->buf[0] & 0x07;
			mdec->x = mdec->buf[1];
			mdec->y = mdec->buf[2];
			if ((mdec->buf[0] & 0x10) != 0) mdec->x |= 0xffffff00;
			if ((mdec->buf[0] & 0x20) != 0) mdec->y |= 0xffffff00;
			// ����y�����뻭������෴
			mdec->y = - mdec->y;
			return 1;
		default:
			// �ȴ������� (0xfa)
			if (data == 0xfa) mdec->phase = 1;
	}
	return -1;
}
