pkcs11rsa_parse(dst_key_t *key, isc_lex_t *lexer, dst_key_t *pub) {
	dst_private_t priv;
	isc_result_t ret;
	int i;
	pk11_object_t *rsa;
	CK_ATTRIBUTE *attr;
	isc_mem_t *mctx = key->mctx;
	const char *engine = NULL, *label = NULL;

	/* read private key file */
	ret = dst__privstruct_parse(key, DST_ALG_RSA, lexer, mctx, &priv);
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

	for (i = 0; i < priv.nelements; i++) {
		switch (priv.elements[i].tag) {
		case TAG_RSA_ENGINE:
			engine = (char *)priv.elements[i].data;
			break;
		case TAG_RSA_LABEL:
			label = (char *)priv.elements[i].data;
			break;
		default:
			break;
		}
	}
	rsa = (pk11_object_t *) isc_mem_get(key->mctx, sizeof(*rsa));
	if (rsa == NULL)
		DST_RET(ISC_R_NOMEMORY);
	memset(rsa, 0, sizeof(*rsa));
	key->keydata.pkey = rsa;

	/* Is this key is stored in a HSM? See if we can fetch it. */
	if ((label != NULL) || (engine != NULL)) {
		ret = pkcs11rsa_fetch(key, engine, label, pub);
		if (ret != ISC_R_SUCCESS)
			goto err;
		dst__privstruct_free(&priv, mctx);
		isc_safe_memwipe(&priv, sizeof(priv));
		return (ret);
	}

	rsa->repr = (CK_ATTRIBUTE *) isc_mem_get(key->mctx, sizeof(*attr) * 8);
	if (rsa->repr == NULL)
		DST_RET(ISC_R_NOMEMORY);
	memset(rsa->repr, 0, sizeof(*attr) * 8);
	rsa->attrcnt = 8;
	attr = rsa->repr;
	attr[0].type = CKA_MODULUS;
	attr[1].type = CKA_PUBLIC_EXPONENT;
	attr[2].type = CKA_PRIVATE_EXPONENT;
	attr[3].type = CKA_PRIME_1;
	attr[4].type = CKA_PRIME_2;
	attr[5].type = CKA_EXPONENT_1;
	attr[6].type = CKA_EXPONENT_2;
	attr[7].type = CKA_COEFFICIENT;

	for (i = 0; i < priv.nelements; i++) {
		CK_BYTE *bn;

		switch (priv.elements[i].tag) {
		case TAG_RSA_ENGINE:
			continue;
		case TAG_RSA_LABEL:
			continue;
		default:
			bn = isc_mem_get(key->mctx, priv.elements[i].length);
			if (bn == NULL)
				DST_RET(ISC_R_NOMEMORY);
			memmove(bn, priv.elements[i].data,
				priv.elements[i].length);
		}

		switch (priv.elements[i].tag) {
			case TAG_RSA_MODULUS:
				attr = pk11_attribute_bytype(rsa, CKA_MODULUS);
				INSIST(attr != NULL);
				attr->pValue = bn;
				attr->ulValueLen = priv.elements[i].length;
				break;
			case TAG_RSA_PUBLICEXPONENT:
				attr = pk11_attribute_bytype(rsa,
						CKA_PUBLIC_EXPONENT);
				INSIST(attr != NULL);
				attr->pValue = bn;
				attr->ulValueLen = priv.elements[i].length;
				break;
			case TAG_RSA_PRIVATEEXPONENT:
				attr = pk11_attribute_bytype(rsa,
						CKA_PRIVATE_EXPONENT);
				INSIST(attr != NULL);
				attr->pValue = bn;
				attr->ulValueLen = priv.elements[i].length;
				break;
			case TAG_RSA_PRIME1:
				attr = pk11_attribute_bytype(rsa, CKA_PRIME_1);
				INSIST(attr != NULL);
				attr->pValue = bn;
				attr->ulValueLen = priv.elements[i].length;
				break;
			case TAG_RSA_PRIME2:
				attr = pk11_attribute_bytype(rsa, CKA_PRIME_2);
				INSIST(attr != NULL);
				attr->pValue = bn;
				attr->ulValueLen = priv.elements[i].length;
				break;
			case TAG_RSA_EXPONENT1:
				attr = pk11_attribute_bytype(rsa,
							     CKA_EXPONENT_1);
				INSIST(attr != NULL);
				attr->pValue = bn;
				attr->ulValueLen = priv.elements[i].length;
				break;
			case TAG_RSA_EXPONENT2:
				attr = pk11_attribute_bytype(rsa,
							     CKA_EXPONENT_2);
				INSIST(attr != NULL);
				attr->pValue = bn;
				attr->ulValueLen = priv.elements[i].length;
				break;
			case TAG_RSA_COEFFICIENT:
				attr = pk11_attribute_bytype(rsa,
							     CKA_COEFFICIENT);
				INSIST(attr != NULL);
				attr->pValue = bn;
				attr->ulValueLen = priv.elements[i].length;
				break;
		}
	}

	if (rsa_check(rsa, pub->keydata.pkey) != ISC_R_SUCCESS)
		DST_RET(DST_R_INVALIDPRIVATEKEY);

	attr = pk11_attribute_bytype(rsa, CKA_MODULUS);
	INSIST(attr != NULL);
	key->key_size = pk11_numbits(attr->pValue, attr->ulValueLen);

	attr = pk11_attribute_bytype(rsa, CKA_PUBLIC_EXPONENT);
	INSIST(attr != NULL);
	if (pk11_numbits(attr->pValue, attr->ulValueLen) > RSA_MAX_PUBEXP_BITS)
		DST_RET(ISC_R_RANGE);

	dst__privstruct_free(&priv, mctx);
	isc_safe_memwipe(&priv, sizeof(priv));

	return (ISC_R_SUCCESS);

 err:
	pkcs11rsa_destroy(key);
	dst__privstruct_free(&priv, mctx);
	isc_safe_memwipe(&priv, sizeof(priv));
	return (ret);
}