#include <stdio.h>
#include "../apilib.h"

#define MAX 10000

void HariMain()
{
	char flag[MAX], s[8];
	int i, j;
	api_initmalloc();
	for (i = 2; i < MAX; i++) {
		if (flag[i] == 0) {
			sprintf(s, "%d ", i);
			api_putstr(s);
			for (j = i * 2; j < MAX; j += i) {
				flag[j] = 1;
			}
		}
	}
	api_end();
}
