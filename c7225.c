int snmp_version(void *context, size_t hdrlen, unsigned char tag,
		 const void *data, size_t datalen)
{
	if (*(unsigned char *)data > 1)
		return -ENOTSUPP;
	return 1;
}