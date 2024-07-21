create_jwt_token_fields(const char *algo, time_t exp, time_t iat, time_t nbf,
			ARRAY_TYPE(oauth2_field) *fields)
{
	const struct oauth2_field *field;
	buffer_t *tokenbuf = t_buffer_create(64);

	base64url_encode_str(
		t_strdup_printf("{\"alg\":\"%s\",\"typ\":\"JWT\"}", algo),
		tokenbuf);
	buffer_append(tokenbuf, ".", 1);

	string_t *bodybuf = t_str_new(64);
	str_append_c(bodybuf, '{');
	if (exp > 0) {
		append_key_value(bodybuf, "exp", dec2str(exp), FALSE);
	}
	if (iat > 0) {
		if (exp > 0)
			str_append_c(bodybuf, ',');
		append_key_value(bodybuf, "iat", dec2str(iat), FALSE);
	}
	if (nbf > 0) {
		if (exp > 0 || iat > 0)
			str_append_c(bodybuf, ',');
		append_key_value(bodybuf, "nbf", dec2str(nbf), FALSE);
	}
	array_foreach(fields, field) {
		if (str_data(bodybuf)[bodybuf->used-1] != '{')
			str_append_c(bodybuf, ',');
		append_key_value(bodybuf, field->name, field->value, TRUE);
	}
	str_append_c(bodybuf, '}');
	base64url_encode_str(str_c(bodybuf), tokenbuf);

	return tokenbuf;
}