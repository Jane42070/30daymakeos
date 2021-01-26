#include <stdio.h>

#include "../apilib.h"

void HariMain()
{
	char *buf, s[12] = {0};
	int win, timer, sec = 0, min = 0, hour = 0;
	api_initmalloc();
	buf = api_malloc(150 * 50);
	win = api_openwin(buf, 150, 50, -1, "Noodle");
	timer = api_alloctimer();
	api_inittimer(timer, 128);
	for (;;) {
		sprintf(s, "%5d:%02d:%02d", hour, min, sec);
		api_boxfilwin(win, 28, 27, 115, 41, 7);
		api_putstrwin(win, 28, 27, 0, 11, s);

		api_settimer(timer, 100);
		if (api_getkey(1) != 128) break;
		sec++;
		if (sec == 60) {
			sec = 0;
			min++;
			if (min == 60) {
				min = 0;
				hour++;
			}
		}
	}
	api_end();
}
