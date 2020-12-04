#include "bootpack.h"
void init_pic(void)
/* PIC 初始化 */
{
	// 禁止所有中断
	io_out8(PIC0_IMR, 0xff);
	io_out8(PIC1_IMR, 0xff);

	// 边缘触发模式
	io_out8(PIC0_ICW1, 0x11);
	// IRQ0 ~ 7 由 INT20 ~ 27 接收
	io_out8(PIC0_ICW2, 0x20);
	// PIC1 由 IRQ2 连接
	io_out8(PIC0_ICW3, 1 << 2);
	// 无缓冲模式
	io_out8(PIC0_ICW4, 0x01);

	// 边缘触发模式
	io_out8(PIC1_ICW1, 0x11);
	// IRQ8 ~ 15 由 INT28 ~ 2f 接收
	io_out8(PIC1_ICW2, 0x28);
	// PIC1 由 IRQ2 连接
	io_out8(PIC1_ICW3, 2);
	// 无缓冲模式
	io_out8(PIC1_ICW4, 0x01);

	// 11111011 PIC1 以外全部禁止
	io_out8(PIC0_IMR,  0xfb);
	// 1111111 禁止所有中断
	io_out8(PIC1_IMR,  0xff);

	return;
}
