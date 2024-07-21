PJ_DEF(pj_status_t) pjmedia_rtcp_fb_parse_sli(
					const void *buf,
					pj_size_t length,
					unsigned *sli_cnt,
					pjmedia_rtcp_fb_sli sli[])
{
    pjmedia_rtcp_common *hdr = (pjmedia_rtcp_common*) buf;
    pj_uint8_t *p;
    unsigned cnt, i;

    PJ_ASSERT_RETURN(buf && sli_cnt && sli, PJ_EINVAL);
    PJ_ASSERT_RETURN(length >= sizeof(pjmedia_rtcp_common), PJ_ETOOSMALL);

    /* PLI uses pt==RTCP_PSFB and FMT==2 */
    if (hdr->pt != RTCP_PSFB || hdr->count != 2)
	return PJ_ENOTFOUND;

    cnt = pj_ntohs((pj_uint16_t)hdr->length) - 2;
    if (length < (cnt+3)*4)
	return PJ_ETOOSMALL;

    *sli_cnt = PJ_MIN(*sli_cnt, cnt);

    p = (pj_uint8_t*)hdr + sizeof(*hdr);
    for (i = 0; i < *sli_cnt; ++i) {
	/* 'first' takes 13 bit */
	sli[i].first = (p[0] << 5) + ((p[1] & 0xF8) >> 3);
	/* 'number' takes 13 bit */
	sli[i].number = ((p[1] & 0x07) << 10) +
			(p[2] << 2) +
			((p[3] & 0xC0) >> 6);
	/* 'pict_id' takes 6 bit */
	sli[i].pict_id = (p[3] & 0x3F);
	p += 4;
    }

    return PJ_SUCCESS;
}