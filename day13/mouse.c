#include "bootpack.h"
// 鼠标缓冲区
struct FIFO8 mousefifo;
// 鼠标实时数据结构体
struct MOUSE_DEC mdec;

/* 接收来自PS/2鼠标的中断 */
void inthandler2c(int *esp)
{
	// 从鼠标接收数据
	/* 通知 PIC1 IRQ-12 的受理已经完成 */
	io_out8(PIC1_OCW2, 0x64);
	/* 通知 PIC0 IRQ-02 的受理已经完成 */
	io_out8(PIC0_OCW2, 0x62);
	unsigned char data = io_in8(PORT_KEYDAT);
	fifo8_put(&mousefifo, data);
}

void enable_mouse(struct MOUSE_DEC *mdec)
{
	/** 激活鼠标 */
	wait_KBC_sendready();
	// 对键盘电路发送指令 0xd4
	io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
	// 等待电路响应
	wait_KBC_sendready();
	// 发送鼠标命令
	io_out8(PORT_KEYDAT, MOUSECMD_ENABLE); /** 如果成功，键盘电路会返回ACK(0xfa) */
	mdec->phase = 0;
}

int mouse_decode(struct MOUSE_DEC *mdec, unsigned char data)
{
	switch (mdec->phase) {
		/** 等待鼠标发送第一个字节 */
		case 1:
			mdec->buf[0] = data;
			mdec->phase = 2;
			return 0;
		/** 等待鼠标发送第二个字节 */
		case 2:
			mdec->buf[1] = data;
			mdec->phase = 3;
			return 0;
		/** 等待鼠标发送第三个字节
		 *	并处理所有数据 */
		case 3:
			mdec->buf[2] = data;
			mdec->phase = 1;
			mdec->btn = mdec->buf[0] & 0x07;
			mdec->x = mdec->buf[1];
			mdec->y = mdec->buf[2];
			if ((mdec->buf[0] & 0x10) != 0) mdec->x |= 0xffffff00;
			if ((mdec->buf[0] & 0x20) != 0) mdec->y |= 0xffffff00;
			// 鼠标的y方向与画面符号相反
			mdec->y = - mdec->y;
			return 1;
		default:
			// 等待鼠标就绪 (0xfa)
			if (data == 0xfa) mdec->phase = 1;
	}
	return -1;
}
