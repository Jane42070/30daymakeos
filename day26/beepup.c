void api_end();
int api_getkey(int mode);
int api_alloctimer();
void api_inittimer(int timer, int data);
void api_settimer(int timer, int time);
void api_beep(int tone);

void HariMain()
{
	int i, timer;
	timer = api_alloctimer();
	api_inittimer(timer, 128);
	for (i = 20000; i <= 20000000; i += i / 100) {
		// 20KHz ~ 20Hz 即人类可以听到的声音
		// i 以 1% 的速度递减
		api_beep(i);
		api_settimer(timer, 1);		// 0.01 秒
		if (api_getkey(1) != 128) break;
	}
	api_beep(0);
	api_end();
}
