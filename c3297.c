static void ldap_decode_response(TALLOC_CTX *mem_ctx,
				 struct asn1_data *data,
				 struct ldap_Result *result)
{
	asn1_read_enumerated(data, &result->resultcode);
	asn1_read_OctetString_talloc(mem_ctx, data, &result->dn);
	asn1_read_OctetString_talloc(mem_ctx, data, &result->errormessage);
	if (asn1_peek_tag(data, ASN1_CONTEXT(3))) {
		asn1_start_tag(data, ASN1_CONTEXT(3));
		asn1_read_OctetString_talloc(mem_ctx, data, &result->referral);
		asn1_end_tag(data);
	} else {
		result->referral = NULL;
	}
}