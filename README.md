# 30days make a os
- just note my daily practice
- day01 ~ day04 were missing because this repo was created at the very 5 day

## Progress
- day06 分割编译与中断处理
![day06](./day06/6.png)
- day07 FIFO 与鼠标控制
![day07](./day07/7.png)
- day08 鼠标控制与 32 位模式切换
![day08](./day08/8.png)
- day09 内存管理
	- 系统目前的内存分区表

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
- day10 叠加处理
