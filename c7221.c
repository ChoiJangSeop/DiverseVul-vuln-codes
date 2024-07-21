compute_tag(dns_name_t *name, dns_rdata_dnskey_t *dnskey, isc_mem_t *mctx,
	    dns_keytag_t *tag)
{
	isc_result_t result;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	unsigned char data[4096];
	isc_buffer_t buffer;
	dst_key_t *dstkey = NULL;

	isc_buffer_init(&buffer, data, sizeof(data));
	dns_rdata_fromstruct(&rdata, dnskey->common.rdclass,
			     dns_rdatatype_dnskey, dnskey, &buffer);

	result = dns_dnssec_keyfromrdata(name, &rdata, mctx, &dstkey);
	if (result == ISC_R_SUCCESS)
		*tag = dst_key_id(dstkey);
	dst_key_free(&dstkey);

	return (result);
}