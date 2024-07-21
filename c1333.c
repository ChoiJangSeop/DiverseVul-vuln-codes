void credssp_send(rdpCredssp* credssp)
{
	wStream* s;
	int length;
	int ts_request_length;
	int nego_tokens_length;
	int pub_key_auth_length;
	int auth_info_length;

	nego_tokens_length = (credssp->negoToken.cbBuffer > 0) ? credssp_skip_nego_tokens(credssp->negoToken.cbBuffer) : 0;
	pub_key_auth_length = (credssp->pubKeyAuth.cbBuffer > 0) ? credssp_skip_pub_key_auth(credssp->pubKeyAuth.cbBuffer) : 0;
	auth_info_length = (credssp->authInfo.cbBuffer > 0) ? credssp_skip_auth_info(credssp->authInfo.cbBuffer) : 0;

	length = nego_tokens_length + pub_key_auth_length + auth_info_length;
	ts_request_length = credssp_skip_ts_request(length);

	s = Stream_New(NULL, ts_request_length);

	/* TSRequest */
	length = der_get_content_length(ts_request_length);
	der_write_sequence_tag(s, length); /* SEQUENCE */

	/* [0] version */
	ber_write_contextual_tag(s, 0, 3, TRUE);
	ber_write_integer(s, 2); /* INTEGER */

	/* [1] negoTokens (NegoData) */
	if (nego_tokens_length > 0)
	{
		length = nego_tokens_length;
		length -= der_write_contextual_tag(s, 1, der_get_content_length(length), TRUE); /* NegoData */
		length -= der_write_sequence_tag(s, der_get_content_length(length)); /* SEQUENCE OF NegoDataItem */
		length -= der_write_sequence_tag(s, der_get_content_length(length)); /* NegoDataItem */
		length -= der_write_contextual_tag(s, 0, der_get_content_length(length), TRUE); /* [0] negoToken */
		der_write_octet_string(s, (BYTE*) credssp->negoToken.pvBuffer, credssp->negoToken.cbBuffer); /* OCTET STRING */
	}

	/* [2] authInfo (OCTET STRING) */
	if (auth_info_length > 0)
	{
		length = auth_info_length;
		length -= ber_write_contextual_tag(s, 2, ber_get_content_length(length), TRUE);
		ber_write_octet_string(s, credssp->authInfo.pvBuffer, credssp->authInfo.cbBuffer);
	}

	/* [3] pubKeyAuth (OCTET STRING) */
	if (pub_key_auth_length > 0)
	{
		length = pub_key_auth_length;
		length -= ber_write_contextual_tag(s, 3, ber_get_content_length(length), TRUE);
		ber_write_octet_string(s, credssp->pubKeyAuth.pvBuffer, credssp->pubKeyAuth.cbBuffer);
	}

	transport_write(credssp->transport, s);
	Stream_Free(s, TRUE);
}