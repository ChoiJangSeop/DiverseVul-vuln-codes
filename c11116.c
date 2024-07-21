PJ_DEF(void) pjmedia_rtcp_rx_rtcp( pjmedia_rtcp_session *sess,
				   const void *pkt,
				   pj_size_t size)
{
    pj_uint8_t *p, *p_end;

    p = (pj_uint8_t*)pkt;
    p_end = p + size;
    while (p < p_end) {
	pjmedia_rtcp_common *common = (pjmedia_rtcp_common*)p;
	unsigned len;

	len = (pj_ntohs((pj_uint16_t)common->length)+1) * 4;
	if (p + len > p_end)
	    break;

	switch(common->pt) {
	case RTCP_SR:
	case RTCP_RR:
	case RTCP_XR:
	    parse_rtcp_report(sess, p, len);
	    break;
	case RTCP_SDES:
	    parse_rtcp_sdes(sess, p, len);
	    break;
	case RTCP_BYE:
	    parse_rtcp_bye(sess, p, len);
	    break;
	case RTCP_RTPFB:
	case RTCP_PSFB:
	    parse_rtcp_fb(sess, p, len);
	    break;
	default:
	    /* Ignore unknown RTCP */
	    TRACE_((sess->name, "Received unknown RTCP packet type=%d",
		    common->pt));
	    break;
	}

	p += len;
    }
}