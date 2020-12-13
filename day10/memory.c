#include "bootpack.h"

// 测试内存
unsigned int memtest(unsigned int start, unsigned int end)
{
	// CPU 架构 >= 486 才有高速缓存
	// 486CPU 的 EFLAGS寄存器 18 位存储着AC标志位
	// 386CPU 这位为 0
	char flg486 = 0;
	unsigned int eflg, cr0, i;

	/* 确认CPU是386还是486以上 */
	eflg = io_load_eflags();
	/* AC-bit = 1 */
	eflg |= EFLAGS_AC_BIT;
	io_store_eflags(eflg);
	eflg = io_load_eflags();
	// 如果是 386，即使设置AC=1，AC的值还是会回到 0
	if ((eflg & EFLAGS_AC_BIT) != 0) { flg486 = 1; }
	/* AC-bit = 1 */
	// 取反
	eflg &= ~EFLAGS_AC_BIT;
	io_store_eflags(eflg);

	// 禁止缓存
	// 对CR0寄存器操作
	if (flg486 != 0) {
		cr0 = load_cr0();
		cr0 |= CR0_CACHE_DISABLE;
		store_cr0(cr0);
	}

	i = memtest_sub(start, end);

	// 允许缓存
	if (flg486 != 0) {
		cr0 = load_cr0();
		cr0 &= ~CR0_CACHE_DISABLE;
		store_cr0(cr0);
	}

	return i;
}

// 初始化内存管理
void memman_init(struct MEMMAN *man)
{
	man->frees    = 0;	// 可用信息数目
	man->maxfrees = 0;	// 用于观察可用状况：frees的最大值
	man->lostsize = 0;	// 释放失败的内存大小的总和
	man->losts    = 0;	// 释放失败次数
}

// 报告总空余内存
unsigned int memman_total(struct MEMMAN *man)
{
	unsigned int i, t = 0;
	for (i = 0; i < man->frees; i++) t += man->free[i].size;
	return t;
}

// 分配内存
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size)
{
	unsigned int i, a;
	for (i = 0; i < man->frees; i++) {
		// 有足够大的内存
		if (man->free[i].size >= size) {
			a = man->free[i].addr;
			man->free[i].addr += size;
			man->free[i].size -= size;
			// free[i] 变为 0，就删掉一条可用信息
			if (man->free[i].size == 0) {
				man->frees--;
				for (; i < man->frees; i++) {
					// 往前移送
					man->free[i] = man->free[i + 1];
				}
			}
			return a;
		}
	}
	// 没有可用空间
	return 0;
}

// 释放内存
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size)
{
	int i, j;
	// 为便于归纳内存，将 free[] 按照 addr 的顺序排列
	// 所以，先决定放在哪里
	for (i = 0; i < man->frees; i++) {
		if (man->free[i].addr > addr) break;
	}
	// free[i - 1].addr < addr < free[i].addr
	if (i > 0) {
		// 前面可用内存
		if (man->free[i - 1].addr + man->free[i - 1].size == addr) {
			// 可以与前面的内存归纳到一起
			man->free[i - 1].size += size;
			if (i < man->frees) {
				// 后面也有
				if (addr + size == man->free[i].addr) {
					// 也可以与后面的归纳到一起
					man->free[i - 1].size += man->free[i].size;
					// man->free[i] 删除
					// free[i]变成0后归纳到前面去
					man->frees--;
					for (; i < man->frees; i++) {
						// 往前移送
						man->free[i] = man->free[i + 1];
					}
				}
			}
			// 成功完成
			return 0;
		}
	}
	// 无法与前面的可用空间归纳到一起
	if (i < man->frees) {
		// 后面还有
		if (addr + size == man->free[i].addr) {
			// 可以和后面的内容归纳到一起
			man->free[i].addr = addr;
			man->free[i].size += size;
			// 成功完成
			return 0;
		}
	}
	// 既不能与前面归纳到一起，也不能和后面归纳到一起
	if (man->frees < MEMMAN_FREES) {
		// free[i] 之后的，向后移动，腾出一点可用空间
		for (j = man->frees; j > i; j--) {
			man->free[j] = man->free[j - 1];
		}
		man->frees++;
		// 更新最大值
		if (man->maxfrees < man->frees) man->maxfrees = man->frees;
		man->free[i].addr = addr;
		man->free[i].size = size;
		// 成功完成
		return 0;
	}
	// 不能往后移动
	man->losts++;
	man->lostsize += size;
	// 失败
	return -1;
}

unsigned int memman_alloc_4k(struct MEMMAN *man, unsigned int size)
{
	unsigned int a;
	size = (size + 0xfff) & 0xfffff000;
	a = memman_alloc(man, size);
	return a;
}

unsigned int memman_free_4k(struct MEMMAN *man, unsigned int addr, unsigned int size)
{
	int i;
	size = (size + 0xfff) & 0xfffff000;
	i = memman_free(man, addr, size);
	return i;
}
