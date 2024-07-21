bool asn1_read_OID(struct asn1_data *data, TALLOC_CTX *mem_ctx, char **OID)
{
	DATA_BLOB blob;
	int len;

	if (!asn1_start_tag(data, ASN1_OID)) return false;

	len = asn1_tag_remaining(data);
	if (len < 0) {
		data->has_error = true;
		return false;
	}

	blob = data_blob(NULL, len);
	if (!blob.data) {
		data->has_error = true;
		return false;
	}

	asn1_read(data, blob.data, len);
	asn1_end_tag(data);
	if (data->has_error) {
		data_blob_free(&blob);
		return false;
	}

	if (!ber_read_OID_String(mem_ctx, blob, OID)) {
		data->has_error = true;
		data_blob_free(&blob);
		return false;
	}

	data_blob_free(&blob);
	return true;
}