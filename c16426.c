int ssl2_put_cipher_by_char(const SSL_CIPHER *c, unsigned char *p)
	{
	long l;

	if (p != NULL)
		{
		l=c->id;
		if ((l & 0xff000000) != 0x02000000) return(0);
		p[0]=((unsigned char)(l>>16L))&0xFF;
		p[1]=((unsigned char)(l>> 8L))&0xFF;
		p[2]=((unsigned char)(l     ))&0xFF;
		}
	return(3);
	}