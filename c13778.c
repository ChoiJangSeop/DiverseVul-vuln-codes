piv_compute_signature(sc_card_t *card, const u8 * data, size_t datalen,
		u8 * out, size_t outlen)
{
	piv_private_data_t * priv = PIV_DATA(card);
	int r;
	int i;
	size_t nLen;
	u8 rbuf[128]; /* For EC conversions  384 will fit */
	const u8 * body;
	size_t bodylen;
	const u8 * tag;
	size_t taglen;

	SC_FUNC_CALLED(card->ctx, SC_LOG_DEBUG_VERBOSE);

	/* The PIV returns a DER SEQUENCE{INTEGER, INTEGER}
	 * Which may have leading 00 to force positive
	 * TODO: -DEE should check if PKCS15 want the same
	 * But PKCS11 just wants 2* filed_length in bytes
	 * So we have to strip out the integers
	 * if present and pad on left if too short.
	 */

	if (priv->alg_id == 0x11 || priv->alg_id == 0x14 ) {
		nLen = (priv->key_size + 7) / 8;
		if (outlen < 2*nLen) {
			sc_log(card->ctx,
			       " output too small for EC signature %"SC_FORMAT_LEN_SIZE_T"u < %"SC_FORMAT_LEN_SIZE_T"u",
			       outlen, 2 * nLen);
			r = SC_ERROR_INVALID_DATA;
			goto err;
		}
		memset(out, 0, outlen);

		r = piv_validate_general_authentication(card, data, datalen, rbuf, sizeof rbuf);
		if (r < 0)
			goto err;

		body = sc_asn1_find_tag(card->ctx, rbuf, r, 0x30, &bodylen);

		for (i = 0; i<2; i++) {
			if (body) {
				tag = sc_asn1_find_tag(card->ctx, body,  bodylen, 0x02, &taglen);
				if (tag) {
					bodylen -= taglen - (tag - body);
					body = tag + taglen;

					if (taglen > nLen) { /* drop leading 00 if present */
						if (*tag != 0x00) {
							r = SC_ERROR_INVALID_DATA;
							goto err;
						}
						tag++;
						taglen--;
					}
					memcpy(out + nLen*i + nLen - taglen , tag, taglen);
				} else {
					r = SC_ERROR_INVALID_DATA;
					goto err;
				}
			} else  {
				r = SC_ERROR_INVALID_DATA;
				goto err;
			}
		}
		r = 2 * nLen;
	} else { /* RSA is all set */
		r = piv_validate_general_authentication(card, data, datalen, out, outlen);
	}

err:
	SC_FUNC_RETURN(card->ctx, SC_LOG_DEBUG_VERBOSE, r);
}