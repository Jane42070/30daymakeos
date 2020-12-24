#include "bootpack.h"

struct TASKCTL *taskctl;
struct TIMER *task_timer;

// 初始化所有预分配任务
struct TASK *task_init(struct MEMMAN *memman)
{
	int i;
	struct TASK *task;
	struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) ADR_GDT;
	taskctl = (struct TASKCTL *) memman_alloc_4k(memman, sizeof(struct TASKCTL)); // 给任务管理器分配内存
	for (i = 0; i < MAX_TASKS; i++) {
		taskctl->tasks0[i].flags = 0;
		taskctl->tasks0[i].sel = (TASK_GDT0 + i) * 8;
		set_segmdesc(gdt + TASK_GDT0 + i, 103, (int) &taskctl->tasks0[i].tss, AR_TSS32);
	}
	task = task_alloc();
	task->flags = 2;	// 活动中标志
	taskctl->running = 1;
	taskctl->now = 0;
	taskctl->tasks[0] = task;
	load_tr(task->sel);
	task_timer = timer_alloc();
	timer_settime(task_timer, 2);
	return task;
}

// 分配任务
struct TASK *task_alloc()
{
	int i;
	struct TASK *task;
	for (i = 0; i < MAX_TASKS; i++) {
		if (taskctl->tasks0[i].flags == 0) {
			task = &taskctl->tasks0[i];
			task->flags      = 1;			// 正在使用的标志
			task->tss.eflags = 0x00000202;	// IF = 1
			task->tss.eax    = 0;			// 先置为 0
			task->tss.ecx    = 0;
			task->tss.edx    = 0;
			task->tss.ebx    = 0;
			task->tss.ebp    = 0;
			task->tss.esi    = 0;
			task->tss.edi    = 0;
			task->tss.es     = 0;
			task->tss.ds     = 0;
			task->tss.fs     = 0;
			task->tss.gs     = 0;
			task->tss.ldtr   = 0;
			task->tss.iomap  = 0x40000000;
			return task;
		}
	}
	return 0; // 全部正在使用
}

// 运行任务
void task_run(struct TASK *task)
{
	task->flags = 2;	// 标志设为活动中
	// task 添加到 tasks 的末尾
	taskctl->tasks[taskctl->running] = task;
	// 正在运行的任务数量 + 1
	taskctl->running++;
}

// 任务切换
void task_switch()
{
	timer_settime(task_timer, 2);
	if (taskctl->running > 1) {	// 正在运行的任务数量大于 1
		taskctl->now++;	// 记录将要运行的任务
		if (taskctl->now == taskctl->running) taskctl->now = 0;	// 因为索引是从 0 开始
		farjmp(0, taskctl->tasks[taskctl->now]->sel);
	}
}

// 任务休眠
void task_sleep(struct TASK *task)
{
	int i;
	char ts = 0;	// task_switch flag
	if (task->flags == 2) {	// 如果指定任务处于唤醒状态
		if (task == taskctl->tasks[taskctl->now]) ts = 1;	// 让自己休眠后，还需要进行任务切换
		for (i = 0; i < taskctl->running; i++) if (taskctl->tasks[i] == task) break;	// 寻找 task 所在的位置
		taskctl->running--;
		if (i < taskctl->now) taskctl->now--;	// 需要移动成员，要相应的处理
		for (; i < taskctl->running; i++) taskctl->tasks[i] = taskctl->tasks[i + 1];	// 进行移位操作
		task->flags = 1;	// 置为不工作状态
		if (ts != 0) {	// 切换任务
			if (taskctl->now >= taskctl->running) taskctl->now = 0;	// 如果 now 的值出现异常，则进行修正，运行第一个分配的任务
			farjmp(0, taskctl->tasks[taskctl->now]->sel);	// 跳转到该代码段
		}
	}
}
