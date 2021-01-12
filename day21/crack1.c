void api_end();

void HariMain()
{
	*((char *) 0x00102600) = 0;
	api_end();
}
