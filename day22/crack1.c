void api_end();
void api_putchar(int c);

void HariMain()
{
	*((char *) 0x00102600) = 1;
	int p = *((int *) 0x00102600);
	api_putchar(p);
	api_end();
}
