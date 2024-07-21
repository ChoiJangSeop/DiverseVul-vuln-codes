unsigned short atalk_checksum(struct ddpehdr *ddp, int len)
{
	unsigned long sum = 0;	/* Assume unsigned long is >16 bits */
	unsigned char *data = (unsigned char *)ddp;

	len  -= 4;		/* skip header 4 bytes */
	data += 4;

	/* This ought to be unwrapped neatly. I'll trust gcc for now */
	while (len--) {
		sum += *data;
		sum <<= 1;
		if (sum & 0x10000) {
			sum++;
			sum &= 0xFFFF;
		}
		data++;
	}
	/* Use 0xFFFF for 0. 0 itself means none */
	return sum ? htons((unsigned short)sum) : 0xFFFF;
}