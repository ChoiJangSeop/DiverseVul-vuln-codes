PJ_DEF(pj_status_t) pjmedia_rtcp_fb_build_pli(
					pjmedia_rtcp_session *session, 
					void *buf,
					pj_size_t *length)
{
    pjmedia_rtcp_common *hdr;
    unsigned len;

    PJ_ASSERT_RETURN(session && buf && length, PJ_EINVAL);

    len = 12;
    if (len > *length)
	return PJ_ETOOSMALL;

    /* Build RTCP-FB PLI header */
    hdr = (pjmedia_rtcp_common*)buf;
    pj_memcpy(hdr, &session->rtcp_rr_pkt.common,  sizeof(*hdr));
    hdr->pt = RTCP_PSFB;
    hdr->count = 1; /* FMT = 1 */
    hdr->length = pj_htons((pj_uint16_t)(len/4 - 1));

    /* Finally */
    *length = len;

    return PJ_SUCCESS;
}