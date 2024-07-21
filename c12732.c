LONG ValidateSignature(HWND hDlg, const char* path)
{
	LONG r;
	WINTRUST_DATA trust_data = { 0 };
	WINTRUST_FILE_INFO trust_file = { 0 };
	GUID guid_generic_verify =	// WINTRUST_ACTION_GENERIC_VERIFY_V2
		{ 0xaac56b, 0xcd44, 0x11d0,{ 0x8c, 0xc2, 0x0, 0xc0, 0x4f, 0xc2, 0x95, 0xee } };
	char *signature_name;
	size_t i;
	uint64_t current_ts, update_ts;

	// Check the signature name. Make it specific enough (i.e. don't simply check for "Akeo")
	// so that, besides hacking our server, it'll place an extra hurdle on any malicious entity
	// into also fooling a C.A. to issue a certificate that passes our test.
	signature_name = GetSignatureName(path, cert_country, (hDlg == INVALID_HANDLE_VALUE));
	if (signature_name == NULL) {
		uprintf("PKI: Could not get signature name");
		if (hDlg != INVALID_HANDLE_VALUE)
			MessageBoxExU(hDlg, lmprintf(MSG_284), lmprintf(MSG_283), MB_OK | MB_ICONERROR | MB_IS_RTL, selected_langid);
		return TRUST_E_NOSIGNATURE;
	}
	for (i = 0; i < ARRAYSIZE(cert_name); i++) {
		if (strcmp(signature_name, cert_name[i]) == 0)
			break;
	}
	if (i >= ARRAYSIZE(cert_name)) {
		uprintf("PKI: Signature '%s' is unexpected...", signature_name);
		if ((hDlg == INVALID_HANDLE_VALUE) || (MessageBoxExU(hDlg,
			lmprintf(MSG_285, signature_name), lmprintf(MSG_283),
			MB_YESNO | MB_ICONWARNING | MB_IS_RTL, selected_langid) != IDYES))
			return TRUST_E_EXPLICIT_DISTRUST;
	}

	trust_file.cbStruct = sizeof(trust_file);
	trust_file.pcwszFilePath = utf8_to_wchar(path);
	if (trust_file.pcwszFilePath == NULL) {
		uprintf("PKI: Unable to convert '%s' to UTF16", path);
		return ERROR_SEVERITY_ERROR | FAC(FACILITY_CERT) | ERROR_NOT_ENOUGH_MEMORY;
	}

	trust_data.cbStruct = sizeof(trust_data);
	// NB: WTD_UI_ALL can result in ERROR_SUCCESS even if the signature validation fails,
	// because it still prompts the user to run untrusted software, even after explicitly
	// notifying them that the signature invalid (and of course Microsoft had to make
	// that UI prompt a bit too similar to the other benign prompt you get when running
	// trusted software, which, as per cert.org's assessment, may confuse non-security
	// conscious-users who decide to gloss over these kind of notifications).
	trust_data.dwUIChoice = WTD_UI_NONE;
	// We just downloaded from the Internet, so we should be able to check revocation
	trust_data.fdwRevocationChecks = WTD_REVOKE_WHOLECHAIN;
	// 0x400 = WTD_MOTW  for Windows 8.1 or later
	trust_data.dwProvFlags = WTD_REVOCATION_CHECK_CHAIN | 0x400;
	trust_data.dwUnionChoice = WTD_CHOICE_FILE;
	trust_data.pFile = &trust_file;

	r = WinVerifyTrustEx(INVALID_HANDLE_VALUE, &guid_generic_verify, &trust_data);
	safe_free(trust_file.pcwszFilePath);
	switch (r) {
	case ERROR_SUCCESS:
		// hDlg = INVALID_HANDLE_VALUE is used when validating the Fido PS1 script
		if (hDlg == INVALID_HANDLE_VALUE)
			break;
		// Verify that the timestamp of the downloaded update is in the future of our current one.
		// This is done to prevent the use of an officially signed, but older binary, as potential attack vector.
		current_ts = GetSignatureTimeStamp(NULL);
		if (current_ts == 0ULL) {
			uprintf("PKI: Cannot retrieve the current binary's timestamp - Aborting update");
			r = TRUST_E_TIME_STAMP;
		} else {
			update_ts = GetSignatureTimeStamp(path);
			if (update_ts < current_ts) {
				uprintf("PKI: Update timestamp (%" PRIi64 ") is younger than ours (%" PRIi64 ") - Aborting update", update_ts, current_ts);
				r = TRUST_E_TIME_STAMP;
			}
		}
		if ((r != ERROR_SUCCESS) && (force_update < 2))
			MessageBoxExU(hDlg, lmprintf(MSG_300), lmprintf(MSG_299), MB_OK | MB_ICONERROR | MB_IS_RTL, selected_langid);
		break;
	case TRUST_E_NOSIGNATURE:
		// Should already have been reported, but since we have a custom message for it...
		uprintf("PKI: File does not appear to be signed: %s", WinPKIErrorString());
		if (hDlg != INVALID_HANDLE_VALUE)
			MessageBoxExU(hDlg, lmprintf(MSG_284), lmprintf(MSG_283), MB_OK | MB_ICONERROR | MB_IS_RTL, selected_langid);
		break;
	default:
		uprintf("PKI: Failed to validate signature: %s", WinPKIErrorString());
		if (hDlg != INVALID_HANDLE_VALUE)
			MessageBoxExU(hDlg, lmprintf(MSG_240), lmprintf(MSG_283), MB_OK | MB_ICONERROR | MB_IS_RTL, selected_langid);
		break;
	}

	return r;
}