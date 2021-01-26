#include "../apilib.h"

char buf[150 * 50];

void HariMain()
{
	int win;
	win = api_openwin(buf, 150, 50, -1, "Hello");
	for (;;) {
		if (api_getkey(1) == 0x1c) break;
	}
	api_end();
}
