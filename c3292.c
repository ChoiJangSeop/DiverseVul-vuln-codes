void ldap_decode_attribs_bare(TALLOC_CTX *mem_ctx, struct asn1_data *data,
			      struct ldb_message_element **attributes,
			      int *num_attributes)
{
	while (asn1_peek_tag(data, ASN1_SEQUENCE(0))) {
		struct ldb_message_element attrib;
		ZERO_STRUCT(attrib);
		ldap_decode_attrib(mem_ctx, data, &attrib);
		add_attrib_to_array_talloc(mem_ctx, &attrib,
					   attributes, num_attributes);
	}
}