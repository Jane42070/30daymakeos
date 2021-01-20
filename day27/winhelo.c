#include "apilib.h"

char buf[150 * 50];

void HariMain()
{
	int win;
	win = api_openwin(buf, 150, 50, -1, "Hello");
	api_getkey(1);
	api_end();
}
