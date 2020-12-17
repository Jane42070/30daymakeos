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
	for (i = 0; i < MAX_TIMER; i++) timerctl.timers0[i].flags = 0;
}

// 分配定时器
struct TIMER *timer_alloc()
{
	int i;
	for (i = 0; i < MAX_TIMER; i++) {
		if (timerctl.timers0[i].flags == 0) {
			timerctl.timers0[i].flags = TIMER_FLAGS_ALLOC;
			return &timerctl.timers0[i];
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
// 将 timer 注册到 timers 中去
// 事先关闭中断
void timer_settime(struct TIMER *timer, unsigned int timeout)
{
	int e, i, j;
	timer->timeout = timeout + timerctl.count;
	timer->flags = TIMER_FLAGS_ACTING;
	e = io_load_eflags();
	// 屏蔽其他中断
	io_cli();
	// 搜索注册位置
	for (i = 0; i < timerctl.acting; i++) {
		if (timerctl.timers[i]->timeout >= timer->timeout) break;
	}
	// i 号之后全部一移位
	for (j = timerctl.acting; j < i; j--) timerctl.timers[j] = timerctl.timers[j - 1];
	timerctl.acting++;
	// 插入到空位上
	timerctl.timers[i] = timer;
	timerctl.next = timerctl.timers[0]->timeout;
	io_store_eflags(e);
}

// 定时器中断处理
// timeout 下一个计时器时间
// timerctl.next 通过时间先后顺序来排列
// 对活动的定时器进行移位同时将超时的定时器置为失效
// 计算活动的定时器数量
// 如果有活动的定时器将 timeout 设置为最近的计时器时间
// 没有则设为默认值
void inthandler20(int *esp)
{
	int i, j;
	io_out8(PIC0_OCW2, 0x60);	// 把 IRQ-00 信号接收完了的信息通知给 PIC
	timerctl.count++;
	if (timerctl.next > timerctl.count) return;
	for (i = 0; i < timerctl.acting; i++) {
		// timers 的定时器都在活动，就不设置 flags
		if (timerctl.timers[i]->timeout > timerctl.count) break;
		// 超时
		timerctl.timers[i]->flags = TIMER_FLAGS_ALLOC;
		fifo8_put(timerctl.timers[i]->fifo, timerctl.timers[i]->data);
	}
	// 正好有 i 个定时器超时了，其余的进行移位
	timerctl.acting -= i;
	for (j = 0; j < timerctl.acting; j++) timerctl.timers[j] = timerctl.timers[i + j];
	if (timerctl.acting > 0) timerctl.next = timerctl.timers[0]->timeout;
	else timerctl.next = 0xffffffff;
}
