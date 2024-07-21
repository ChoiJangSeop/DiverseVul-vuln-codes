PJ_DEF(pj_status_t) pjmedia_rtcp_fb_build_rpsi(
					    pjmedia_rtcp_session *session, 
					    void *buf,
					    pj_size_t *length,
					    const pjmedia_rtcp_fb_rpsi *rpsi)
{
    pjmedia_rtcp_common *hdr;
    pj_uint8_t *p;
    unsigned bitlen, padlen, len;

    PJ_ASSERT_RETURN(session && buf && length && rpsi, PJ_EINVAL);

    bitlen = (unsigned)rpsi->rpsi_bit_len + 16;
    padlen = (32 - (bitlen % 32)) % 32;
    len = (3 + (bitlen+padlen)/32) * 4;
    if (len > *length)
	return PJ_ETOOSMALL;

    /* Build RTCP-FB RPSI header */
    hdr = (pjmedia_rtcp_common*)buf;
    pj_memcpy(hdr, &session->rtcp_rr_pkt.common,  sizeof(*hdr));
    hdr->pt = RTCP_PSFB;
    hdr->count = 3; /* FMT = 3 */
    hdr->length = pj_htons((pj_uint16_t)(len/4 - 1));

    /* Build RTCP-FB RPSI FCI */
    p = (pj_uint8_t*)hdr + sizeof(*hdr);
    /* PB (number of padding bits) */
    *p++ = (pj_uint8_t)padlen;
    /* Payload type */
    *p++ = rpsi->pt & 0x7F;
    /* RPSI bit string */
    pj_memcpy(p, rpsi->rpsi.ptr, rpsi->rpsi_bit_len/8);
    p += rpsi->rpsi_bit_len/8;
    if (rpsi->rpsi_bit_len % 8) {
	*p++ = *(rpsi->rpsi.ptr + rpsi->rpsi_bit_len/8);
    }
    /* Zero padding */
    if (padlen >= 8)
	pj_bzero(p, padlen/8);

    /* Finally */
    *length = len;

    return PJ_SUCCESS;
}