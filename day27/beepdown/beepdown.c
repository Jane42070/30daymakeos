#include "../apilib.h"

void HariMain()
{
	int i, timer;
	timer = api_alloctimer();
	api_inittimer(timer, 128);
	for (i = 20000000; i >= 20000; i -= i / 100) {
		// 20KHz ~ 20Hz 即人类可以听到的声音
		// i 以 1% 的速度递减
		api_beep(i);
		api_settimer(timer, 1);		// 0.01 秒
		if (api_getkey(1) != 128) break;
	}
	api_beep(0);
	api_end();
}
