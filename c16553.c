int ASN1_GENERALIZEDTIME_print(BIO *bp, ASN1_GENERALIZEDTIME *tm)
	{
	char *v;
	int gmt=0;
	int i;
	int y=0,M=0,d=0,h=0,m=0,s=0;

	i=tm->length;
	v=(char *)tm->data;

	if (i < 12) goto err;
	if (v[i-1] == 'Z') gmt=1;
	for (i=0; i<12; i++)
		if ((v[i] > '9') || (v[i] < '0')) goto err;
	y= (v[0]-'0')*1000+(v[1]-'0')*100 + (v[2]-'0')*10+(v[3]-'0');
	M= (v[4]-'0')*10+(v[5]-'0');
	if ((M > 12) || (M < 1)) goto err;
	d= (v[6]-'0')*10+(v[7]-'0');
	h= (v[8]-'0')*10+(v[9]-'0');
	m=  (v[10]-'0')*10+(v[11]-'0');
	if (	(v[12] >= '0') && (v[12] <= '9') &&
		(v[13] >= '0') && (v[13] <= '9'))
		s=  (v[12]-'0')*10+(v[13]-'0');

	if (BIO_printf(bp,"%s %2d %02d:%02d:%02d %d%s",
		mon[M-1],d,h,m,s,y,(gmt)?" GMT":"") <= 0)
		return(0);
	else
		return(1);
err:
	BIO_write(bp,"Bad time value",14);
	return(0);
	}