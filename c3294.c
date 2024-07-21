static void ldap_decode_attrib(TALLOC_CTX *mem_ctx, struct asn1_data *data,
			       struct ldb_message_element *attrib)
{
	asn1_start_tag(data, ASN1_SEQUENCE(0));
	asn1_read_OctetString_talloc(mem_ctx, data, &attrib->name);
	asn1_start_tag(data, ASN1_SET);
	while (asn1_peek_tag(data, ASN1_OCTET_STRING)) {
		DATA_BLOB blob;
		asn1_read_OctetString(data, mem_ctx, &blob);
		add_value_to_attrib(mem_ctx, &blob, attrib);
	}
	asn1_end_tag(data);
	asn1_end_tag(data);
	
}