#include "bootpack.h"
// 键盘缓冲区
struct FIFO32 *keyfifo;
int keydata0;

/* 接收来自PS/2键盘的中断 */
void inthandler21(int *esp)
{
	// 从键盘接收数据
	/* 通知PIC的IRQ-01 已经受理完毕 */
	io_out8(PIC0_OCW2, 0x61);
	int data = io_in8(PORT_KEYDAT);
	// FIFO
	fifo32_put(keyfifo, data + keydata0);
}

/** 等待键盘控制电路准备完毕 */
void wait_KBC_sendready(void)
{
	for(;;)  if ((io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0) { break; }
}

/** 初始化键盘控制电路
 *  鼠标控制电路集成在键盘控制电路上 */
void init_keyboard(struct FIFO32 *fifo, int data0)
{
	// 将 FIFO 缓冲区的信息保存到全局变量里
	keyfifo = fifo;
	keydata0 = data0;
	wait_KBC_sendready();
	// 对鼠标发送模式设定指令 0x60
	io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
	// 等待电路响应
	wait_KBC_sendready();
	// 开启鼠标模式 0x47
	io_out8(PORT_KEYDAT, KBC_MODE);
}
