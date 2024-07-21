int secure_decrypt(void *data, unsigned int data_length, int is_signed)
{
	at91_aes_key_size_t key_size;
	unsigned int cmac_key[8], cipher_key[8];
	unsigned int iv[AT91_AES_IV_SIZE_WORD];
	unsigned int computed_cmac[AT91_AES_BLOCK_SIZE_WORD];
	unsigned int fixed_length;
	const unsigned int *cmac;
	int rc = -1;

	/* Init keys */
	init_keys(&key_size, cipher_key, cmac_key, iv);

	/* Init periph */
	at91_aes_init();

	/* Check signature if required */
	if (is_signed) {
		/* Compute the CMAC */
		if (at91_aes_cmac(data_length, data, computed_cmac,
				  key_size, cmac_key))
			goto exit;

		/* Check the CMAC */
		fixed_length = at91_aes_roundup(data_length);
		cmac = (const unsigned int *)((char *)data + fixed_length);
		if (memcmp(cmac, computed_cmac, AT91_AES_BLOCK_SIZE_BYTE))
			goto exit;
	}

	/* Decrypt the whole file */
	if (at91_aes_cbc(data_length, data, data, 0,
			 key_size, cipher_key, iv))
		goto exit;

	rc = 0;
exit:
	/* Reset periph */
	at91_aes_cleanup();

	/* Reset keys */
	memset(cmac_key, 0, sizeof(cmac_key));
	memset(cipher_key, 0, sizeof(cipher_key));
	memset(iv, 0, sizeof(iv));

	return rc;
}