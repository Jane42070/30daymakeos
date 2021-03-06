#include "bootpack.h"

struct TIMERCTL timerctl;
struct FIFO32 timerfifo;

// 初始化PIT
void init_pit()
{
	int i;
	struct TIMER *t;
	io_out8(PIT_CTRL, 0x34);
	io_out8(PIT_CNT0, 0x9c);
	io_out8(PIT_CNT0, 0x2e);
	timerctl.count = 0;
	// 设置预先设置定时器为未使用状态
	for (i = 0; i < MAX_TIMER; i++) timerctl.timers0[i].flags = 0;
	// 设置一个哨兵
	t = timer_alloc();
	t->timeout = 0xffffffff;
	t->flags = TIMER_FLAGS_ACTING;
	// 因为目前只有一个
	t->next = 0;
	// 哨兵
	timerctl.t0 = t;
	// 只有哨兵，所以超时时刻就是哨兵的时刻
	timerctl.next = 0xffffffff;
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
void timer_init(struct TIMER *timer, struct FIFO32 *fifo, int data)
{
	timer->fifo = fifo;
	timer->data = data;
}

// 设置定时器时间
// 将 timer 注册到 timers 中去
// 事先关闭中断
// 由于设置了哨兵，需要判断的情况从 4 种变为 2 种
// 不可能插入到最后，只有一个定时器
void timer_settime(struct TIMER *timer, unsigned int timeout)
{
	struct TIMER *t, *s;
	timer->timeout = timeout + timerctl.count;
	timer->flags = TIMER_FLAGS_ACTING;
	int e = io_load_eflags();
	// 屏蔽其他中断
	io_cli();
	t = timerctl.t0;
	// 插入到最前面
	if (timer->timeout <= t->timeout) {
		timerctl.t0 = timer;
		// 然后 next 指向 t
		timer->next = t;
		// 修正 timeout
		timerctl.next = timer->timeout;
		io_store_eflags(e);
		return;
	}
	// 搜索插入位置
	for(;;) {
		s = t;
		t = t->next;
		// 最后面
		if (t == 0) break;
		// 插入到 s 和 t 之间时
		if (timer->timeout <= t->timeout) {
			s->next = timer;
			timer->next = t;
			io_store_eflags(e);
			return;
		}
	}
}

// 定时器中断处理
// timeout 下一个计时器时间
// timerctl.next 通过时间先后顺序来排列
// 对活动的定时器进行移位同时将超时的定时器置为失效
// 计算活动的定时器数量
// 如果有活动的定时器将 timeout 设置为最近的计时器时间
// 没有则设为默认值
// 因为设置了哨兵，acting 废除
void inthandler20(int *esp)
{
	char ts = 0;
	struct TIMER *timer;
	io_out8(PIC0_OCW2, 0x60);	// 把 IRQ-00 信号接收完了的信息通知给 PIC
	timerctl.count++;
	if (timerctl.next > timerctl.count) return;
	// 结构体指针 timer 指向第一个 timer （计时器）
	timer = timerctl.t0;
	for (;;) {
		// timers 的定时器都在活动，就不设置 flags
		if (timer->timeout > timerctl.count) break;
		// 超时
		timer->flags = TIMER_FLAGS_ALLOC;
		if (timer != task_timer) fifo32_put(timer->fifo, timer->data);
		else ts = 1;
		// timer->next 指向下一个定时器地址
		timer = timer->next;
	}
	// 直接移位
	timerctl.t0 = timer;
	timerctl.next = timer->timeout;
	if (ts != 0) task_switch();
}
