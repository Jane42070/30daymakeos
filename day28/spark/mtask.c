#include "bootpack.h"

struct TASKCTL *taskctl;
struct TIMER *task_timer;

// 初始化所有预分配任务
struct TASK *task_init(struct MEMMAN *memman)
{
	int i;
	struct TASK *task, *idle;
	struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) ADR_GDT;
	taskctl = (struct TASKCTL *) memman_alloc_4k(memman, sizeof(struct TASKCTL)); // 给任务管理器分配内存
	for (i = 0; i < MAX_TASKS; i++) {
		taskctl->tasks0[i].flags = 0;
		taskctl->tasks0[i].sel   = (TASK_GDT0 + i) * 8;
		taskctl->tasks0[i].tss.ldtr = (TASK_GDT0 + MAX_TASKS + i) * 8;
		set_segmdesc(gdt + TASK_GDT0 + i, 103, (int) &taskctl->tasks0[i].tss, AR_TSS32);
		set_segmdesc(gdt + TASK_GDT0 + MAX_TASKS + i, 15, (int) taskctl->tasks0[i].ldt, AR_LDT);
	}
	for (i = 0; i < MAX_TASKLEVELS; i++) {
		taskctl->level[i].running = 0;
		taskctl->level[i].now     = 0;
	}
	task = task_alloc();
	task->flags    = 2;	// 活动中标志
	task->priority = 2;	// 0.02 秒
	task->level    = 0;
	task_add(task);
	task_switchsub();	// LEVEL 设置
	load_tr(task->sel);
	task_timer = timer_alloc();
	timer_settime(task_timer, task->priority);

	idle = task_alloc();
	idle->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024;
	idle->tss.eip = (int) &task_idle;
	idle->tss.es  = 1 * 8;
	idle->tss.cs  = 2 * 8;
	idle->tss.ss  = 1 * 8;
	idle->tss.ds  = 1 * 8;
	idle->tss.fs  = 1 * 8;
	idle->tss.gs  = 1 * 8;
	task_run(idle, MAX_TASKLEVELS - 1, 1);

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
			task->tss.iomap  = 0x40000000;
			task->tss.ss0    = 0;
			return task;
		}
	}
	return 0; // 全部正在使用
}

// 运行任务，设定优先级
void task_run(struct TASK *task, int level, int priority)
{
	if (level < 0) level = task->level;	// 不改变 LEVEL
	if (priority > 0) task->priority = priority;
	if (task->flags == 2 && task->level != level) task_remove(task);
	if (task->flags != 2) {
		// 从休眠状态唤醒的情形
		task->level = level;
		task_add(task);
	}
	// 下次任务切换时检查 LEVEL
	taskctl->lv_change = 1;
}

// 任务切换
void task_switch()
{
	struct TASKLEVEL *tl = &taskctl->level[taskctl->now_lv];
	struct TASK *new_task, *now_task = tl->tasks[tl->now];
	tl->now++;
	if (tl->now == tl->running) tl->now = 0;
	if (taskctl->lv_change != 0) {
		task_switchsub();
		tl = &taskctl->level[taskctl->now_lv];
	}
	new_task = tl->tasks[tl->now];
	timer_settime(task_timer, new_task->priority);
	if (new_task != now_task) farjmp(0, new_task->sel);
}

// 任务休眠
void task_sleep(struct TASK *task)
{
	struct TASK *now_task;
	if (task->flags == 2) {
		now_task = task_now();	// 如果处于活动状态
		task_remove(task);		// 将任务 flag 变为 1 休眠
		if (task == now_task) {
			// 如果是让自己休眠，则进行任务切换
			task_switchsub();
			now_task = task_now();
			farjmp(0, now_task->sel);
		}
	}
}

// 返回在活动的 struct TASK 的内存地址
struct TASK *task_now()
{
	struct TASKLEVEL *tl = &taskctl->level[taskctl->now_lv];
	return tl->tasks[tl->now];
}

// 在相应优先级任务管理中添加任务
void task_add(struct TASK *task)
{
	struct TASKLEVEL *tl = &taskctl->level[task->level];
	if (tl->running < MAX_TASKS_LV) {	// 如果还放得下
		tl->tasks[tl->running] = task;
		tl->running++;
		task->flags = 2;	// 活动中标志
	}
}

// 在 struct TASKLEVEL 中删除任务
// 模仿 task_sleep
void task_remove(struct TASK *task)
{
	int i;
	struct TASKLEVEL *tl = &taskctl->level[task->level];
	// 寻找 task 所在的位置
	for (i = 0; i < tl->running; i++) if (tl->tasks[i] == task) break;
	tl->running--;
	if (i < tl->now) tl->now--;					// 移动成员
	if (tl->now >= tl->running) tl->now = 0;	// 如果now的值出现异常，修正
	task->flags = 1;							// 休眠
	for (; i < tl->running; i++) tl->tasks[i] = tl->tasks[i + 1];	// 移动成员
}

// 根据等级切换任务
void task_switchsub()
{
	int i;
	// 寻找存在活动任务的 ttaskctl->level
	for (i = 0; i < MAX_TASKLEVELS; i++) if (taskctl->level[i].running > 0) break;
	taskctl->now_lv = i;
	taskctl->lv_change = 0;
}

// 闲置任务
void task_idle()
{
	for (;;) io_hlt();
}
