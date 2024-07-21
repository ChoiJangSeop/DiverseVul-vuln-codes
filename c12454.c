bool DNP3_Base::AddToBuffer(Endpoint* endp, int target_len, const u_char** data, int* len)
	{
	if ( ! target_len )
		return true;

	int to_copy = min(*len, target_len - endp->buffer_len);

	memcpy(endp->buffer + endp->buffer_len, *data, to_copy);
	*data += to_copy;
	*len -= to_copy;
	endp->buffer_len += to_copy;

	return endp->buffer_len == target_len;
	}