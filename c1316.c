void credssp_encode_ts_credentials(rdpCredssp* credssp)
{
	wStream* s;
	int length;

	length = credssp_skip_ts_credentials(credssp);
	sspi_SecBufferAlloc(&credssp->ts_credentials, length);

	s = Stream_New(credssp->ts_credentials.pvBuffer, length);
	credssp_write_ts_credentials(credssp, s);

	Stream_Free(s, FALSE);
}