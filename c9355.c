pkcs11dh_parse(dst_key_t *key, isc_lex_t *lexer, dst_key_t *pub) {
	dst_private_t priv;
	isc_result_t ret;
	int i;
	pk11_object_t *dh = NULL;
	CK_ATTRIBUTE *attr;
	isc_mem_t *mctx;

	UNUSED(pub);
	mctx = key->mctx;

	/* read private key file */
	ret = dst__privstruct_parse(key, DST_ALG_DH, lexer, mctx, &priv);
	if (ret != ISC_R_SUCCESS)
		return (ret);

	if (key->external)
		DST_RET(DST_R_EXTERNALKEY);

	dh = (pk11_object_t *) isc_mem_get(key->mctx, sizeof(*dh));
	if (dh == NULL)
		DST_RET(ISC_R_NOMEMORY);
	memset(dh, 0, sizeof(*dh));
	key->keydata.pkey = dh;
	dh->repr = (CK_ATTRIBUTE *) isc_mem_get(key->mctx, sizeof(*attr) * 4);
	if (dh->repr == NULL)
		DST_RET(ISC_R_NOMEMORY);
	memset(dh->repr, 0, sizeof(*attr) * 4);
	dh->attrcnt = 4;
	attr = dh->repr;
	attr[0].type = CKA_PRIME;
	attr[1].type = CKA_BASE;
	attr[2].type = CKA_VALUE;
	attr[3].type = CKA_VALUE2;

	for (i = 0; i < priv.nelements; i++) {
		CK_BYTE *bn;

		bn = isc_mem_get(key->mctx, priv.elements[i].length);
		if (bn == NULL)
			DST_RET(ISC_R_NOMEMORY);
		memmove(bn, priv.elements[i].data, priv.elements[i].length);

		switch (priv.elements[i].tag) {
			case TAG_DH_PRIME:
				attr = pk11_attribute_bytype(dh, CKA_PRIME);
				INSIST(attr != NULL);
				attr->pValue = bn;
				attr->ulValueLen = priv.elements[i].length;
				break;
			case TAG_DH_GENERATOR:
				attr = pk11_attribute_bytype(dh, CKA_BASE);
				INSIST(attr != NULL);
				attr->pValue = bn;
				attr->ulValueLen = priv.elements[i].length;
				break;
			case TAG_DH_PRIVATE:
				attr = pk11_attribute_bytype(dh, CKA_VALUE2);
				INSIST(attr != NULL);
				attr->pValue = bn;
				attr->ulValueLen = priv.elements[i].length;
				break;
			case TAG_DH_PUBLIC:
				attr = pk11_attribute_bytype(dh, CKA_VALUE);
				INSIST(attr != NULL);
				attr->pValue = bn;
				attr->ulValueLen = priv.elements[i].length;
				break;
		}
	}
	dst__privstruct_free(&priv, mctx);

	attr = pk11_attribute_bytype(dh, CKA_PRIME);
	INSIST(attr != NULL);
	key->key_size = pk11_numbits(attr->pValue, attr->ulValueLen);

	return (ISC_R_SUCCESS);

 err:
	pkcs11dh_destroy(key);
	dst__privstruct_free(&priv, mctx);
	isc_safe_memwipe(&priv, sizeof(priv));
	return (ret);
}