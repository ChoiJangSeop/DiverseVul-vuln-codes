_XimCountNumberOfAttr(
    INT16	 total,
    CARD16	*attr,
    int		*names_len)
{
    unsigned int n;
    INT16	 len;
    INT16	 min_len = sizeof(CARD16)	/* sizeof attribute ID */
			 + sizeof(CARD16)	/* sizeof type of value */
			 + sizeof(INT16);	/* sizeof length of attribute */

    n = 0;
    *names_len = 0;
    while (total > min_len) {
	len = attr[2];
	*names_len += (len + 1);
	len += (min_len + XIM_PAD(len + 2));
	total -= len;
	attr = (CARD16 *)((char *)attr + len);
	n++;
    }
    return n;
}