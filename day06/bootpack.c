#include "bootpack.h"

void HariMain(void)
{
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
	extern char hankaku[4096];
	char *mcursor;
	int mx = (binfo->scrnx - 16) / 2; /* фад╩ */
	int my = (binfo->scrny - 28 - 16) / 2;
	unsigned char *s;

	init_palette();

	init_screen((unsigned char *)binfo->vram, binfo->scrnx, binfo->scrny);
	init_mouse_cursor8(mcursor, COL8_008484);
	putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);

	putfont8_pos(binfo->vram, binfo->scrnx, 0, 8, COL8_FFFFFF, (unsigned char *)"hello world!");
	putfont8_pos(binfo->vram, binfo->scrnx, 0, 40, COL8_FFFFFF, (unsigned char *)"CLAY");
	putfont8_pos(binfo->vram, binfo->scrnx, 0, 92, COL8_FFFFFF, (unsigned char *)"HARIBOTE.SYS");

	/** sprintf(s, "scrnx = { %d }", binfo->scrnx); */
	/** putfont8_str(binfo->vram, binfo->scrnx, 100, 66, COL8_FFFFFF, s); */
	for (;;) {
		io_hlt();
	}
}
