#include "bootpack.h"

struct TIMERCTL timerctl;
struct FIFO8 timerfifo;

void init_pit()
{
	io_out8(PIT_CTRL, 0x34);
	io_out8(PIT_CNT0, 0x9c);
	io_out8(PIT_CNT0, 0x2e);
	timerctl.count = 0;
	timerctl.timeout = 0;
}

void inthandler20(int *esp)
{
	io_out8(PIC0_OCW2, 0x60);	// 把 IRQ-00 信号接收完了的信息通知给 PIC
	timerctl.count++;
	// 如果设置了超时时间
	if (timerctl.timeout > 0) {
		timerctl.timeout--;
		if (timerctl.timeout == 0) fifo8_put(timerctl.fifo, timerctl.data);
	}
}

void settimer(unsigned int timeout, struct FIFO8 *fifo, unsigned char data)
{
	int eflags = io_load_eflags();
	io_cli();
	timerctl.timeout = timeout;
	timerctl.fifo = fifo;
	timerctl.data = data;
	io_store_eflags(eflags);
}
