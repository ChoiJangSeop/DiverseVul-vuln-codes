bool asn1_check_enumerated(struct asn1_data *data, int v)
{
	uint8_t b;
	if (!asn1_start_tag(data, ASN1_ENUMERATED)) return false;
	asn1_read_uint8(data, &b);
	asn1_end_tag(data);

	if (v != b)
		data->has_error = false;

	return !data->has_error;
}