static int cac_read_binary(sc_card_t *card, unsigned int idx,
		unsigned char *buf, size_t count, unsigned long flags)
{
	cac_private_data_t * priv = CAC_DATA(card);
	int r = 0;
	u8 *val = NULL;
	u8 *cert_ptr;
	size_t val_len;
	size_t len, cert_len;
	u8 cert_type;

	SC_FUNC_CALLED(card->ctx, SC_LOG_DEBUG_VERBOSE);

	/* if we didn't return it all last time, return the remainder */
	if (priv->cached) {
		sc_log(card->ctx, 
			"returning cached value idx=%d count=%"SC_FORMAT_LEN_SIZE_T"u",
			idx, count);
		if (idx > priv->cache_buf_len) {
			LOG_FUNC_RETURN(card->ctx, SC_ERROR_FILE_END_REACHED);
		}
		len = MIN(count, priv->cache_buf_len-idx);
		memcpy(buf, &priv->cache_buf[idx], len);
		LOG_FUNC_RETURN(card->ctx, len);
	}

	sc_log(card->ctx, 
		"clearing cache idx=%d count=%"SC_FORMAT_LEN_SIZE_T"u",
		idx, count);
	free(priv->cache_buf);
	priv->cache_buf = NULL;
	priv->cache_buf_len = 0;

	r = cac_cac1_get_certificate(card, &val, &val_len);
	if (r < 0)
		goto done;
	if (val_len < 1) {
		r = SC_ERROR_INVALID_DATA;
		goto done;
	}

	cert_type = val[0];
	cert_ptr = val + 1;
	cert_len = val_len - 1;

	/* if the info byte is 1, then the cert is compressed, decompress it */
	if ((cert_type & 0x3) == 1) {
#ifdef ENABLE_ZLIB
		r = sc_decompress_alloc(&priv->cache_buf, &priv->cache_buf_len,
			cert_ptr, cert_len, COMPRESSION_AUTO);
#else
		sc_log(card->ctx, "CAC compression not supported, no zlib");
		r = SC_ERROR_NOT_SUPPORTED;
#endif
		if (r)
			goto done;
	} else if (cert_len > 0) {
		priv->cache_buf = malloc(cert_len);
		if (priv->cache_buf == NULL) {
			r = SC_ERROR_OUT_OF_MEMORY;
			goto done;
		}
		priv->cache_buf_len = cert_len;
		memcpy(priv->cache_buf, cert_ptr, cert_len);
	}

	/* OK we've read the data, now copy the required portion out to the callers buffer */
	priv->cached = 1;
	len = MIN(count, priv->cache_buf_len-idx);
	if (len && priv->cache_buf)
		memcpy(buf, &priv->cache_buf[idx], len);
	r = len;
done:
	if (val)
		free(val);
	LOG_FUNC_RETURN(card->ctx, r);
}