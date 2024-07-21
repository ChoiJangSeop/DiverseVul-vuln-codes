int credssp_server_authenticate(rdpCredssp* credssp)
{
	UINT32 cbMaxToken;
	ULONG fContextReq;
	ULONG pfContextAttr;
	SECURITY_STATUS status;
	CredHandle credentials;
	TimeStamp expiration;
	PSecPkgInfo pPackageInfo;
	SecBuffer input_buffer;
	SecBuffer output_buffer;
	SecBufferDesc input_buffer_desc;
	SecBufferDesc output_buffer_desc;
	BOOL have_context;
	BOOL have_input_buffer;
	BOOL have_pub_key_auth;

	sspi_GlobalInit();

	if (credssp_ntlm_server_init(credssp) == 0)
		return 0;

#ifdef WITH_NATIVE_SSPI
	if (!credssp->SspiModule)
		credssp->SspiModule = _tcsdup(_T("secur32.dll"));
#endif

	if (credssp->SspiModule)
	{
		HMODULE hSSPI;
		INIT_SECURITY_INTERFACE pInitSecurityInterface;

		hSSPI = LoadLibrary(credssp->SspiModule);

		if (!hSSPI)
		{
			_tprintf(_T("Failed to load SSPI module: %s\n"), credssp->SspiModule);
			return 0;
		}

#ifdef UNICODE
		pInitSecurityInterface = (INIT_SECURITY_INTERFACE) GetProcAddress(hSSPI, "InitSecurityInterfaceW");
#else
		pInitSecurityInterface = (INIT_SECURITY_INTERFACE) GetProcAddress(hSSPI, "InitSecurityInterfaceA");
#endif

		credssp->table = (*pInitSecurityInterface)();
	}
#ifndef WITH_NATIVE_SSPI
	else
	{
		credssp->table = InitSecurityInterface();
	}
#endif

	status = credssp->table->QuerySecurityPackageInfo(NLA_PKG_NAME, &pPackageInfo);

	if (status != SEC_E_OK)
	{
		fprintf(stderr, "QuerySecurityPackageInfo status: 0x%08X\n", status);
		return 0;
	}

	cbMaxToken = pPackageInfo->cbMaxToken;

	status = credssp->table->AcquireCredentialsHandle(NULL, NLA_PKG_NAME,
			SECPKG_CRED_INBOUND, NULL, NULL, NULL, NULL, &credentials, &expiration);

	if (status != SEC_E_OK)
	{
		fprintf(stderr, "AcquireCredentialsHandle status: 0x%08X\n", status);
		return 0;
	}

	have_context = FALSE;
	have_input_buffer = FALSE;
	have_pub_key_auth = FALSE;
	ZeroMemory(&input_buffer, sizeof(SecBuffer));
	ZeroMemory(&output_buffer, sizeof(SecBuffer));
	ZeroMemory(&input_buffer_desc, sizeof(SecBufferDesc));
	ZeroMemory(&output_buffer_desc, sizeof(SecBufferDesc));
	ZeroMemory(&credssp->ContextSizes, sizeof(SecPkgContext_Sizes));

	/* 
	 * from tspkg.dll: 0x00000112
	 * ASC_REQ_MUTUAL_AUTH
	 * ASC_REQ_CONFIDENTIALITY
	 * ASC_REQ_ALLOCATE_MEMORY
	 */

	fContextReq = 0;
	fContextReq |= ASC_REQ_MUTUAL_AUTH;
	fContextReq |= ASC_REQ_CONFIDENTIALITY;

	fContextReq |= ASC_REQ_CONNECTION;
	fContextReq |= ASC_REQ_USE_SESSION_KEY;

	fContextReq |= ASC_REQ_REPLAY_DETECT;
	fContextReq |= ASC_REQ_SEQUENCE_DETECT;

	fContextReq |= ASC_REQ_EXTENDED_ERROR;

	while (TRUE)
	{
		input_buffer_desc.ulVersion = SECBUFFER_VERSION;
		input_buffer_desc.cBuffers = 1;
		input_buffer_desc.pBuffers = &input_buffer;
		input_buffer.BufferType = SECBUFFER_TOKEN;

		/* receive authentication token */

		input_buffer_desc.ulVersion = SECBUFFER_VERSION;
		input_buffer_desc.cBuffers = 1;
		input_buffer_desc.pBuffers = &input_buffer;
		input_buffer.BufferType = SECBUFFER_TOKEN;

		if (credssp_recv(credssp) < 0)
			return -1;

#ifdef WITH_DEBUG_CREDSSP
		fprintf(stderr, "Receiving Authentication Token\n");
		credssp_buffer_print(credssp);
#endif

		input_buffer.pvBuffer = credssp->negoToken.pvBuffer;
		input_buffer.cbBuffer = credssp->negoToken.cbBuffer;

		if (credssp->negoToken.cbBuffer < 1)
		{
			fprintf(stderr, "CredSSP: invalid negoToken!\n");
			return -1;
		}

		output_buffer_desc.ulVersion = SECBUFFER_VERSION;
		output_buffer_desc.cBuffers = 1;
		output_buffer_desc.pBuffers = &output_buffer;
		output_buffer.BufferType = SECBUFFER_TOKEN;
		output_buffer.cbBuffer = cbMaxToken;
		output_buffer.pvBuffer = malloc(output_buffer.cbBuffer);

		status = credssp->table->AcceptSecurityContext(&credentials,
			have_context? &credssp->context: NULL,
			&input_buffer_desc, fContextReq, SECURITY_NATIVE_DREP, &credssp->context,
			&output_buffer_desc, &pfContextAttr, &expiration);

		credssp->negoToken.pvBuffer = output_buffer.pvBuffer;
		credssp->negoToken.cbBuffer = output_buffer.cbBuffer;

		if ((status == SEC_I_COMPLETE_AND_CONTINUE) || (status == SEC_I_COMPLETE_NEEDED))
		{
			if (credssp->table->CompleteAuthToken != NULL)
				credssp->table->CompleteAuthToken(&credssp->context, &output_buffer_desc);

			if (status == SEC_I_COMPLETE_NEEDED)
				status = SEC_E_OK;
			else if (status == SEC_I_COMPLETE_AND_CONTINUE)
				status = SEC_I_CONTINUE_NEEDED;
		}

		if (status == SEC_E_OK)
		{
			have_pub_key_auth = TRUE;

			if (credssp->table->QueryContextAttributes(&credssp->context, SECPKG_ATTR_SIZES, &credssp->ContextSizes) != SEC_E_OK)
			{
				fprintf(stderr, "QueryContextAttributes SECPKG_ATTR_SIZES failure\n");
				return 0;
			}

			if (credssp_decrypt_public_key_echo(credssp) != SEC_E_OK)
			{
				fprintf(stderr, "Error: could not verify client's public key echo\n");
				return -1;
			}

			sspi_SecBufferFree(&credssp->negoToken);
			credssp->negoToken.pvBuffer = NULL;
			credssp->negoToken.cbBuffer = 0;

			credssp_encrypt_public_key_echo(credssp);
		}

		if ((status != SEC_E_OK) && (status != SEC_I_CONTINUE_NEEDED))
		{
			fprintf(stderr, "AcceptSecurityContext status: 0x%08X\n", status);
			return -1;
		}

		/* send authentication token */

#ifdef WITH_DEBUG_CREDSSP
		fprintf(stderr, "Sending Authentication Token\n");
		credssp_buffer_print(credssp);
#endif

		credssp_send(credssp);
		credssp_buffer_free(credssp);

		if (status != SEC_I_CONTINUE_NEEDED)
			break;

		have_context = TRUE;
	}

	/* Receive encrypted credentials */

	if (credssp_recv(credssp) < 0)
		return -1;

	if (credssp_decrypt_ts_credentials(credssp) != SEC_E_OK)
	{
		fprintf(stderr, "Could not decrypt TSCredentials status: 0x%08X\n", status);
		return 0;
	}

	if (status != SEC_E_OK)
	{
		fprintf(stderr, "AcceptSecurityContext status: 0x%08X\n", status);
		return 0;
	}

	status = credssp->table->ImpersonateSecurityContext(&credssp->context);

	if (status != SEC_E_OK)
	{
		fprintf(stderr, "ImpersonateSecurityContext status: 0x%08X\n", status);
		return 0;
	}
	else
	{
		status = credssp->table->RevertSecurityContext(&credssp->context);

		if (status != SEC_E_OK)
		{
			fprintf(stderr, "RevertSecurityContext status: 0x%08X\n", status);
			return 0;
		}
	}

	credssp->table->FreeContextBuffer(pPackageInfo);

	return 1;
}