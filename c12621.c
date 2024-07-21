static BOOL nla_client_init_cred_handle(rdpNla* nla)
{
	SEC_WINPR_KERBEROS_SETTINGS* kerbSettings;

	WINPR_ASSERT(nla);
	kerbSettings = nla->kerberosSettings;
	WINPR_ASSERT(kerbSettings);
	WINPR_ASSERT(nla->table->SetCredentialsAttributes);

	if (kerbSettings->kdcUrl)
	{
#ifdef UNICODE
		SecPkgCredentials_KdcUrlW secAttr = { NULL };
		ConvertToUnicode(CP_UTF8, 0, kerbSettings->kdcUrl, -1, &secAttr.KdcUrl, 0);

		if (!secAttr.KdcUrl)
			return FALSE;

		nla->table->SetCredentialsAttributesW(&nla->credentials, SECPKG_CRED_ATTR_KDC_URL,
		                                      (void*)&secAttr, sizeof(secAttr));

		free(secAttr.KdcUrl);
#else
		SecPkgCredentials_KdcUrlA secAttr = { NULL };
		secAttr.KdcUrl = kerbSettings->kdcUrl;
		nla->table->SetCredentialsAttributesA(&nla->credentials, SECPKG_CRED_ATTR_KDC_URL,
		                                      (void*)&secAttr, sizeof(secAttr));
#endif
	}

	return TRUE;
}