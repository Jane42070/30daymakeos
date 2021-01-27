#include "../apilib.h"

void HariMain()
{
	int fh;
	char c;
	char cmdline[30], *p;
	api_cmdline(cmdline, 30);
	// 跳过之前的内容，直到遇到空格
	for (p = cmdline; *p > ' '; p++) {  }
	// 跳过空格
	for (; *p == ' '; p++) {  }
	fh = api_fopen(p);
	if (fh != 0) {
		for (;;) {
			if (api_fread(&c, 1, fh) == 0) break;
			api_putchar(c);
		}
	} else {
		api_putstr("File not found.");
	}
	api_end();
}
