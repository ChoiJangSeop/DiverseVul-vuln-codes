PJ_DEF(pj_status_t) pjmedia_rtcp_fb_build_sli(
					pjmedia_rtcp_session *session, 
					void *buf,
					pj_size_t *length,
					unsigned sli_cnt,
					const pjmedia_rtcp_fb_sli sli[])
{
    pjmedia_rtcp_common *hdr;
    pj_uint8_t *p;
    unsigned len, i;

    PJ_ASSERT_RETURN(session && buf && length && sli_cnt && sli, PJ_EINVAL);

    len = (3 + sli_cnt) * 4;
    if (len > *length)
	return PJ_ETOOSMALL;

    /* Build RTCP-FB SLI header */
    hdr = (pjmedia_rtcp_common*)buf;
    pj_memcpy(hdr, &session->rtcp_rr_pkt.common,  sizeof(*hdr));
    hdr->pt = RTCP_PSFB;
    hdr->count = 2; /* FMT = 2 */
    hdr->length = pj_htons((pj_uint16_t)(len/4 - 1));

    /* Build RTCP-FB SLI FCI */
    p = (pj_uint8_t*)hdr + sizeof(*hdr);
    for (i = 0; i < sli_cnt; ++i) {
	/* 'first' takes 13 bit */
	*p++  = (pj_uint8_t)((sli[i].first >> 5) & 0xFF);   /* 8 MSB bits */
	*p    = (pj_uint8_t)((sli[i].first & 31) << 3);	    /* 5 LSB bits */
	/* 'number' takes 13 bit */
	*p++ |= (pj_uint8_t)((sli[i].number >> 10) & 7);    /* 3 MSB bits */
	*p++  = (pj_uint8_t)((sli[i].number >> 2) & 0xFF);  /* 8 mid bits */
	*p    = (pj_uint8_t)((sli[i].number & 3) << 6);	    /* 2 LSB bits */
	/* 'pict_id' takes 6 bit */
	*p++ |= (sli[i].pict_id & 63);
    }

    /* Finally */
    *length = len;

    return PJ_SUCCESS;
}