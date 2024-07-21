proto_read_byte_buffer (p11_rpc_message *msg,
                        CK_BYTE_PTR *buffer,
                        CK_ULONG *n_buffer)
{
	uint32_t length;

	assert (msg != NULL);
	assert (buffer != NULL);
	assert (n_buffer != NULL);
	assert (msg->input != NULL);

	/* Check that we're supposed to be reading this at this point */
	assert (!msg->signature || p11_rpc_message_verify_part (msg, "fy"));

	/* The number of ulongs there's room for on the other end */
	if (!p11_rpc_buffer_get_uint32 (msg->input, &msg->parsed, &length))
		return PARSE_ERROR;

	*n_buffer = length;
	*buffer = NULL;

	/* If set to zero, then they just want the length */
	if (length == 0)
		return CKR_OK;

	*buffer = p11_rpc_message_alloc_extra (msg, length * sizeof (CK_BYTE));
	if (*buffer == NULL)
		return CKR_DEVICE_MEMORY;

	return CKR_OK;
}