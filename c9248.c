int ntlm_construct_authenticate_target_info(NTLM_CONTEXT* context)
{
	ULONG size;
	ULONG AvPairsCount;
	ULONG AvPairsValueLength;
	NTLM_AV_PAIR* AvTimestamp;
	NTLM_AV_PAIR* AvNbDomainName;
	NTLM_AV_PAIR* AvNbComputerName;
	NTLM_AV_PAIR* AvDnsDomainName;
	NTLM_AV_PAIR* AvDnsComputerName;
	NTLM_AV_PAIR* AvDnsTreeName;
	NTLM_AV_PAIR* ChallengeTargetInfo;
	NTLM_AV_PAIR* AuthenticateTargetInfo;
	size_t cbAvTimestamp;
	size_t cbAvNbDomainName;
	size_t cbAvNbComputerName;
	size_t cbAvDnsDomainName;
	size_t cbAvDnsComputerName;
	size_t cbAvDnsTreeName;
	size_t cbChallengeTargetInfo;
	size_t cbAuthenticateTargetInfo;
	AvPairsCount = 1;
	AvPairsValueLength = 0;
	ChallengeTargetInfo = (NTLM_AV_PAIR*)context->ChallengeTargetInfo.pvBuffer;
	cbChallengeTargetInfo = context->ChallengeTargetInfo.cbBuffer;
	AvNbDomainName = ntlm_av_pair_get(ChallengeTargetInfo, cbChallengeTargetInfo, MsvAvNbDomainName,
	                                  &cbAvNbDomainName);
	AvNbComputerName = ntlm_av_pair_get(ChallengeTargetInfo, cbChallengeTargetInfo,
	                                    MsvAvNbComputerName, &cbAvNbComputerName);
	AvDnsDomainName = ntlm_av_pair_get(ChallengeTargetInfo, cbChallengeTargetInfo,
	                                   MsvAvDnsDomainName, &cbAvDnsDomainName);
	AvDnsComputerName = ntlm_av_pair_get(ChallengeTargetInfo, cbChallengeTargetInfo,
	                                     MsvAvDnsComputerName, &cbAvDnsComputerName);
	AvDnsTreeName = ntlm_av_pair_get(ChallengeTargetInfo, cbChallengeTargetInfo, MsvAvDnsTreeName,
	                                 &cbAvDnsTreeName);
	AvTimestamp = ntlm_av_pair_get(ChallengeTargetInfo, cbChallengeTargetInfo, MsvAvTimestamp,
	                               &cbAvTimestamp);

	if (AvNbDomainName)
	{
		AvPairsCount++; /* MsvAvNbDomainName */
		AvPairsValueLength += ntlm_av_pair_get_len(AvNbDomainName);
	}

	if (AvNbComputerName)
	{
		AvPairsCount++; /* MsvAvNbComputerName */
		AvPairsValueLength += ntlm_av_pair_get_len(AvNbComputerName);
	}

	if (AvDnsDomainName)
	{
		AvPairsCount++; /* MsvAvDnsDomainName */
		AvPairsValueLength += ntlm_av_pair_get_len(AvDnsDomainName);
	}

	if (AvDnsComputerName)
	{
		AvPairsCount++; /* MsvAvDnsComputerName */
		AvPairsValueLength += ntlm_av_pair_get_len(AvDnsComputerName);
	}

	if (AvDnsTreeName)
	{
		AvPairsCount++; /* MsvAvDnsTreeName */
		AvPairsValueLength += ntlm_av_pair_get_len(AvDnsTreeName);
	}

	AvPairsCount++; /* MsvAvTimestamp */
	AvPairsValueLength += 8;

	if (context->UseMIC)
	{
		AvPairsCount++; /* MsvAvFlags */
		AvPairsValueLength += 4;
	}

	if (context->SendSingleHostData)
	{
		AvPairsCount++; /* MsvAvSingleHost */
		ntlm_compute_single_host_data(context);
		AvPairsValueLength += context->SingleHostData.Size;
	}

	/**
	 * Extended Protection for Authentication:
	 * http://blogs.technet.com/b/srd/archive/2009/12/08/extended-protection-for-authentication.aspx
	 */

	if (!context->SuppressExtendedProtection)
	{
		/**
		 * SEC_CHANNEL_BINDINGS structure
		 * http://msdn.microsoft.com/en-us/library/windows/desktop/dd919963/
		 */
		AvPairsCount++; /* MsvChannelBindings */
		AvPairsValueLength += 16;
		ntlm_compute_channel_bindings(context);

		if (context->ServicePrincipalName.Length > 0)
		{
			AvPairsCount++; /* MsvAvTargetName */
			AvPairsValueLength += context->ServicePrincipalName.Length;
		}
	}

	size = ntlm_av_pair_list_size(AvPairsCount, AvPairsValueLength);

	if (context->NTLMv2)
		size += 8; /* unknown 8-byte padding */

	if (!sspi_SecBufferAlloc(&context->AuthenticateTargetInfo, size))
		goto fail;

	AuthenticateTargetInfo = (NTLM_AV_PAIR*)context->AuthenticateTargetInfo.pvBuffer;
	cbAuthenticateTargetInfo = context->AuthenticateTargetInfo.cbBuffer;

	if (!ntlm_av_pair_list_init(AuthenticateTargetInfo, cbAuthenticateTargetInfo))
		goto fail;

	if (AvNbDomainName)
	{
		if (!ntlm_av_pair_add_copy(AuthenticateTargetInfo, cbAuthenticateTargetInfo, AvNbDomainName,
		                           cbAvNbDomainName))
			goto fail;
	}

	if (AvNbComputerName)
	{
		if (!ntlm_av_pair_add_copy(AuthenticateTargetInfo, cbAuthenticateTargetInfo,
		                           AvNbComputerName, cbAvNbComputerName))
			goto fail;
	}

	if (AvDnsDomainName)
	{
		if (!ntlm_av_pair_add_copy(AuthenticateTargetInfo, cbAuthenticateTargetInfo,
		                           AvDnsDomainName, cbAvDnsDomainName))
			goto fail;
	}

	if (AvDnsComputerName)
	{
		if (!ntlm_av_pair_add_copy(AuthenticateTargetInfo, cbAuthenticateTargetInfo,
		                           AvDnsComputerName, cbAvDnsComputerName))
			goto fail;
	}

	if (AvDnsTreeName)
	{
		if (!ntlm_av_pair_add_copy(AuthenticateTargetInfo, cbAuthenticateTargetInfo, AvDnsTreeName,
		                           cbAvDnsTreeName))
			goto fail;
	}

	if (AvTimestamp)
	{
		if (!ntlm_av_pair_add_copy(AuthenticateTargetInfo, cbAuthenticateTargetInfo, AvTimestamp,
		                           cbAvTimestamp))
			goto fail;
	}

	if (context->UseMIC)
	{
		UINT32 flags;
		Data_Write_UINT32(&flags, MSV_AV_FLAGS_MESSAGE_INTEGRITY_CHECK);

		if (!ntlm_av_pair_add(AuthenticateTargetInfo, cbAuthenticateTargetInfo, MsvAvFlags,
		                      (PBYTE)&flags, 4))
			goto fail;
	}

	if (context->SendSingleHostData)
	{
		if (!ntlm_av_pair_add(AuthenticateTargetInfo, cbAuthenticateTargetInfo, MsvAvSingleHost,
		                      (PBYTE)&context->SingleHostData, context->SingleHostData.Size))
			goto fail;
	}

	if (!context->SuppressExtendedProtection)
	{
		if (!ntlm_av_pair_add(AuthenticateTargetInfo, cbAuthenticateTargetInfo, MsvChannelBindings,
		                      context->ChannelBindingsHash, 16))
			goto fail;

		if (context->ServicePrincipalName.Length > 0)
		{
			if (!ntlm_av_pair_add(AuthenticateTargetInfo, cbAuthenticateTargetInfo, MsvAvTargetName,
			                      (PBYTE)context->ServicePrincipalName.Buffer,
			                      context->ServicePrincipalName.Length))
				goto fail;
		}
	}

	if (context->NTLMv2)
	{
		NTLM_AV_PAIR* AvEOL;
		AvEOL = ntlm_av_pair_get(ChallengeTargetInfo, cbChallengeTargetInfo, MsvAvEOL, NULL);

		if (!AvEOL)
			goto fail;

		ZeroMemory(AvEOL, sizeof(NTLM_AV_PAIR));
	}

	return 1;
fail:
	sspi_SecBufferFree(&context->AuthenticateTargetInfo);
	return -1;
}