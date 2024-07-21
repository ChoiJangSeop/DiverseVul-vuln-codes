soup_ntlm_response (const char *nonce, 
		    const char *user,
		    guchar      nt_hash[21],
		    guchar      lm_hash[21],
		    const char *host, 
		    const char *domain,
		    gboolean    ntlmv2_session)
{
	int offset;
	gsize hlen, dlen, ulen;
	guchar lm_resp[24], nt_resp[24];
	char *user_conv, *host_conv, *domain_conv;
	NTLMResponse resp;
	char *out, *p;
	int state, save;

	if (ntlmv2_session) {
		calc_ntlm2_session_response (nonce, nt_hash, lm_hash,
					     lm_resp, sizeof(lm_resp), nt_resp);
	} else {
		/* Compute a regular response */
		calc_response (nt_hash, (guchar *) nonce, nt_resp);
		calc_response (lm_hash, (guchar *) nonce, lm_resp);
	}

	memset (&resp, 0, sizeof (resp));
	memcpy (resp.header, NTLM_RESPONSE_HEADER, sizeof (resp.header));
	resp.flags = GUINT32_TO_LE (NTLM_RESPONSE_FLAGS);
	if (ntlmv2_session)
		resp.flags |= GUINT32_TO_LE (NTLMSSP_NEGOTIATE_EXTENDED_SESSIONSECURITY);

	offset = sizeof (resp);

	if (!host)
		host = "UNKNOWN";

	domain_conv = g_convert (domain, -1, "UCS-2LE", "UTF-8", NULL, &dlen, NULL);
	user_conv = g_convert (user, -1, "UCS-2LE", "UTF-8", NULL, &ulen, NULL);
	host_conv = g_convert (host, -1, "UCS-2LE", "UTF-8", NULL, &hlen, NULL);

	ntlm_set_string (&resp.domain, &offset, dlen);
	ntlm_set_string (&resp.user, &offset, ulen);
	ntlm_set_string (&resp.host, &offset, hlen);
	ntlm_set_string (&resp.lm_resp, &offset, sizeof (lm_resp));
	ntlm_set_string (&resp.nt_resp, &offset, sizeof (nt_resp));

	out = g_malloc (((offset + 3) * 4) / 3 + 6);
	memcpy (out, "NTLM ", 5);
	p = out + 5;

	state = save = 0;

	p += g_base64_encode_step ((const guchar *) &resp, sizeof (resp), 
				   FALSE, p, &state, &save);
	p += g_base64_encode_step ((const guchar *) domain_conv, dlen, 
				   FALSE, p, &state, &save);
	p += g_base64_encode_step ((const guchar *) user_conv, ulen, 
				   FALSE, p, &state, &save);
	p += g_base64_encode_step ((const guchar *) host_conv, hlen, 
				   FALSE, p, &state, &save);
	p += g_base64_encode_step (lm_resp, sizeof (lm_resp), 
				   FALSE, p, &state, &save);
	p += g_base64_encode_step (nt_resp, sizeof (nt_resp), 
				   FALSE, p, &state, &save);
	p += g_base64_encode_close (FALSE, p, &state, &save);
	*p = '\0';

	g_free (domain_conv);
	g_free (user_conv);
	g_free (host_conv);

	return out;
}