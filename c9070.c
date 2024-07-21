SECURITY_STATUS ntlm_read_NegotiateMessage(NTLM_CONTEXT* context, PSecBuffer buffer)
{
	wStream* s;
	size_t length;
	NTLM_NEGOTIATE_MESSAGE* message;
	message = &context->NEGOTIATE_MESSAGE;
	ZeroMemory(message, sizeof(NTLM_NEGOTIATE_MESSAGE));
	s = Stream_New((BYTE*)buffer->pvBuffer, buffer->cbBuffer);

	if (!s)
		return SEC_E_INTERNAL_ERROR;

	if (ntlm_read_message_header(s, (NTLM_MESSAGE_HEADER*)message) < 0)
	{
		Stream_Free(s, FALSE);
		return SEC_E_INVALID_TOKEN;
	}

	if (message->MessageType != MESSAGE_TYPE_NEGOTIATE)
	{
		Stream_Free(s, FALSE);
		return SEC_E_INVALID_TOKEN;
	}

	Stream_Read_UINT32(s, message->NegotiateFlags); /* NegotiateFlags (4 bytes) */

	if (!((message->NegotiateFlags & NTLMSSP_REQUEST_TARGET) &&
	      (message->NegotiateFlags & NTLMSSP_NEGOTIATE_NTLM) &&
	      (message->NegotiateFlags & NTLMSSP_NEGOTIATE_UNICODE)))
	{
		Stream_Free(s, FALSE);
		return SEC_E_INVALID_TOKEN;
	}

	context->NegotiateFlags = message->NegotiateFlags;

	/* only set if NTLMSSP_NEGOTIATE_DOMAIN_SUPPLIED is set */

	if (ntlm_read_message_fields(s, &(message->DomainName)) < 0) /* DomainNameFields (8 bytes) */
	{
		Stream_Free(s, FALSE);
		return SEC_E_INVALID_TOKEN;
	}

	/* only set if NTLMSSP_NEGOTIATE_WORKSTATION_SUPPLIED is set */

	if (ntlm_read_message_fields(s, &(message->Workstation)) < 0) /* WorkstationFields (8 bytes) */
	{
		Stream_Free(s, FALSE);
		return SEC_E_INVALID_TOKEN;
	}

	if (message->NegotiateFlags & NTLMSSP_NEGOTIATE_VERSION)
	{
		if (ntlm_read_version_info(s, &(message->Version)) < 0) /* Version (8 bytes) */
		{
			Stream_Free(s, FALSE);
			return SEC_E_INVALID_TOKEN;
		}
	}

	length = Stream_GetPosition(s);
	buffer->cbBuffer = length;

	if (!sspi_SecBufferAlloc(&context->NegotiateMessage, length))
	{
		Stream_Free(s, FALSE);
		return SEC_E_INTERNAL_ERROR;
	}

	CopyMemory(context->NegotiateMessage.pvBuffer, buffer->pvBuffer, buffer->cbBuffer);
	context->NegotiateMessage.BufferType = buffer->BufferType;
#ifdef WITH_DEBUG_NTLM
	WLog_DBG(TAG, "NEGOTIATE_MESSAGE (length = %" PRIu32 ")", context->NegotiateMessage.cbBuffer);
	winpr_HexDump(TAG, WLOG_DEBUG, context->NegotiateMessage.pvBuffer,
	              context->NegotiateMessage.cbBuffer);
	ntlm_print_negotiate_flags(message->NegotiateFlags);

	if (message->NegotiateFlags & NTLMSSP_NEGOTIATE_VERSION)
		ntlm_print_version_info(&(message->Version));

#endif
	context->state = NTLM_STATE_CHALLENGE;
	Stream_Free(s, FALSE);
	return SEC_I_CONTINUE_NEEDED;
}