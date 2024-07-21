read_subpkt(cdk_stream_t inp, cdk_subpkt_t * r_ctx, size_t * r_nbytes)
{
	byte c, c1;
	size_t size, nread, n;
	cdk_subpkt_t node;
	cdk_error_t rc;

	if (!inp || !r_nbytes)
		return CDK_Inv_Value;

	if (DEBUG_PKT)
		_gnutls_write_log("read_subpkt:\n");

	n = 0;
	*r_nbytes = 0;
	c = cdk_stream_getc(inp);
	n++;
	if (c == 255) {
		size = read_32(inp);
		n += 4;
	} else if (c >= 192 && c < 255) {
		c1 = cdk_stream_getc(inp);
		n++;
		if (c1 == 0)
			return 0;
		size = ((c - 192) << 8) + c1 + 192;
	} else if (c < 192)
		size = c;
	else
		return CDK_Inv_Packet;

	node = cdk_subpkt_new(size);
	if (!node)
		return CDK_Out_Of_Core;
	node->size = size;
	node->type = cdk_stream_getc(inp);
	if (DEBUG_PKT)
		_gnutls_write_log(" %d octets %d type\n", node->size,
				  node->type);
	n++;
	node->size--;
	rc = stream_read(inp, node->d, node->size, &nread);
	n += nread;
	if (rc) {
		cdk_subpkt_free(node);
		return rc;
	}
	*r_nbytes = n;
	if (!*r_ctx)
		*r_ctx = node;
	else
		cdk_subpkt_add(*r_ctx, node);
	return rc;
}