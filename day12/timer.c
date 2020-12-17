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
	// 初始化的时候没有正在运行的计时器
	timerctl.next = 0xffffffff;
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
	// 更新下一时刻
	if (timerctl.next > timer->timeout) timerctl.next = timer->timeout;
}

// 定时器中断处理
// timeout 下一个计时器时间
// timerctl.next 通过时间先后顺序来排列
// 先判断当前时间（timerctl.count）是否没有到达最近的计时器时间，没有达到直接返回
// 达到后先设置一个不可能达到的时间，遍历所有的计时器并选择激活的计时器
// 如果当前时间大于或等于计时时间，设置计时器为已配置状态（关闭当前计时器，但不重置为未配置状态）并对计数器缓冲区发送设置的数据
// 还未到达 timerctl.count 判断计时器时间是否合法，合法则将下一个计时器时间赋值到 timeout
void inthandler20(int *esp)
{
	int i;
	io_out8(PIC0_OCW2, 0x60);	// 把 IRQ-00 信号接收完了的信息通知给 PIC
	timerctl.count++;
	if (timerctl.next > timerctl.count) return;
	timerctl.next = 0xffffffff;
	for (i = 0; i < MAX_TIMER; i++) {
		if (timerctl.timer[i].flags == TIMER_FLAGS_USING) {
			if (timerctl.timer[i].timeout <= timerctl.count) {
				timerctl.timer[i].flags = TIMER_FLAGS_ALLOC;
				fifo8_put(timerctl.timer[i].fifo, timerctl.timer[i].data);
			}
			else {
				if (timerctl.next > timerctl.timer[i].timeout) timerctl.next = timerctl.timer[i].timeout;
			}
		}
	}
}
