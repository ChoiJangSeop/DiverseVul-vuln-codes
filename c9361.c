pkcs11dh_fromdns(dst_key_t *key, isc_buffer_t *data) {
	pk11_object_t *dh = NULL;
	isc_region_t r;
	uint16_t plen, glen, plen_, glen_, publen;
	CK_BYTE *prime = NULL, *base = NULL, *pub = NULL;
	CK_ATTRIBUTE *attr;
	int special = 0;
	isc_result_t result;

	isc_buffer_remainingregion(data, &r);
	if (r.length == 0) {
		result = ISC_R_SUCCESS;
		goto cleanup;
	}

	dh = (pk11_object_t *) isc_mem_get(key->mctx, sizeof(*dh));
	if (dh == NULL) {
		result = ISC_R_NOMEMORY;
		goto cleanup;
	}

	memset(dh, 0, sizeof(*dh));
	result = DST_R_INVALIDPUBLICKEY;

	/*
	 * Read the prime length.  1 & 2 are table entries, > 16 means a
	 * prime follows, otherwise an error.
	 */
	if (r.length < 2)
		goto cleanup;

	plen = uint16_fromregion(&r);
	if (plen < 16 && plen != 1 && plen != 2)
		goto cleanup;

	if (r.length < plen)
		goto cleanup;

	plen_ = plen;
	if (plen == 1 || plen == 2) {
		if (plen == 1) {
			special = *r.base;
			isc_region_consume(&r, 1);
		} else {
			special = uint16_fromregion(&r);
		}
		switch (special) {
			case 1:
				prime = pk11_dh_bn768;
				plen_ = sizeof(pk11_dh_bn768);
				break;
			case 2:
				prime = pk11_dh_bn1024;
				plen_ = sizeof(pk11_dh_bn1024);
				break;
			case 3:
				prime = pk11_dh_bn1536;
				plen_ = sizeof(pk11_dh_bn1536);
				break;
			default:
				goto cleanup;
		}
	}
	else {
		prime = r.base;
		isc_region_consume(&r, plen);
	}

	/*
	 * Read the generator length.  This should be 0 if the prime was
	 * special, but it might not be.  If it's 0 and the prime is not
	 * special, we have a problem.
	 */
	if (r.length < 2)
		goto cleanup;

	glen = uint16_fromregion(&r);
	if (r.length < glen)
		goto cleanup;

	glen_ = glen;
	if (special != 0) {
		if (glen == 0) {
			base = pk11_dh_bn2;
			glen_ = sizeof(pk11_dh_bn2);
		}
		else {
			base = r.base;
			if (!isc_safe_memequal(base, pk11_dh_bn2, glen))
				goto cleanup;
			base = pk11_dh_bn2;
			glen_ = sizeof(pk11_dh_bn2);
		}
	}
	else {
		if (glen == 0)
			goto cleanup;
		base = r.base;
	}
	isc_region_consume(&r, glen);

	if (r.length < 2)
		goto cleanup;

	publen = uint16_fromregion(&r);
	if (r.length < publen)
		goto cleanup;

	pub = r.base;
	isc_region_consume(&r, publen);

	key->key_size = pk11_numbits(prime, plen_);

	dh->repr = (CK_ATTRIBUTE *) isc_mem_get(key->mctx, sizeof(*attr) * 3);
	if (dh->repr == NULL)
		goto nomemory;
	memset(dh->repr, 0, sizeof(*attr) * 3);
	dh->attrcnt = 3;

	attr = dh->repr;
	attr[0].type = CKA_PRIME;
	attr[0].pValue = isc_mem_get(key->mctx, plen_);
	if (attr[0].pValue == NULL)
		goto nomemory;
	memmove(attr[0].pValue, prime, plen_);
	attr[0].ulValueLen = (CK_ULONG) plen_;

	attr[1].type = CKA_BASE;
	attr[1].pValue = isc_mem_get(key->mctx, glen_);
	if (attr[1].pValue == NULL)
		goto nomemory;
	memmove(attr[1].pValue, base, glen_);
	attr[1].ulValueLen = (CK_ULONG) glen_;

	attr[2].type = CKA_VALUE;
	attr[2].pValue = isc_mem_get(key->mctx, publen);
	if (attr[2].pValue == NULL)
		goto nomemory;
	memmove(attr[2].pValue, pub, publen);
	attr[2].ulValueLen = (CK_ULONG) publen;

	isc_buffer_forward(data, plen + glen + publen + 6);

	key->keydata.pkey = dh;

	return (ISC_R_SUCCESS);

 nomemory:
	for (attr = pk11_attribute_first(dh);
	     attr != NULL;
	     attr = pk11_attribute_next(dh, attr))
		switch (attr->type) {
		case CKA_VALUE:
		case CKA_PRIME:
		case CKA_BASE:
			if (attr->pValue != NULL) {
				isc_safe_memwipe(attr->pValue,
						 attr->ulValueLen);
				isc_mem_put(key->mctx,
					    attr->pValue,
					    attr->ulValueLen);
			}
			break;
		}
	if (dh->repr != NULL) {
		isc_safe_memwipe(dh->repr, dh->attrcnt * sizeof(*attr));
		isc_mem_put(key->mctx, dh->repr, dh->attrcnt * sizeof(*attr));
	}

	result = ISC_R_NOMEMORY;

 cleanup:
	if (dh != NULL) {
		isc_safe_memwipe(dh, sizeof(*dh));
		isc_mem_put(key->mctx, dh, sizeof(*dh));
	}
	return (result);
}