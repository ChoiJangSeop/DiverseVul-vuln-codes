pkcs11dsa_parse(dst_key_t *key, isc_lex_t *lexer, dst_key_t *pub) {
	dst_private_t priv;
	isc_result_t ret;
	int i;
	pk11_object_t *dsa = NULL;
	CK_ATTRIBUTE *attr;
	isc_mem_t *mctx = key->mctx;

	/* read private key file */
	ret = dst__privstruct_parse(key, DST_ALG_DSA, lexer, mctx, &priv);
	if (ret != ISC_R_SUCCESS)
		return (ret);

	if (key->external) {
		if (priv.nelements != 0)
			DST_RET(DST_R_INVALIDPRIVATEKEY);
		if (pub == NULL)
			DST_RET(DST_R_INVALIDPRIVATEKEY);

		key->keydata.pkey = pub->keydata.pkey;
		pub->keydata.pkey = NULL;
		key->key_size = pub->key_size;

		dst__privstruct_free(&priv, mctx);
		isc_safe_memwipe(&priv, sizeof(priv));

		return (ISC_R_SUCCESS);
	}

	dsa = (pk11_object_t *) isc_mem_get(key->mctx, sizeof(*dsa));
	if (dsa == NULL)
		DST_RET(ISC_R_NOMEMORY);
	memset(dsa, 0, sizeof(*dsa));
	key->keydata.pkey = dsa;

	dsa->repr = (CK_ATTRIBUTE *) isc_mem_get(key->mctx, sizeof(*attr) * 5);
	if (dsa->repr == NULL)
		DST_RET(ISC_R_NOMEMORY);
	memset(dsa->repr, 0, sizeof(*attr) * 5);
	dsa->attrcnt = 5;
	attr = dsa->repr;
	attr[0].type = CKA_PRIME;
	attr[1].type = CKA_SUBPRIME;
	attr[2].type = CKA_BASE;
	attr[3].type = CKA_VALUE;
	attr[4].type = CKA_VALUE2;

	for (i = 0; i < priv.nelements; i++) {
		CK_BYTE *bn;

		bn = isc_mem_get(key->mctx, priv.elements[i].length);
		if (bn == NULL)
			DST_RET(ISC_R_NOMEMORY);
		memmove(bn, priv.elements[i].data, priv.elements[i].length);

		switch (priv.elements[i].tag) {
			case TAG_DSA_PRIME:
				attr = pk11_attribute_bytype(dsa, CKA_PRIME);
				INSIST(attr != NULL);
				attr->pValue = bn;
				attr->ulValueLen = priv.elements[i].length;
				break;
			case TAG_DSA_SUBPRIME:
				attr = pk11_attribute_bytype(dsa,
							     CKA_SUBPRIME);
				INSIST(attr != NULL);
				attr->pValue = bn;
				attr->ulValueLen = priv.elements[i].length;
				break;
			case TAG_DSA_BASE:
				attr = pk11_attribute_bytype(dsa, CKA_BASE);
				INSIST(attr != NULL);
				attr->pValue = bn;
				attr->ulValueLen = priv.elements[i].length;
				break;
			case TAG_DSA_PRIVATE:
				attr = pk11_attribute_bytype(dsa, CKA_VALUE2);
				INSIST(attr != NULL);
				attr->pValue = bn;
				attr->ulValueLen = priv.elements[i].length;
				break;
			case TAG_DSA_PUBLIC:
				attr = pk11_attribute_bytype(dsa, CKA_VALUE);
				INSIST(attr != NULL);
				attr->pValue = bn;
				attr->ulValueLen = priv.elements[i].length;
				break;
		}
	}
	dst__privstruct_free(&priv, mctx);

	attr = pk11_attribute_bytype(dsa, CKA_PRIME);
	INSIST(attr != NULL);
	key->key_size = pk11_numbits(attr->pValue, attr->ulValueLen);

	return (ISC_R_SUCCESS);

 err:
	pkcs11dsa_destroy(key);
	dst__privstruct_free(&priv, mctx);
	isc_safe_memwipe(&priv, sizeof(priv));
	return (ret);
}