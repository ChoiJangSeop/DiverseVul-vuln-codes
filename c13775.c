static int piv_general_external_authenticate(sc_card_t *card,
		unsigned int key_ref, unsigned int alg_id)
{
	int r;
#ifdef ENABLE_OPENSSL
	int tmplen;
	int outlen;
	int locked = 0;
	u8 *p;
	u8 rbuf[4096];
	u8 *key = NULL;
	u8 *cypher_text = NULL;
	u8 *output_buf = NULL;
	const u8 *body = NULL;
	const u8 *challenge_data = NULL;
	size_t body_len;
	size_t output_len;
	size_t challenge_len;
	size_t keylen = 0;
	size_t cypher_text_len = 0;
	u8 sbuf[255];
	EVP_CIPHER_CTX * ctx = NULL;
	const EVP_CIPHER *cipher;

	SC_FUNC_CALLED(card->ctx, SC_LOG_DEBUG_VERBOSE);

	ctx = EVP_CIPHER_CTX_new();
	if (ctx == NULL) {
	    r = SC_ERROR_OUT_OF_MEMORY;
	    goto err;
	}

	sc_debug(card->ctx, SC_LOG_DEBUG_VERBOSE, "Selected cipher for algorithm id: %02x\n", alg_id);

	cipher = get_cipher_for_algo(alg_id);
	if(!cipher) {
		sc_debug(card->ctx, SC_LOG_DEBUG_VERBOSE, "Invalid cipher selector, none found for:  %02x\n", alg_id);
		r = SC_ERROR_INVALID_ARGUMENTS;
		goto err;
	}

	r = piv_get_key(card, alg_id, &key, &keylen);
	if (r) {
		sc_debug(card->ctx, SC_LOG_DEBUG_VERBOSE, "Error getting General Auth key\n");
		goto err;
	}

	r = sc_lock(card);
	if (r != SC_SUCCESS) {
		sc_debug(card->ctx, SC_LOG_DEBUG_VERBOSE, "sc_lock failed\n");
		goto err; /* cleanup */
	}
	locked = 1;

	p = sbuf;
	*p++ = 0x7C;
	*p++ = 0x02;
	*p++ = 0x81;
	*p++ = 0x00;

	/* get a challenge */
	r = piv_general_io(card, 0x87, alg_id, key_ref, sbuf, p - sbuf, rbuf, sizeof rbuf);
	if (r < 0) {
		sc_debug(card->ctx, SC_LOG_DEBUG_VERBOSE, "Error getting Challenge\n");
		goto err;
	}

	/*
	 * the value here corresponds with the response size, so we use this
	 * to alloc the response buffer, rather than re-computing it.
	 */
	output_len = r;

	/* Remove the encompassing outer TLV of 0x7C and get the data */
	body = sc_asn1_find_tag(card->ctx, rbuf,
		r, 0x7C, &body_len);
	if (!body) {
		sc_debug(card->ctx, SC_LOG_DEBUG_VERBOSE, "Invalid Challenge Data response of NULL\n");
		r =  SC_ERROR_INVALID_DATA;
		goto err;
	}

	/* Get the challenge data indicated by the TAG 0x81 */
	challenge_data = sc_asn1_find_tag(card->ctx, body,
		body_len, 0x81, &challenge_len);
	if (!challenge_data) {
		sc_debug(card->ctx, SC_LOG_DEBUG_VERBOSE, "Invalid Challenge Data none found in TLV\n");
		r =  SC_ERROR_INVALID_DATA;
		goto err;
	}

	/* Store this to sanity check that plaintext length and cyphertext lengths match */
	/* TODO is this required */
	tmplen = challenge_len;

	/* Encrypt the challenge with the secret */
	if (!EVP_EncryptInit(ctx, cipher, key, NULL)) {
		sc_debug(card->ctx, SC_LOG_DEBUG_VERBOSE, "Encrypt fail\n");
		r = SC_ERROR_INTERNAL;
		goto err;
	}

	cypher_text = malloc(challenge_len);
	if (!cypher_text) {
		sc_debug(card->ctx, SC_LOG_DEBUG_VERBOSE, "Could not allocate buffer for cipher text\n");
		r = SC_ERROR_INTERNAL;
		goto err;
	}

	EVP_CIPHER_CTX_set_padding(ctx,0);
	if (!EVP_EncryptUpdate(ctx, cypher_text, &outlen, challenge_data, challenge_len)) {
		sc_debug(card->ctx, SC_LOG_DEBUG_VERBOSE, "Encrypt update fail\n");
		r = SC_ERROR_INTERNAL;
		goto err;
	}
	cypher_text_len += outlen;

	if (!EVP_EncryptFinal(ctx, cypher_text + cypher_text_len, &outlen)) {
		sc_debug(card->ctx, SC_LOG_DEBUG_VERBOSE, "Final fail\n");
		r = SC_ERROR_INTERNAL;
		goto err;
	}
	cypher_text_len += outlen;

	/*
	 * Actually perform the sanity check on lengths plaintext length vs
	 * encrypted length
	 */
	if (cypher_text_len != (size_t)tmplen) {
		sc_debug(card->ctx, SC_LOG_DEBUG_VERBOSE, "Length test fail\n");
		r = SC_ERROR_INTERNAL;
		goto err;
	}

	output_buf = malloc(output_len);
	if(!output_buf) {
		sc_debug(card->ctx, SC_LOG_DEBUG_VERBOSE, "Could not allocate output buffer: %s\n",
				strerror(errno));
		r = SC_ERROR_INTERNAL;
		goto err;
	}

	p = output_buf;

	/*
	 * Build: 7C<len>[82<len><challenge>]
	 * Start off by capturing the data of the response:
	 *     - 82<len><encrypted challenege response>
	 * Build the outside TLV (7C)
	 * Advance past that tag + len
	 * Build the body (82)
	 * memcopy the body past the 7C<len> portion
	 * Transmit
	 */
	tmplen = sc_asn1_put_tag(0x82, NULL, cypher_text_len, NULL, 0, NULL);
	if (tmplen <= 0) {
		r = SC_ERROR_INTERNAL;
		goto err;
	}

	r = sc_asn1_put_tag(0x7C, NULL, tmplen, p, output_len, &p);
	if (r != SC_SUCCESS) {
		goto err;
	}

	/* Build the 0x82 TLV and append to the 7C<len> tag */
	r = sc_asn1_put_tag(0x82, cypher_text, cypher_text_len, p, output_len - (p - output_buf), &p);
	if (r != SC_SUCCESS) {
		goto err;
	}

	/* Sanity check the lengths again */
	tmplen = sc_asn1_put_tag(0x7C, NULL, tmplen, NULL, 0, NULL)
		+ sc_asn1_put_tag(0x82, NULL, cypher_text_len, NULL, 0, NULL);
	if (output_len != (size_t)tmplen) {
		sc_debug(card->ctx, SC_LOG_DEBUG_VERBOSE, "Allocated and computed lengths do not match! "
			 "Expected %"SC_FORMAT_LEN_SIZE_T"d, found: %d\n", output_len, tmplen);
		r = SC_ERROR_INTERNAL;
		goto err;
	}

	r = piv_general_io(card, 0x87, alg_id, key_ref, output_buf, output_len, NULL, 0);
	sc_debug(card->ctx, SC_LOG_DEBUG_VERBOSE, "Got response  challenge\n");

err:
	if (ctx)
		EVP_CIPHER_CTX_free(ctx);

	if (locked)
		sc_unlock(card);

	if (key) {
		sc_mem_clear(key, keylen);
		free(key);
	}

	if (cypher_text)
		free(cypher_text);

	if (output_buf)
		free(output_buf);
#else
	sc_log(card->ctx, "OpenSSL Required");
	r = SC_ERROR_NOT_SUPPORTED;
#endif /* ENABLE_OPENSSL */

	LOG_FUNC_RETURN(card->ctx, r);
}