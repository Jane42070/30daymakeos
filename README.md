<!-- vim-markdown-toc GFM -->

* [30days make a os](#30days-make-a-os)
	* [Progress](#progress)
		* [day05 包括之前的处理](#day05-包括之前的处理)
		* [day06 分割编译与中断处理](#day06-分割编译与中断处理)
		* [day07 FIFO 与鼠标控制](#day07-fifo-与鼠标控制)
		* [day08 鼠标控制与 32 位模式切换](#day08-鼠标控制与-32-位模式切换)
		* [day09 内存管理](#day09-内存管理)
		* [day10 叠加处理](#day10-叠加处理)
		* [day11 制作窗口](#day11-制作窗口)
		* [day12 定时器](#day12-定时器)
		* [day13 继续优化定时器](#day13-继续优化定时器)

<!-- vim-markdown-toc -->

# 30days make a os
- just note my daily practice
- day01 ~ day04 were missing because this repo was created at the very 5 day

## Progress
### day05 包括之前的处理
### day06 分割编译与中断处理
- 将 bootpack.c 进行分割，更改 makefile 编译
- PIC 分为主 PIC 和从 PIC，主 PIC 的第 2 个 IO 引脚连接从 PIC，对其注册后接收从 PIC 的中断信号
- 初始化 PIC，处理鼠标和键盘中断，接收电气中断但不处理
- 键盘中断处理顺利，鼠标中断处理有问题，暂时没找到原因
![day06](./day06/6.png)
### day07 FIFO 与鼠标控制
- 整理文件
- 创建 FIFO 缓冲区，优化键盘数据和鼠标数据接收速度
- 通过资料得出鼠标的处理电路集成在键盘处理电路上，向键盘电路发送指令注册鼠标处理电路然后才能接收中断
- 鼠标一次性发送 3 字节数据，需要比键盘更大的缓冲区
- 对 FIFO 缓冲区进行优化，采用循环链表，不需要进行移位操作，提高缓冲区处理速度和性能
![day07](./day07/7.png)
### day08 鼠标控制与 32 位模式切换
- 整理文件
- 对鼠标发送的 3 字节数据进行处理，采用`switch`提高处理速度，在`for(;;)`里进行不断的接收数据
- 鼠标的渲染会将其他画面的渲染覆盖，思考处理方式
- haribote.sys - 9.21KB
![day08](./day08/8.png)
### day09 内存管理
- 整理文件
- 通过对内存每字节写入数据进行容量检查
- 单个字节写入太慢，每 4KB 的开头一个字节写入，写入后重置原状态，提高检查速度
- 开机先进行内存容量检查（是否有内存坏块）
	- 由于 386 架构以上 CPU 有高速缓存，所以在内存检查时检查架构
		- 通过检查 EFLAGS 寄存器的 AC 标志位（第 18 位）是否为 1 并设置为 0 关闭高速缓存
		- 进行内存检查`memtest()`后再重置为原状态
	- 编译器会自动优化`memtest()`，决定用汇编
- 采用列表式段内存管理分配 - 管理 3GB 的内存只需要 8KB 左右的空间
- 系统目前的内存分区表
- haribote.sys - 8.95KB

| 地址                    | 用途                             |
|-------------------------|----------------------------------|
| 0x00000000 ~ 0x000fffff | 在启动中使用，但之后变空 （1MB） |
| 0x00100000 ~ 0x00267fff | 用途保存磁盘的内容 （1440KB）    |
| 0x00268000 ~ 0x0026f7ff | 空（30KB）                       |
| 0x0026f800 ~ 0x026fffff | IDT（2KB）                       |
| 0x00270000 ~ 0x0027ffff | GDT （64KB）                     |
| 0x00280000 ~ 0x002fffff | bootpack.hrb（512KB）            |
| 0x00300000 ~ 0x003fffff | 栈及其他（1MB）                  |
| 0x00400000 ~            | 空                               |

![day09](./day09/9.png)
### day10 叠加处理
- 整理文件
- 继续改善内存管理，考虑到进行大量的内存分配释放后，内存中会出现不连续的空闲碎片，占用`man->frees`段数量
- 编写总是以 0x1000 字节（4KB）为单位进行分配或释放的函数，采用与运算进行舍入运算
- 通过内存管理的思想创建一个图层渲染管理（叠加处理）的函数
- 通过结构体`SHEET`记录图层的各种信息，`SHEETCTL`结构体对所有图层进行记录管理，设置图层最多 256 个
- 编写画面刷新函数，使用透明色对鼠标的背景颜色进行重新绘制
- 全局画面刷新性能太差，采用局部刷新，提高画面刷新的速度和性能
- haribote.sys - 10.8KB
![day10](./day10/10.png)

### day11 制作窗口
- 鼠标显示问题，设置为可以隐藏，让局部刷新函数不刷新画面外的内容（增加`if`判断限制范围）
- 简化`sheet.c`的函数调用，将`SHTCTL`设置到`sheet`层级窗口内部
- 添加了绘制窗口的函数`make_window8`，修正了字体`hankaku`
	- 关闭按钮来自鼠标绘制算法
	- 窗体修改了初始化屏幕算法
- 添加一个层级（一下统称窗口）时的步骤：
	```c
	// 声明此窗口的缓冲区
	unsigned char *win_buf;
	// 向层级管理中注册此窗口
	sht_win = sheet_alloc(SHTCTL);
	// 向内存管理器申请内存 160 * 68 = 10880
	win_buf = (unsigned char *) memman_alloc_4k(memman, 160 * 68);
	// 设置窗口的信息
	sheet_setbuf(sht_win, buf_win, 160, 68, -1);
	// 创建窗口
	make_window8(buf_win, 160, 68, "Window");
	// 设置窗口在移动时能够进行叠加刷新
	sheet_slide(sht_win, 80, 72);
	// 设置窗口叠加显示的优先级
	sheet_updown(sht_win, 1);
	```
- 添加高速计数器窗口，但是显示的内容在闪烁，判断应该是`sheet_refresh`的问题
	- `sheet_refresh`思想：从显示优先度低的先刷新，然后到高的刷新
	- 既然`Windows`没有这种闪烁，就肯定有解决办法
- 消除了窗口的闪烁，在`sheet_refresh`函数里添加了一个高度参数，只有此高度以上的窗口才进行刷新，但是鼠标会闪烁
	- 思路：在鼠标所在的`VRAM`不进行处理
- 在内存开辟一块空间存放`VRAM`的映射，储存在`struct SHTCTL->vmap`，用图层号码而不是色号存入内存地址
- 每一次画面的变动都会相应的在`SHTCTL->vmap`记录，在`sheet_refreshsub`改动使其每次对照`vmap`的内容对`vram`进行写入
- 避免从下往顶刷新，创建高度限定参数`h1`，对调用`sheet_refreshsub`的其他函数进行修改
![day11](./day11/11.png)

### day12 定时器
- 使用定时器（PIT Programmable Interval Timer）
- 要管理定时器需要对定时器进行设定
- 定时器在以前是作为单独的芯片安装在主板上，现在和 PIC 一样被集成到了其他芯片里
- 连接 IRQ（引脚） 的 0 号，只要设定了 PIT 就可以设定 IRQ0 的中断间隔
```asm
; IRQ0 的中断周期变更
AL= 0x34:OUT(0x43,AL)
AL= 中断周期的低 8 位 ;OUT(0x40,AL)
AL= 中断周期的高 8 位 ;OUT(0x40,AL)
; 中断产生的频率 = 主频 / 设定的数值
; 指定中断周期为 0，默认为 65536
; 设定周期为 1000，中断产生的频率为 119.318HZ
; 设定为 11932，频率就是 100HZ
; 11932 = 0x2e9c
```
- 从`IRQ0`的中断变更可以看出需要执行 3 次 OUT 指令来完成设定，第一个指令的输出地址为 0x43，激活`IRQ0`中断
```c
#define PIT_CTRL	0x004
#define PIT_CNT0	0x0040

void init_pit()
{
	io_out8(PIT_CTRL, 0x34);
	io_out8(PIT_CNT0, 0x9c);
	io_out8(PIT_CNT0, 0x2e);
}
```
- 在中断记录表里注册计时器中断在`0x20`地址
- 使用 `PIT` 记录开机时间
	- 定义`struct TIMERCTL`，定义一个`count`变量
	- 初始化时设置`count = 0`，每次发生中断时，`count++`
- 设置一个定时器，到达时间后显示
	- 此时的`TIMERCTL`
```c
// 计时器管理
// 计时 count
// 剩余时间 timeout
// 缓冲区 fifo
// 剩余时间没有后需要向缓冲区写入的数据 data
struct TIMERCTL {
	unsigned int count;
	unsigned int timeout;
	struct FIFO8 *fifo;
	unsigned char data;
};
```
	- 写一个倒计时函数`settimer()`
	- 在`bootpack.c`中进行相应的处理
- 设置多个定时器，思路和图层管理器一样，设置一个定时器管理，预先设置好最多能有`500`个定时器
- 绘制闪烁光标，方便以后使用在文本编辑器，终端等应用场景中
- 加快中断处理 `timeout`不再是剩余时间，而是给定的计时时间，免除了每次中断处理需要当前活动计时器的`timeout`自减的操作
- 由于`timectl.count`的类型是`int`，最大值为`0xffffffff`，不能无限制计时，设定为 1 年时间后计时器归零
	- 或许有更好的方法，是否应该储存为年月日时分秒形式？
- 再次加快中断处理
	- 原`timer.c`中断处理
```c
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

```

	由于设置的 1ms 发生一次中断，1s = 100 次中断，MAX_TIMER = 500，所以 1s 执行 50000 次 if，49998 次是无用功
- 添加`timerctl.next`优化`timer.c` - 短时先判断
- 继续优化中断处理，用`struct SHTCTL`思路在`struct TIMERCTL`中创建一个排序好的`timers`
- haribote.sys 23.1KB
![day12](./day12/12.png)
### day13 继续优化定时器
- 修复了 day12 的 bug，应该是 `j > i` 而不是 `j < i`
- 简化叠加处理字符串显示
- 调整计时器缓冲区 `timerfifo`，优化了过程
- 性能测试

| 版本  | 虚拟机    |
|-------|-----------|
| day12 | 113228031 |
| day12 | 115222105 |
| day12 | 115367183 |
| day12 | 114781422 |

- 整合缓冲区，`timer, keyboard, mouse` 使用一个缓冲区 `struct FIFO32 *fifo`
- 性能测试

| 版本  | 虚拟机    |
|-------|-----------|
| day13 | 211905694 |
| day13 | 212633087 |
| day13 | 211646820 |
| day13 | 211641338 |

- 整合缓冲区后性能提升 1.85 倍
- 加快中断处理
