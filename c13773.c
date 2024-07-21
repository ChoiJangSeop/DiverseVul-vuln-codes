static int piv_generate_key(sc_card_t *card,
		sc_cardctl_piv_genkey_info_t *keydata)
{
	int r;
	u8 rbuf[4096];
	u8 *p;
	const u8 *tag;
	u8 tagbuf[16];
	u8 outdata[3]; /* we could also add tag 81 for exponent */
	size_t taglen, i;
	size_t out_len;
	size_t in_len;
	unsigned int cla_out, tag_out;

	SC_FUNC_CALLED(card->ctx, SC_LOG_DEBUG_VERBOSE);

	keydata->exponent = 0;
	keydata->pubkey = NULL;
	keydata->pubkey_len = 0;
	keydata->ecparam = NULL; /* will show size as we only support 2 curves */
	keydata->ecparam_len = 0;
	keydata->ecpoint = NULL;
	keydata->ecpoint_len = 0;

	out_len = 3;
	outdata[0] = 0x80;
	outdata[1] = 0x01;
	outdata[2] = keydata->key_algid;
	switch (keydata->key_algid) {
		case 0x05: keydata->key_bits = 3072; break;
		case 0x06: keydata->key_bits = 1024; break;
		case 0x07: keydata->key_bits = 2048; break;
		/* TODO: - DEE For EC, also set the curve parameter as the OID */
		case 0x11: keydata->key_bits = 0;
			keydata->ecparam =0; /* we only support prime256v1 for 11 */
			keydata->ecparam_len =0;
			break;
		case 0x14: keydata->key_bits = 0;
			keydata->ecparam = 0; /* we only support secp384r1 */
			keydata->ecparam_len = 0;
			break;
		default:
			LOG_FUNC_RETURN(card->ctx, SC_ERROR_INVALID_ARGUMENTS);
	}

	p = tagbuf;

	r = sc_asn1_put_tag(0xAC, outdata, out_len, tagbuf, sizeof(tagbuf), &p);
	if (r != SC_SUCCESS) {
		sc_log(card->ctx, "Failed to encode ASN1 tag");
		goto err;
	}

	r = piv_general_io(card, 0x47, 0x00, keydata->key_num,
			tagbuf, p - tagbuf, rbuf, sizeof rbuf);

	if (r >= 0) {
		const u8 *cp;
		keydata->exponent = 0;

		/* expected tag is 7f49.  */
		/* we will whatever tag is present */

		cp = rbuf;
		in_len = r;

		r = sc_asn1_read_tag(&cp, in_len, &cla_out, &tag_out, &in_len);
		if (cp == NULL) {
			r = SC_ERROR_ASN1_OBJECT_NOT_FOUND;
		}
		if (r != SC_SUCCESS) {
			sc_log(card->ctx, "Tag buffer not found");
			goto err;
		}

		/* if RSA vs EC */
		if (keydata->key_bits > 0 ) {
			tag = sc_asn1_find_tag(card->ctx, cp, in_len, 0x82, &taglen);
			if (tag != NULL && taglen <= 4) {
				keydata->exponent = 0;
				for (i = 0; i < taglen;i++)
					keydata->exponent = (keydata->exponent<<8) + tag[i];
			}

			tag = sc_asn1_find_tag(card->ctx, cp, in_len, 0x81, &taglen);
			if (tag != NULL && taglen > 0) {
				keydata->pubkey = malloc(taglen);
				if (keydata->pubkey == NULL)
					LOG_FUNC_RETURN(card->ctx, SC_ERROR_OUT_OF_MEMORY);
				keydata->pubkey_len = taglen;
				memcpy (keydata->pubkey, tag, taglen);
			}
		}
		else { /* must be EC */
			tag = sc_asn1_find_tag(card->ctx, cp, in_len, 0x86, &taglen);
			if (tag != NULL && taglen > 0) {
				keydata->ecpoint = malloc(taglen);
				if (keydata->ecpoint == NULL)
					LOG_FUNC_RETURN(card->ctx, SC_ERROR_OUT_OF_MEMORY);
				keydata->ecpoint_len = taglen;
				memcpy (keydata->ecpoint, tag, taglen);
			}
		}

		/* TODO: -DEE Could add key to cache so could use engine to generate key,
		 * and sign req in single operation */
		r = 0;
	}

err:
	LOG_FUNC_RETURN(card->ctx, r);
}