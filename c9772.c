hex_encode (const unsigned char *data,
            size_t n_data)
{
	char *result;
	size_t i;
	size_t o;

	result = malloc (n_data * 3 + 1);
	if (result == NULL)
		return NULL;

	for (i = 0, o = 0; i < n_data; i++) {
		if (i > 0)
			result[o++] = ':';
		result[o++] = HEXC_LOWER[data[i] >> 4 & 0xf];
		result[o++] = HEXC_LOWER[data[i] & 0xf];
	}

	result[o] = 0;
	return result;
}