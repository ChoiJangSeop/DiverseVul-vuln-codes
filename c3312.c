bool asn1_write_enumerated(struct asn1_data *data, uint8_t v)
{
	if (!asn1_push_tag(data, ASN1_ENUMERATED)) return false;
	asn1_write_uint8(data, v);
	asn1_pop_tag(data);
	return !data->has_error;
}