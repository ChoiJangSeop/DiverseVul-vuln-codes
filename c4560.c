push_symbol(Str str, char symbol, int width, int n)
{
    char buf[2], *p;
    int i;

#ifdef USE_M17N
    if (width == 2)
	p = alt2_symbol[(int)symbol];
    else
#endif
	p = alt_symbol[(int)symbol];
    for (i = 0; i < 2 && *p; i++, p++)
	buf[i] = (*p == ' ') ? NBSP_CODE : *p;

    Strcat(str, Sprintf("<_SYMBOL TYPE=%d>", symbol));
    for (; n > 0; n--)
	Strcat_charp_n(str, buf, i);
    Strcat_charp(str, "</_SYMBOL>");
}