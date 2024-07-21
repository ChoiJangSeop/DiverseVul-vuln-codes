PJ_DEF(pj_status_t) pjmedia_rtcp_fb_build_nack(
					pjmedia_rtcp_session *session,
					void *buf,
					pj_size_t *length,
					unsigned nack_cnt,
					const pjmedia_rtcp_fb_nack nack[])
{
    pjmedia_rtcp_common *hdr;
    pj_uint8_t *p;
    unsigned len, i;

    PJ_ASSERT_RETURN(session && buf && length && nack_cnt && nack, PJ_EINVAL);

    len = (3 + nack_cnt) * 4;
    if (len > *length)
	return PJ_ETOOSMALL;

    /* Build RTCP-FB NACK header */
    hdr = (pjmedia_rtcp_common*)buf;
    pj_memcpy(hdr, &session->rtcp_rr_pkt.common,  sizeof(*hdr));
    hdr->pt = RTCP_RTPFB;
    hdr->count = 1; /* FMT = 1 */
    hdr->length = pj_htons((pj_uint16_t)(len/4 - 1));

    /* Build RTCP-FB NACK FCI */
    p = (pj_uint8_t*)hdr + sizeof(*hdr);
    for (i = 0; i < nack_cnt; ++i) {
	pj_uint16_t val;
	val = pj_htons((pj_uint16_t)nack[i].pid);
	pj_memcpy(p, &val, 2);
	val = pj_htons(nack[i].blp);
	pj_memcpy(p+2, &val, 2);
	p += 4;
    }

    /* Finally */
    *length = len;

    return PJ_SUCCESS;
}