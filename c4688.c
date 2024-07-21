read_attribute(cdk_stream_t inp, size_t pktlen, cdk_pkt_userid_t attr,
	       int name_size)
{
	const byte *p;
	byte *buf;
	size_t len, nread;
	cdk_error_t rc;

	if (!inp || !attr || !pktlen)
		return CDK_Inv_Value;

	if (DEBUG_PKT)
		_gnutls_write_log("read_attribute: %d octets\n",
				  (int) pktlen);

	_gnutls_str_cpy(attr->name, name_size, ATTRIBUTE);
	attr->len = MIN(name_size, sizeof(ATTRIBUTE) - 1);

	buf = cdk_calloc(1, pktlen);
	if (!buf)
		return CDK_Out_Of_Core;
	rc = stream_read(inp, buf, pktlen, &nread);
	if (rc) {
		cdk_free(buf);
		return CDK_Inv_Packet;
	}
	p = buf;
	len = *p++;
	pktlen--;
	if (len == 255) {
		len = _cdk_buftou32(p);
		p += 4;
		pktlen -= 4;
	} else if (len >= 192) {
		if (pktlen < 2) {
			cdk_free(buf);
			return CDK_Inv_Packet;
		}
		len = ((len - 192) << 8) + *p + 192;
		p++;
		pktlen--;
	}

	if (*p != 1) {		/* Currently only 1, meaning an image, is defined. */
		cdk_free(buf);
		return CDK_Inv_Packet;
	}
	p++;
	len--;

	if (len >= pktlen) {
		cdk_free(buf);
		return CDK_Inv_Packet;
	}
	attr->attrib_img = cdk_calloc(1, len);
	if (!attr->attrib_img) {
		cdk_free(buf);
		return CDK_Out_Of_Core;
	}
	attr->attrib_len = len;
	memcpy(attr->attrib_img, p, len);
	cdk_free(buf);
	return rc;
}