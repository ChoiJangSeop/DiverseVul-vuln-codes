bool asn1_read_LDAPString(struct asn1_data *data, TALLOC_CTX *mem_ctx, char **s)
{
	int len;
	len = asn1_tag_remaining(data);
	if (len < 0) {
		data->has_error = true;
		return false;
	}
	*s = talloc_array(mem_ctx, char, len+1);
	if (! *s) {
		data->has_error = true;
		return false;
	}
	asn1_read(data, *s, len);
	(*s)[len] = 0;
	return !data->has_error;
}