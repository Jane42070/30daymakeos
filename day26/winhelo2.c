int api_openwin(char *buf, int sxiz, int yxiz, int col_inv, char *title);
void api_putstrwin(int win, int x, int y, int col, int len, char *str);
void api_boxfilwin(int win, int x0, int y0, int x1, int y1, int col);
void api_end();

char buf[150 * 50];

void HariMain()
{
	int win;
	win = api_openwin(buf, 150, 50, -1, "Hello2");
	api_boxfilwin(win, 8, 36, 141, 43, 3);// 黄色
	api_putstrwin(win, 28, 28, 0, 12, "Hello, world");
	api_end();
}
