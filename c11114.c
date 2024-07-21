PJ_DEF(pj_status_t) pjmedia_rtcp_fb_parse_pli(
					const void *buf,
					pj_size_t length)
{
    pjmedia_rtcp_common *hdr = (pjmedia_rtcp_common*) buf;

    PJ_ASSERT_RETURN(buf, PJ_EINVAL);
    PJ_ASSERT_RETURN(length >= 12, PJ_ETOOSMALL);

    /* PLI uses pt==RTCP_PSFB and FMT==1 */
    if (hdr->pt != RTCP_PSFB || hdr->count != 1)
	return PJ_ENOTFOUND;

    return PJ_SUCCESS;
}