static int piv_validate_general_authentication(sc_card_t *card,
					const u8 * data, size_t datalen,
					u8 * out, size_t outlen)
{
	piv_private_data_t * priv = PIV_DATA(card);
	int r, tmplen, tmplen2;
	u8 *p;
	const u8 *tag;
	size_t taglen;
	const u8 *body;
	size_t bodylen;
	unsigned int real_alg_id, op_tag;

	u8 sbuf[4096]; /* needs work. for 3072 keys, needs 384+10 or so */
	size_t sbuflen = sizeof(sbuf);
	u8 rbuf[4096];

	SC_FUNC_CALLED(card->ctx, SC_LOG_DEBUG_VERBOSE);

	/* should assume large send data */
	p = sbuf;
	tmplen = sc_asn1_put_tag(0xff, NULL, datalen, NULL, 0, NULL);
	tmplen2 = sc_asn1_put_tag(0x82, NULL, 0, NULL, 0, NULL);
	if (tmplen <= 0 || tmplen2 <= 0) {
		LOG_FUNC_RETURN(card->ctx, SC_ERROR_INTERNAL);
	}
	tmplen += tmplen2;
	if ((r = sc_asn1_put_tag(0x7c, NULL, tmplen, p, sbuflen, &p)) != SC_SUCCESS ||
	    (r = sc_asn1_put_tag(0x82, NULL, 0, p, sbuflen - (p - sbuf), &p)) != SC_SUCCESS) {
		LOG_FUNC_RETURN(card->ctx, r);
	}
	if (priv->operation == SC_SEC_OPERATION_DERIVE
			&& priv->algorithm == SC_ALGORITHM_EC) {
		op_tag = 0x85;
	} else {
		op_tag = 0x81;
	}
	r = sc_asn1_put_tag(op_tag, data, datalen, p, sbuflen - (p - sbuf), &p);
	if (r != SC_SUCCESS) {
		LOG_FUNC_RETURN(card->ctx, r);
	}

	/*
	 * alg_id=06 is a place holder for all RSA keys.
	 * Derive the real alg_id based on the size of the
	 * the data, as we are always using raw mode.
	 * Non RSA keys needs some work in this area.
	 */

	real_alg_id = priv->alg_id;
	if (priv->alg_id == 0x06) {
		switch  (datalen) {
			case 128: real_alg_id = 0x06; break;
			case 256: real_alg_id = 0x07; break;
			case 384: real_alg_id = 0x05; break;
			default:
				SC_FUNC_RETURN(card->ctx, SC_LOG_DEBUG_VERBOSE, SC_ERROR_NO_CARD_SUPPORT);
		}
	}
	/* EC alg_id was already set */

	r = piv_general_io(card, 0x87, real_alg_id, priv->key_ref,
			sbuf, p - sbuf, rbuf, sizeof rbuf);

	if (r >= 0) {
		body = sc_asn1_find_tag(card->ctx, rbuf, r, 0x7c, &bodylen);
		if (body) {
			tag = sc_asn1_find_tag(card->ctx, body,  bodylen, 0x82, &taglen);
			if (tag) {
				memcpy(out, tag, taglen);
				r = taglen;
			} else
				r = SC_ERROR_INVALID_DATA;
		} else
			r = SC_ERROR_INVALID_DATA;
	}

	LOG_FUNC_RETURN(card->ctx, r);
}