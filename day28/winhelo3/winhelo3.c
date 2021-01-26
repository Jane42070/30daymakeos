#include "../apilib.h"

void HariMain()
{
	int win;
	char *buf;
	api_initmalloc();
	buf = api_malloc(150 * 50);
	win = api_openwin(buf, 150, 50, -1, "Hello3");
	api_boxfilwin(win, 8, 36, 141, 43, 6);// 浅蓝色
	api_putstrwin(win, 28, 28, 0, 12, "Hello, world");
	api_end();
}
