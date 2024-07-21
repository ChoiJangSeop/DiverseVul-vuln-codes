void get_xsrf_token(const char *username, const char *pass,
		    const char *formname, time_t xsrf_time, char token_str[33])
{
	MD5_CTX md5_ctx;
	uint8_t token[16];
	int i;

	token_str[0] = '\0';
	ZERO_STRUCT(md5_ctx);
	MD5Init(&md5_ctx);

	MD5Update(&md5_ctx, (uint8_t *)formname, strlen(formname));
	MD5Update(&md5_ctx, (uint8_t *)&xsrf_time, sizeof(time_t));
	if (username != NULL) {
		MD5Update(&md5_ctx, (uint8_t *)username, strlen(username));
	}
	if (pass != NULL) {
		MD5Update(&md5_ctx, (uint8_t *)pass, strlen(pass));
	}

	MD5Final(token, &md5_ctx);

	for(i = 0; i < sizeof(token); i++) {
		char tmp[3];

		snprintf(tmp, sizeof(tmp), "%02x", token[i]);
		strlcat(token_str, tmp, sizeof(tmp));
	}
}