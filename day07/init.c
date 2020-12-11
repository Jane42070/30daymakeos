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

/* 接收来自PS/2键盘的中断 */
void inthandler21(int *esp)
{
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
	// 从键盘接收数据
	/* 通知PIC的IRQ-01 已经受理完毕 */
	io_out8(PIC0_OCW2, 0x61);
	unsigned char data;
	data = io_in8(PORT_KEYDAT);
	if (keybuf.flag == 0) {
		keybuf.data = data;
		keybuf.flag = 1;
	}
}

/* 接收来自PS/2鼠标的中断 */
void inthandler2c(int *esp)
{
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
	boxfill8((unsigned char *)binfo->vram, binfo->scrnx, COL8_000000, 0, 0, 32 * 8 - 1, 15);
	putfont8_str(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, (unsigned char *)"INT 2C (IRQ-12) : PS/2 Mouse");
	for (;;) { io_hlt(); }
}

/* PIC0中断的不完整策略 */
/* 这个中断在Athlon64X2上通过芯片组提供的便利，只需执行一次 */
/* 这个中断只是接收，不执行任何操作 */
/* 为什么不处理？
	→  因为这个中断可能是电气噪声引发的、只是处理一些重要的情况。*/
void inthandler27(int *esp)
{
	io_out8(PIC0_OCW2, 0x67); /* 通知PIC的IRQ-07（参考7-1） */
	return;
}
