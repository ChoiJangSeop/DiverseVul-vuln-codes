bool asn1_read_ContextSimple(struct asn1_data *data, uint8_t num, DATA_BLOB *blob)
{
	int len;
	ZERO_STRUCTP(blob);
	if (!asn1_start_tag(data, ASN1_CONTEXT_SIMPLE(num))) return false;
	len = asn1_tag_remaining(data);
	if (len < 0) {
		data->has_error = true;
		return false;
	}
	*blob = data_blob(NULL, len);
	if ((len != 0) && (!blob->data)) {
		data->has_error = true;
		return false;
	}
	asn1_read(data, blob->data, len);
	asn1_end_tag(data);
	return !data->has_error;
}