PJ_DEF(pj_status_t) pjmedia_rtp_decode_rtp2(
					    pjmedia_rtp_session *ses,
					    const void *pkt, int pkt_len,
					    const pjmedia_rtp_hdr **hdr,
					    pjmedia_rtp_dec_hdr *dec_hdr,
					    const void **payload,
					    unsigned *payloadlen)
{
    int offset;

    PJ_UNUSED_ARG(ses);

    /* Assume RTP header at the start of packet. We'll verify this later. */
    *hdr = (pjmedia_rtp_hdr*)pkt;

    /* Check RTP header sanity. */
    if ((*hdr)->v != RTP_VERSION) {
	return PJMEDIA_RTP_EINVER;
    }

    /* Payload is located right after header plus CSRC */
    offset = sizeof(pjmedia_rtp_hdr) + ((*hdr)->cc * sizeof(pj_uint32_t));

    /* Decode RTP extension. */
    if ((*hdr)->x) {
        dec_hdr->ext_hdr = (pjmedia_rtp_ext_hdr*)(((pj_uint8_t*)pkt) + offset);
        dec_hdr->ext = (pj_uint32_t*)(dec_hdr->ext_hdr + 1);
        dec_hdr->ext_len = pj_ntohs((dec_hdr->ext_hdr)->length);
        offset += ((dec_hdr->ext_len + 1) * sizeof(pj_uint32_t));
    } else {
	dec_hdr->ext_hdr = NULL;
	dec_hdr->ext = NULL;
	dec_hdr->ext_len = 0;
    }

    /* Check that offset is less than packet size */
    if (offset > pkt_len)
	return PJMEDIA_RTP_EINLEN;

    /* Find and set payload. */
    *payload = ((pj_uint8_t*)pkt) + offset;
    *payloadlen = pkt_len - offset;
 
    /* Remove payload padding if any */
    if ((*hdr)->p && *payloadlen > 0) {
	pj_uint8_t pad_len;

	pad_len = ((pj_uint8_t*)(*payload))[*payloadlen - 1];
	if (pad_len <= *payloadlen)
	    *payloadlen -= pad_len;
    }

    return PJ_SUCCESS;
}