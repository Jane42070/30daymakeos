#include "../apilib.h"

void HariMain()
{
	int win;
	char buf[150 * 50];
	win = api_openwin(buf, 150, 50, -1, "Hello");
	for (;;) {
		if (api_getkey(1) == 0x1c) break;
	}
	api_end();
}
