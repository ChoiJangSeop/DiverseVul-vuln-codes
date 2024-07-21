static unsigned int matches(const gnutls_datum_t *raw1, const gnutls_datum_t *raw2, 
							dane_match_type_t match)
{
uint8_t digest[64];
int ret;

	if (match == DANE_MATCH_EXACT) {
		if (raw1->size != raw2->size)
			return gnutls_assert_val(0);

		if (memcmp(raw1->data, raw2->data, raw1->size) != 0)
			return gnutls_assert_val(0);
		
		return 1;
	} else if (match == DANE_MATCH_SHA2_256) {

		if (raw2->size != 32)
			return gnutls_assert_val(0);
		
		ret = gnutls_hash_fast(GNUTLS_DIG_SHA256, raw1->data, raw1->size, digest);
		if (ret < 0)
			return gnutls_assert_val(0);

		if (memcmp(digest, raw2->data, 32) != 0)
			return gnutls_assert_val(0);
		
		return 1;
	} else if (match == DANE_MATCH_SHA2_512) {
		if (raw2->size != 64)
			return gnutls_assert_val(0);
		
		ret = gnutls_hash_fast(GNUTLS_DIG_SHA512, raw1->data, raw1->size, digest);
		if (ret < 0)
			return gnutls_assert_val(0);
		
		if (memcmp(digest, raw2->data, 64) != 0)
			return gnutls_assert_val(0);
		
		return 1;
	}
	
	return gnutls_assert_val(0);
}