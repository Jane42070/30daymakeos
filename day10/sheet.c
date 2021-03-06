#include "bootpack.h"

// 层级管理初始化
struct SHTCTL *shtctl_init(struct MEMMAN *memman, unsigned char *vram, int xsize, int ysize)
{
	struct SHTCTL *ctl;
	int i;
	ctl = (struct SHTCTL *) memman_alloc(memman, sizeof(struct SHTCTL));
	if (ctl == 0) goto err;
	ctl->vram  = vram;
	ctl->xsize = xsize;
	ctl->ysize = ysize;
	ctl->top   = -1;	// 一个SHEET都没有
	for (i = 0; i < MAX_SHEETS; i++) {
		ctl->sheets0[i].flags = 0;
	}
err:
	return ctl;
}

// 窗口初始化
// 在层级管理中注册一个窗口
struct SHEET *sheet_alloc(struct SHTCTL *ctl)
{
	struct SHEET *sht;
	int i;
	for (i = 0; i < MAX_SHEETS; i++) {
		if (ctl->sheets0[i].flags == 0) {
			sht = &ctl->sheets0[i];
			// 标记为正在使用
			sht->flags = SHEET_USE;
			// 隐藏
			sht->height = -1;
			return sht;
		}
	}
	// 所有SHEET都处于正在使用状态
	return 0;
}

// 窗口信息设置
// 窗口结构体地址，起始地址，长，宽，透明颜色
void sheet_setbuf(struct SHEET *sht, unsigned char *buf, int xsize, int ysize, int col_inv)
{
	sht->buf     = buf;
	sht->bxsize  = xsize;
	sht->bysize  = ysize;
	sht->col_inv = col_inv;
	return;
}

// 局部画面刷新
// V0 整个显存重新赋值实现刷新（处理量太大，系统运行卡顿）
// V1 通过缓冲区的相对坐标位置内显存的刷新（处理量虽然减小，但是用了 if 语句，还是会所有的显存进行判断，还是没有达到理想效果）
// V2 通过限定for语句的范围减少 if 判断
void sheet_refreshsub(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1)
{
	int h, bx, by, vx, vy, bx0, by0, bx1, by1;
	unsigned char *buf, c, *vram = ctl->vram;
	struct SHEET *sht;
	for (h = 0; h <= ctl->top; h++) {
		sht = ctl->sheets[h];
		buf = sht->buf;
		// 使用 vx0 ~ vy1，对 bx0 ~ by1 进行倒推
		// vx = sht->vx0 + bx;
		bx0 = vx0 - sht->vx0;
		by0 = vy0 - sht->vy0;
		bx1 = vx1 - sht->vx0;
		by1 = vy1 - sht->vy0;
		if (bx0 < 0) bx0 = 0;
		if (by0 < 0) by0 = 0;
		if (bx1 > sht->bxsize) bx1 = sht->bxsize;
		if (by1 > sht->bysize) by1 = sht->bysize;
		// for 语句在 bx0 ~ bx1 之间循环
		for (by = 0; by < by1; by++) {
			vy = sht->vy0 + by;
			for (bx = 0; bx < bx1; bx++) {
				vx = sht->vx0 + bx;
				c = buf[by * sht->bxsize + bx];
				if (c != sht->col_inv) vram[vy * ctl->xsize + vx] = c;
			}
		}
	}
}

// 画面刷新
void sheet_refresh(struct SHTCTL *ctl, struct SHEET *sht, int bx0, int by0, int bx1, int by1)
{
	if (sht->height >= 0) sheet_refreshsub(ctl, sht->vx0 + bx0, sht->vy0 + by0, sht->vx0 + bx1, sht->vy0 + by1);
}

/*
 * 叠加处理，窗口显示变化逻辑
 * sheet_updown(struct SHTCTL *ctl, struct SHEET *sht, int height)
 * ctl 窗口管理结构体，sht 更改的窗口，height 更改的高度
 * sht->height: 为窗口的高度
 * 先储存之前窗口高度的值，为 h, old
 * 如果 height 对于 ctl->top 过高，则设置 height 为 ctl->top 最高的窗口
 * 如果 height 过低，则设置为隐藏状态 height = -1
 * 将窗口的层级高度赋值 sht->height
 * 如果 old > height 比之前低，然后再判断 height 为显示还是 = -1 隐藏，做相应的处理
 * 如果 old < height 则是将窗口的层级高度向前移动，其他窗口做相应的处理
 * */
void sheet_updown(struct SHTCTL *ctl, struct SHEET *sht, int height)
{
	// 储存设置前的高度信息
	int h, old = sht->height;
	// 如果高度过高或或低，进行修正
	if (height > ctl->top + 1) height = ctl->top + 1;
	if (height < -1) height = -1;
	
	// 设定高度
	sht->height = height;

	// 下面主要是进行sheets[] 的重新排列
	// 比以前低
	if (old > height) {
		if (height >= 0) {
			// 把中间的往上提
			for (h = old; h > height; h--) {
				ctl->sheets[h] = ctl->sheets[h -1];
				ctl->sheets[h]->height = h;
			}
			ctl->sheets[height] = sht;
		}
		// 隐藏
		else {
			if (ctl->top > old) {
				// 把上面的降下来
				for (h = old; h < ctl->top; h++) {
					ctl->sheets[h] = ctl->sheets[h + 1];
					ctl->sheets[h]->height = h;
				}
			}
			// 由于显示中的图层减少了一个，所以最上面的图层高度下降
			ctl->top--;
		}
		// 按照新图层重新绘制画面
		sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize);
	}
	// 比以前高
	else if (old < height) {
		if (old >= 0) {
			// 把中间的拉下去
			for (h = old; h < height; h++) {
				ctl->sheets[h] = ctl->sheets[h + 1];
				ctl->sheets[h]->height = h;
			}
			ctl->sheets[height] = sht;
		}
		else {
			// 由隐藏状态变为显示状态
			// 将已在上面的提上来
			for (h = ctl->top; h >= height; h--) {
				ctl->sheets[h + 1] = ctl->sheets[h];
				ctl->sheets[h + 1]->height = h + 1;
			}
			ctl->sheets[height] = sht;
			// 由于已显示的图层增加了一个，所以最上面的图层高度增加
			ctl->top++;
		}
		sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize);
	}
	return;
}

// 移动图层
void sheet_slide(struct SHTCTL *ctl, struct SHEET *sht, int vx0, int vy0)
{
	// 先保存之前的位置
	int old_vx0 = sht->vx0, old_vy0 = sht->vy0;
	// 设定新位置
	sht->vx0 = vx0;
	sht->vy0 = vy0;
	// 如果正在显示，则按新图层的信息刷新画面
	if (sht->height >= 0) {
		sheet_refreshsub(ctl, old_vx0, old_vy0, old_vx0 + sht->bxsize, old_vy0 + sht->bysize);
		sheet_refreshsub(ctl, vx0, vy0, vx0 + sht->bxsize, vy0 + sht->bysize);
	}
}

// 释放使用的图层
void sheet_free(struct SHTCTL *ctl, struct SHEET *sht)
{
	// 如果处于显示状态，先设置隐藏
	if (sht->height >= 0) sheet_updown(ctl, sht, -1);
	// 设置窗口为未使用标志
	sht->flags = 0;
}
