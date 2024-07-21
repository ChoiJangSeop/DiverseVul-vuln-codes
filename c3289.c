static void ldap_decode_attribs(TALLOC_CTX *mem_ctx, struct asn1_data *data,
				struct ldb_message_element **attributes,
				int *num_attributes)
{
	asn1_start_tag(data, ASN1_SEQUENCE(0));
	ldap_decode_attribs_bare(mem_ctx, data, 
				 attributes, num_attributes);
	asn1_end_tag(data);
}