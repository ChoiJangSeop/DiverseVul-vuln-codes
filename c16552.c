int ASN1_GENERALIZEDTIME_check(ASN1_GENERALIZEDTIME *d)
	{
	static int min[9]={ 0, 0, 1, 1, 0, 0, 0, 0, 0};
	static int max[9]={99, 99,12,31,23,59,59,12,59};
	char *a;
	int n,i,l,o;

	if (d->type != V_ASN1_GENERALIZEDTIME) return(0);
	l=d->length;
	a=(char *)d->data;
	o=0;
	/* GENERALIZEDTIME is similar to UTCTIME except the year is
         * represented as YYYY. This stuff treats everything as a two digit
         * field so make first two fields 00 to 99
         */
	if (l < 13) goto err;
	for (i=0; i<7; i++)
		{
		if ((i == 6) && ((a[o] == 'Z') ||
			(a[o] == '+') || (a[o] == '-')))
			{ i++; break; }
		if ((a[o] < '0') || (a[o] > '9')) goto err;
		n= a[o]-'0';
		if (++o > l) goto err;

		if ((a[o] < '0') || (a[o] > '9')) goto err;
		n=(n*10)+ a[o]-'0';
		if (++o > l) goto err;

		if ((n < min[i]) || (n > max[i])) goto err;
		}
	/* Optional fractional seconds: decimal point followed by one
	 * or more digits.
	 */
	if (a[o] == '.')
		{
		if (++o > l) goto err;
		i = o;
		while ((a[o] >= '0') && (a[o] <= '9') && (o <= l))
			o++;
		/* Must have at least one digit after decimal point */
		if (i == o) goto err;
		}

	if (a[o] == 'Z')
		o++;
	else if ((a[o] == '+') || (a[o] == '-'))
		{
		o++;
		if (o+4 > l) goto err;
		for (i=7; i<9; i++)
			{
			if ((a[o] < '0') || (a[o] > '9')) goto err;
			n= a[o]-'0';
			o++;
			if ((a[o] < '0') || (a[o] > '9')) goto err;
			n=(n*10)+ a[o]-'0';
			if ((n < min[i]) || (n > max[i])) goto err;
			o++;
			}
		}
	return(o == l);
err:
	return(0);
	}