PJ_DEF(pj_status_t) pjmedia_rtcp_fb_parse_nack(
					const void *buf,
					pj_size_t length,
					unsigned *nack_cnt,
					pjmedia_rtcp_fb_nack nack[])
{
    pjmedia_rtcp_common *hdr = (pjmedia_rtcp_common*) buf;
    pj_uint8_t *p;
    unsigned cnt, i;

    PJ_ASSERT_RETURN(buf && nack_cnt && nack, PJ_EINVAL);
    PJ_ASSERT_RETURN(length >= sizeof(pjmedia_rtcp_common), PJ_ETOOSMALL);

    /* Generic NACK uses pt==RTCP_RTPFB and FMT==1 */
    if (hdr->pt != RTCP_RTPFB || hdr->count != 1)
	return PJ_ENOTFOUND;

    cnt = pj_ntohs((pj_uint16_t)hdr->length) - 2;
    if (length < (cnt+3)*4)
	return PJ_ETOOSMALL;

    *nack_cnt = PJ_MIN(*nack_cnt, cnt);

    p = (pj_uint8_t*)hdr + sizeof(*hdr);
    for (i = 0; i < *nack_cnt; ++i) {
	pj_uint16_t val;

	pj_memcpy(&val, p, 2);
	nack[i].pid = pj_ntohs(val);
	pj_memcpy(&val, p+2, 2);
	nack[i].blp = pj_ntohs(val);
	p += 4;
    }

    return PJ_SUCCESS;
}