NTSTATUS asn1_full_tag(DATA_BLOB blob, uint8_t tag, size_t *packet_size)
{
	struct asn1_data *asn1 = asn1_init(NULL);
	int size;

	NT_STATUS_HAVE_NO_MEMORY(asn1);

	asn1->data = blob.data;
	asn1->length = blob.length;
	asn1_start_tag(asn1, tag);
	if (asn1->has_error) {
		talloc_free(asn1);
		return STATUS_MORE_ENTRIES;
	}
	size = asn1_tag_remaining(asn1) + asn1->ofs;

	talloc_free(asn1);

	if (size > blob.length) {
		return STATUS_MORE_ENTRIES;
	}

	*packet_size = size;
	return NT_STATUS_OK;
}