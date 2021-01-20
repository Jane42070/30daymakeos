int api_openwin(char *buf, int sxiz, int yxiz, int col_inv, char *title);
int api_getkey(int mode);
void api_end();

char buf[150 * 50];

void HariMain()
{
	int win;
	win = api_openwin(buf, 150, 50, -1, "Hello");
	api_getkey(1);
	api_end();
}
