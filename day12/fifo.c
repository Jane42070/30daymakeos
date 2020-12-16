#include "bootpack.h"

/** 初始化FIFO缓冲区 */
void fifo8_init(struct FIFO8 *fifo, int size, unsigned char *buf)
{
	fifo->size = size;
	fifo->buf = buf;
	// 缓冲区大小
	fifo->free = size;
	// 当前索引
	fifo->flags = 0;
	// 下一个数据写入位置
	fifo->w = 0;
	// 下一个数据读出位置
	fifo->r = 0;
}

/** 向FIFO存放数据 */
int fifo8_put(struct FIFO8 *fifo, unsigned char data)
{
	if (fifo->free == 0) {
		// 空余没有了，溢出
		fifo->flags |= FLAGS_OVERRUN;
		return -1;
	}
	fifo->buf[fifo->w] = data;
	fifo->w++;
	if (fifo->w == fifo->size) {
		fifo->w = 0;
	}
	fifo->free--;
	return 0;
}

/** 向FIFO读取数据 */
int fifo8_get(struct FIFO8 *fifo)
{
	int data;
	if (fifo->free == fifo->size) {
		return -1;
	}
	data = fifo->buf[fifo->r];
	fifo->r++;
	if (fifo->r == fifo->size) {
		fifo->r = 0;
	}
	fifo->free++;
	return data;
}

/** 报告FIFO状态 */
int fifo8_status(struct FIFO8 *fifo)
{
	return fifo->size - fifo->free;
}
