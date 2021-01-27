#include "../apilib.h"

void HariMain()
{
	int win;
	char buf[150 * 50];
	win = api_openwin(buf, 150, 50, -1, "Hello2");
	api_boxfilwin(win, 8, 36, 141, 43, 3);// 黄色
	api_putstrwin(win, 28, 28, 0, 12, "Hello, world");
	for (;;) {
		if (api_getkey(1) == 0x1c) break;
	}
	api_end();
}
