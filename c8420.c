static int cac_cac1_get_certificate(sc_card_t *card, u8 **out_buf, size_t *out_len)
{
	u8 buf[CAC_MAX_SIZE];
	u8 *out_ptr;
	size_t size = 0;
	size_t left = 0;
	size_t len, next_len;
	sc_apdu_t apdu;
	int r = SC_SUCCESS;
	SC_FUNC_CALLED(card->ctx, SC_LOG_DEBUG_VERBOSE);
	/* get the size */
	size = left = *out_buf ? *out_len : sizeof(buf);
	out_ptr = *out_buf ? *out_buf : buf;
	sc_format_apdu(card, &apdu, SC_APDU_CASE_2_SHORT, CAC_INS_GET_CERTIFICATE, 0, 0 );
	next_len = MIN(left, 100);
	for (; left > 0; left -= len, out_ptr += len) {
		len = next_len;
		apdu.resp = out_ptr;
		apdu.le = len;
		apdu.resplen = left;
		r = sc_transmit_apdu(card, &apdu);
		if (r < 0) {
			break;
		}
		if (apdu.resplen == 0) {
			r = SC_ERROR_INTERNAL;
			break;
		}
		/* in the old CAC-1, 0x63 means 'more data' in addition to 'pin failed' */
		if (apdu.sw1 != 0x63 || apdu.sw2 < 1)  {
			/* we've either finished reading, or hit an error, break */
			r = sc_check_sw(card, apdu.sw1, apdu.sw2);
			left -= len;
			break;
		}
		next_len = MIN(left, apdu.sw2);
	}
	if (r < 0) {
		SC_FUNC_RETURN(card->ctx, SC_LOG_DEBUG_VERBOSE, r);
	}
	r = size - left;
	if (*out_buf == NULL) {
		*out_buf = malloc(r);
		if (*out_buf == NULL) {
			SC_FUNC_RETURN(card->ctx, SC_LOG_DEBUG_VERBOSE,
				SC_ERROR_OUT_OF_MEMORY);
		}
		memcpy(*out_buf, buf, r);
	}
	*out_len = r;
	SC_FUNC_RETURN(card->ctx, SC_LOG_DEBUG_VERBOSE, r);
}