bool asn1_check_BOOLEAN(struct asn1_data *data, bool v)
{
	uint8_t b = 0;

	asn1_read_uint8(data, &b);
	if (b != ASN1_BOOLEAN) {
		data->has_error = true;
		return false;
	}
	asn1_read_uint8(data, &b);
	if (b != v) {
		data->has_error = true;
		return false;
	}
	return !data->has_error;
}