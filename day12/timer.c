#include "bootpack.h"

struct TIMERCTL timerctl;
struct FIFO8 timerfifo;

// 初始化PIT
void init_pit()
{
	int i;
	io_out8(PIT_CTRL, 0x34);
	io_out8(PIT_CNT0, 0x9c);
	io_out8(PIT_CNT0, 0x2e);
	timerctl.count = 0;
	// 设置预先设置定时器为未使用状态
	for (i = 0; i < MAX_TIMER; i++) timerctl.timer[i].flags = 0;
}

// 分配定时器
struct TIMER *timer_alloc()
{
	int i;
	for (i = 0; i < MAX_TIMER; i++) {
		if (timerctl.timer[i].flags == 0) {
			timerctl.timer[i].flags = TIMER_FLAGS_ALLOC;
			return &timerctl.timer[i];
		}
	}
	// 没有未分配的定时器
	return 0;
}

// 释放定时器
// 设置为未使用
void timer_free(struct TIMER *timer)
{
	timer->flags = 0;
}

// 初始化定时器
void timer_init(struct TIMER *timer, struct FIFO8 *fifo, unsigned char data)
{
	timer->fifo = fifo;
	timer->data = data;
}

// 设置定时器时间
void timer_settime(struct TIMER *timer, unsigned int timeout)
{
	// 从现在开始 timectl.count 秒后
	timer->timeout = timeout + timerctl.count;
	timer->flags = TIMER_FLAGS_USING;
}

// 定时器中断处理
void inthandler20(int *esp)
{
	int i;
	io_out8(PIC0_OCW2, 0x60);	// 把 IRQ-00 信号接收完了的信息通知给 PIC
	timerctl.count++;
	for (i = 0; i < MAX_TIMER; i++) {
		if (timerctl.timer[i].flags == TIMER_FLAGS_USING) {
			if (timerctl.timer[i].timeout <= timerctl.count) {
				timerctl.timer[i].flags = TIMER_FLAGS_ALLOC;
				fifo8_put(timerctl.timer[i].fifo, timerctl.timer[i].data);
			}
		}
	}
}
