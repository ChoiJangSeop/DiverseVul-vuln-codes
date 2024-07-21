append_command(char_u *cmd)
{
    char_u *s = cmd;
    char_u *d;

    STRCAT(IObuff, ": ");
    d = IObuff + STRLEN(IObuff);
    while (*s != NUL && d - IObuff < IOSIZE - 7)
    {
	if (enc_utf8 ? (s[0] == 0xc2 && s[1] == 0xa0) : *s == 0xa0)
	{
	    s += enc_utf8 ? 2 : 1;
	    STRCPY(d, "<a0>");
	    d += 4;
	}
	else
	    MB_COPY_CHAR(s, d);
    }
    *d = NUL;
}