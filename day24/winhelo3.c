int api_openwin(char *buf, int sxiz, int yxiz, int col_inv, char *title);
void api_putstrwin(int win, int x, int y, int col, int len, char *str);
void api_boxfilwin(int win, int x0, int y0, int x1, int y1, int col);
void api_initmalloc();
char *api_malloc(int size);
void api_free(char *addr, int size);
void api_end();

void HariMain()
{
	int win;
	char *buf;
	api_initmalloc();
	buf = api_malloc(150 * 50);
	win = api_openwin(buf, 150, 50, -1, "Hello3");
	api_boxfilwin(win, 8, 36, 141, 43, 6);// 浅蓝色
	api_putstrwin(win, 28, 28, 0, 12, "Hello, world");
	api_end();
}
