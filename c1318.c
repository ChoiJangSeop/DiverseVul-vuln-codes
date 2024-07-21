int credssp_ntlm_client_init(rdpCredssp* credssp)
{
	char* spn;
	int length;
	freerdp* instance;
	rdpSettings* settings;
	
	settings = credssp->settings;
	instance = (freerdp*) settings->instance;

	if ((settings->Password == NULL) || (settings->Username == NULL))
	{
		if (instance->Authenticate)
		{
			BOOL proceed = instance->Authenticate(instance,
					&settings->Username, &settings->Password, &settings->Domain);
			if (!proceed)
				return 0;
		}
	}

	sspi_SetAuthIdentity(&(credssp->identity), settings->Username, settings->Domain, settings->Password);

#ifdef WITH_DEBUG_NLA
	_tprintf(_T("User: %s Domain: %s Password: %s\n"),
		(char*) credssp->identity.User, (char*) credssp->identity.Domain, (char*) credssp->identity.Password);
#endif

	sspi_SecBufferAlloc(&credssp->PublicKey, credssp->transport->TlsIn->PublicKeyLength);
	CopyMemory(credssp->PublicKey.pvBuffer, credssp->transport->TlsIn->PublicKey, credssp->transport->TlsIn->PublicKeyLength);

	length = sizeof(TERMSRV_SPN_PREFIX) + strlen(settings->ServerHostname);

	spn = (SEC_CHAR*) malloc(length + 1);
	sprintf(spn, "%s%s", TERMSRV_SPN_PREFIX, settings->ServerHostname);

#ifdef UNICODE
	credssp->ServicePrincipalName = (LPTSTR) malloc(length * 2 + 2);
	MultiByteToWideChar(CP_UTF8, 0, spn, length,
		(LPWSTR) credssp->ServicePrincipalName, length);
	free(spn);
#else
	credssp->ServicePrincipalName = spn;
#endif

	return 1;
}