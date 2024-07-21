pkcs11rsa_fromdns(dst_key_t *key, isc_buffer_t *data) {
	pk11_object_t *rsa;
	isc_region_t r;
	unsigned int e_bytes, mod_bytes;
	CK_BYTE *exponent = NULL, *modulus = NULL;
	CK_ATTRIBUTE *attr;
	unsigned int length;

	isc_buffer_remainingregion(data, &r);
	if (r.length == 0)
		return (ISC_R_SUCCESS);
	length = r.length;

	rsa = (pk11_object_t *) isc_mem_get(key->mctx, sizeof(*rsa));
	if (rsa == NULL)
		return (ISC_R_NOMEMORY);

	memset(rsa, 0, sizeof(*rsa));

	e_bytes = *r.base;
	isc_region_consume(&r, 1);

	if (e_bytes == 0) {
		if (r.length < 2) {
			isc_safe_memwipe(rsa, sizeof(*rsa));
			isc_mem_put(key->mctx, rsa, sizeof(*rsa));
			return (DST_R_INVALIDPUBLICKEY);
		}
		e_bytes = (*r.base) << 8;
		isc_region_consume(&r, 1);
		e_bytes += *r.base;
		isc_region_consume(&r, 1);
	}

	if (r.length < e_bytes) {
		isc_safe_memwipe(rsa, sizeof(*rsa));
		isc_mem_put(key->mctx, rsa, sizeof(*rsa));
		return (DST_R_INVALIDPUBLICKEY);
	}
	exponent = r.base;
	isc_region_consume(&r, e_bytes);
	modulus = r.base;
	mod_bytes = r.length;

	key->key_size = pk11_numbits(modulus, mod_bytes);

	isc_buffer_forward(data, length);

	rsa->repr = (CK_ATTRIBUTE *) isc_mem_get(key->mctx, sizeof(*attr) * 2);
	if (rsa->repr == NULL)
		goto nomemory;
	memset(rsa->repr, 0, sizeof(*attr) * 2);
	rsa->attrcnt = 2;
	attr = rsa->repr;
	attr[0].type = CKA_MODULUS;
	attr[0].pValue = isc_mem_get(key->mctx, mod_bytes);
	if (attr[0].pValue == NULL)
		goto nomemory;
	memmove(attr[0].pValue, modulus, mod_bytes);
	attr[0].ulValueLen = (CK_ULONG) mod_bytes;
	attr[1].type = CKA_PUBLIC_EXPONENT;
	attr[1].pValue = isc_mem_get(key->mctx, e_bytes);
	if (attr[1].pValue == NULL)
		goto nomemory;
	memmove(attr[1].pValue, exponent, e_bytes);
	attr[1].ulValueLen = (CK_ULONG) e_bytes;

	key->keydata.pkey = rsa;

	return (ISC_R_SUCCESS);

    nomemory:
	for (attr = pk11_attribute_first(rsa);
	     attr != NULL;
	     attr = pk11_attribute_next(rsa, attr))
		switch (attr->type) {
		case CKA_MODULUS:
		case CKA_PUBLIC_EXPONENT:
			if (attr->pValue != NULL) {
				isc_safe_memwipe(attr->pValue,
						 attr->ulValueLen);
				isc_mem_put(key->mctx,
					    attr->pValue,
					    attr->ulValueLen);
			}
			break;
		}
	if (rsa->repr != NULL) {
		isc_safe_memwipe(rsa->repr,
				 rsa->attrcnt * sizeof(*attr));
		isc_mem_put(key->mctx,
			    rsa->repr,
			    rsa->attrcnt * sizeof(*attr));
	}
	isc_safe_memwipe(rsa, sizeof(*rsa));
	isc_mem_put(key->mctx, rsa, sizeof(*rsa));
	return (ISC_R_NOMEMORY);
}