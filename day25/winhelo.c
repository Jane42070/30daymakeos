int api_openwin(char *buf, int sxiz, int yxiz, int col_inv, char *title);
void api_end();

char buf[150 * 50];

void HariMain()
{
	int win;
	win = api_openwin(buf, 150, 50, -1, "Hello");
	api_end();
}
