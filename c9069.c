SECURITY_STATUS ntlm_read_AuthenticateMessage(NTLM_CONTEXT* context, PSecBuffer buffer)
{
	wStream* s;
	size_t length;
	UINT32 flags;
	NTLM_AV_PAIR* AvFlags;
	UINT32 PayloadBufferOffset;
	NTLM_AUTHENTICATE_MESSAGE* message;
	SSPI_CREDENTIALS* credentials = context->credentials;
	flags = 0;
	AvFlags = NULL;
	message = &context->AUTHENTICATE_MESSAGE;
	ZeroMemory(message, sizeof(NTLM_AUTHENTICATE_MESSAGE));
	s = Stream_New((BYTE*)buffer->pvBuffer, buffer->cbBuffer);

	if (!s)
		return SEC_E_INTERNAL_ERROR;

	if (ntlm_read_message_header(s, (NTLM_MESSAGE_HEADER*)message) < 0)
	{
		Stream_Free(s, FALSE);
		return SEC_E_INVALID_TOKEN;
	}

	if (message->MessageType != MESSAGE_TYPE_AUTHENTICATE)
	{
		Stream_Free(s, FALSE);
		return SEC_E_INVALID_TOKEN;
	}

	if (ntlm_read_message_fields(s, &(message->LmChallengeResponse)) <
	    0) /* LmChallengeResponseFields (8 bytes) */
	{
		Stream_Free(s, FALSE);
		return SEC_E_INVALID_TOKEN;
	}

	if (ntlm_read_message_fields(s, &(message->NtChallengeResponse)) <
	    0) /* NtChallengeResponseFields (8 bytes) */
	{
		Stream_Free(s, FALSE);
		return SEC_E_INVALID_TOKEN;
	}

	if (ntlm_read_message_fields(s, &(message->DomainName)) < 0) /* DomainNameFields (8 bytes) */
	{
		Stream_Free(s, FALSE);
		return SEC_E_INVALID_TOKEN;
	}

	if (ntlm_read_message_fields(s, &(message->UserName)) < 0) /* UserNameFields (8 bytes) */
	{
		Stream_Free(s, FALSE);
		return SEC_E_INVALID_TOKEN;
	}

	if (ntlm_read_message_fields(s, &(message->Workstation)) < 0) /* WorkstationFields (8 bytes) */
	{
		Stream_Free(s, FALSE);
		return SEC_E_INVALID_TOKEN;
	}

	if (ntlm_read_message_fields(s, &(message->EncryptedRandomSessionKey)) <
	    0) /* EncryptedRandomSessionKeyFields (8 bytes) */
	{
		Stream_Free(s, FALSE);
		return SEC_E_INVALID_TOKEN;
	}

	Stream_Read_UINT32(s, message->NegotiateFlags); /* NegotiateFlags (4 bytes) */
	context->NegotiateKeyExchange =
	    (message->NegotiateFlags & NTLMSSP_NEGOTIATE_KEY_EXCH) ? TRUE : FALSE;

	if ((context->NegotiateKeyExchange && !message->EncryptedRandomSessionKey.Len) ||
	    (!context->NegotiateKeyExchange && message->EncryptedRandomSessionKey.Len))
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

	PayloadBufferOffset = Stream_GetPosition(s);

	if (ntlm_read_message_fields_buffer(s, &(message->DomainName)) < 0) /* DomainName */
	{
		Stream_Free(s, FALSE);
		return SEC_E_INTERNAL_ERROR;
	}

	if (ntlm_read_message_fields_buffer(s, &(message->UserName)) < 0) /* UserName */
	{
		Stream_Free(s, FALSE);
		return SEC_E_INTERNAL_ERROR;
	}

	if (ntlm_read_message_fields_buffer(s, &(message->Workstation)) < 0) /* Workstation */
	{
		Stream_Free(s, FALSE);
		return SEC_E_INTERNAL_ERROR;
	}

	if (ntlm_read_message_fields_buffer(s, &(message->LmChallengeResponse)) <
	    0) /* LmChallengeResponse */
	{
		Stream_Free(s, FALSE);
		return SEC_E_INTERNAL_ERROR;
	}

	if (ntlm_read_message_fields_buffer(s, &(message->NtChallengeResponse)) <
	    0) /* NtChallengeResponse */
	{
		Stream_Free(s, FALSE);
		return SEC_E_INTERNAL_ERROR;
	}

	if (message->NtChallengeResponse.Len > 0)
	{
		size_t cbAvFlags;
		wStream* snt =
		    Stream_New(message->NtChallengeResponse.Buffer, message->NtChallengeResponse.Len);

		if (!snt)
		{
			Stream_Free(s, FALSE);
			return SEC_E_INTERNAL_ERROR;
		}

		if (ntlm_read_ntlm_v2_response(snt, &(context->NTLMv2Response)) < 0)
		{
			Stream_Free(s, FALSE);
			Stream_Free(snt, FALSE);
			return SEC_E_INVALID_TOKEN;
		}

		Stream_Free(snt, FALSE);
		context->NtChallengeResponse.pvBuffer = message->NtChallengeResponse.Buffer;
		context->NtChallengeResponse.cbBuffer = message->NtChallengeResponse.Len;
		sspi_SecBufferFree(&(context->ChallengeTargetInfo));
		context->ChallengeTargetInfo.pvBuffer = (void*)context->NTLMv2Response.Challenge.AvPairs;
		context->ChallengeTargetInfo.cbBuffer = message->NtChallengeResponse.Len - (28 + 16);
		CopyMemory(context->ClientChallenge, context->NTLMv2Response.Challenge.ClientChallenge, 8);
		AvFlags =
		    ntlm_av_pair_get(context->NTLMv2Response.Challenge.AvPairs,
		                     context->NTLMv2Response.Challenge.cbAvPairs, MsvAvFlags, &cbAvFlags);

		if (AvFlags)
			Data_Read_UINT32(ntlm_av_pair_get_value_pointer(AvFlags), flags);
	}

	if (ntlm_read_message_fields_buffer(s, &(message->EncryptedRandomSessionKey)) <
	    0) /* EncryptedRandomSessionKey */
	{
		Stream_Free(s, FALSE);
		return SEC_E_INTERNAL_ERROR;
	}

	if (message->EncryptedRandomSessionKey.Len > 0)
	{
		if (message->EncryptedRandomSessionKey.Len != 16)
		{
			Stream_Free(s, FALSE);
			return SEC_E_INVALID_TOKEN;
		}

		CopyMemory(context->EncryptedRandomSessionKey, message->EncryptedRandomSessionKey.Buffer,
		           16);
	}

	length = Stream_GetPosition(s);

	if (!sspi_SecBufferAlloc(&context->AuthenticateMessage, length))
	{
		Stream_Free(s, FALSE);
		return SEC_E_INTERNAL_ERROR;
	}

	CopyMemory(context->AuthenticateMessage.pvBuffer, Stream_Buffer(s), length);
	buffer->cbBuffer = length;
	Stream_SetPosition(s, PayloadBufferOffset);

	if (flags & MSV_AV_FLAGS_MESSAGE_INTEGRITY_CHECK)
	{
		context->MessageIntegrityCheckOffset = (UINT32)Stream_GetPosition(s);

		if (Stream_GetRemainingLength(s) < 16)
		{
			Stream_Free(s, FALSE);
			return SEC_E_INVALID_TOKEN;
		}

		Stream_Read(s, message->MessageIntegrityCheck, 16);
	}

#ifdef WITH_DEBUG_NTLM
	WLog_DBG(TAG, "AUTHENTICATE_MESSAGE (length = %" PRIu32 ")",
	         context->AuthenticateMessage.cbBuffer);
	winpr_HexDump(TAG, WLOG_DEBUG, context->AuthenticateMessage.pvBuffer,
	              context->AuthenticateMessage.cbBuffer);

	if (message->NegotiateFlags & NTLMSSP_NEGOTIATE_VERSION)
		ntlm_print_version_info(&(message->Version));

	ntlm_print_message_fields(&(message->DomainName), "DomainName");
	ntlm_print_message_fields(&(message->UserName), "UserName");
	ntlm_print_message_fields(&(message->Workstation), "Workstation");
	ntlm_print_message_fields(&(message->LmChallengeResponse), "LmChallengeResponse");
	ntlm_print_message_fields(&(message->NtChallengeResponse), "NtChallengeResponse");
	ntlm_print_message_fields(&(message->EncryptedRandomSessionKey), "EncryptedRandomSessionKey");
	ntlm_print_av_pair_list(context->NTLMv2Response.Challenge.AvPairs,
	                        context->NTLMv2Response.Challenge.cbAvPairs);

	if (flags & MSV_AV_FLAGS_MESSAGE_INTEGRITY_CHECK)
	{
		WLog_DBG(TAG, "MessageIntegrityCheck:");
		winpr_HexDump(TAG, WLOG_DEBUG, message->MessageIntegrityCheck, 16);
	}

#endif

	if (message->UserName.Len > 0)
	{
		credentials->identity.User = (UINT16*)malloc(message->UserName.Len);

		if (!credentials->identity.User)
		{
			Stream_Free(s, FALSE);
			return SEC_E_INTERNAL_ERROR;
		}

		CopyMemory(credentials->identity.User, message->UserName.Buffer, message->UserName.Len);
		credentials->identity.UserLength = message->UserName.Len / 2;
	}

	if (message->DomainName.Len > 0)
	{
		credentials->identity.Domain = (UINT16*)malloc(message->DomainName.Len);

		if (!credentials->identity.Domain)
		{
			Stream_Free(s, FALSE);
			return SEC_E_INTERNAL_ERROR;
		}

		CopyMemory(credentials->identity.Domain, message->DomainName.Buffer,
		           message->DomainName.Len);
		credentials->identity.DomainLength = message->DomainName.Len / 2;
	}

	Stream_Free(s, FALSE);
	/* Computations beyond this point require the NTLM hash of the password */
	context->state = NTLM_STATE_COMPLETION;
	return SEC_I_COMPLETE_NEEDED;
}