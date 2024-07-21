int secure_check(void *data)
{
	const at91_secure_header_t *header;
	void *file;

	if (secure_decrypt(data, sizeof(*header), 0))
		return -1;

	header = (const at91_secure_header_t *)data;
	if (header->magic != AT91_SECURE_MAGIC)
		return -1;

	file = (unsigned char *)data + sizeof(*header);
	return secure_decrypt(file, header->file_size, 1);
}