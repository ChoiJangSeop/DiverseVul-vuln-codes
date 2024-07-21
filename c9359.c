pkcs11rsa_fromlabel(dst_key_t *key, const char *engine, const char *label,
		    const char *pin)
{
	CK_RV rv;
	CK_OBJECT_HANDLE hKey = CK_INVALID_HANDLE;
	CK_OBJECT_CLASS keyClass = CKO_PUBLIC_KEY;
	CK_KEY_TYPE keyType = CKK_RSA;
	CK_ATTRIBUTE searchTemplate[] =
	{
		{ CKA_CLASS, &keyClass, (CK_ULONG) sizeof(keyClass) },
		{ CKA_KEY_TYPE, &keyType, (CK_ULONG) sizeof(keyType) },
		{ CKA_TOKEN, &truevalue, (CK_ULONG) sizeof(truevalue) },
		{ CKA_LABEL, NULL, 0 }
	};
	CK_ULONG cnt;
	CK_ATTRIBUTE *attr;
	pk11_object_t *rsa;
	pk11_context_t *pk11_ctx = NULL;
	isc_result_t ret;
	unsigned int i;

	UNUSED(pin);

	rsa = (pk11_object_t *) isc_mem_get(key->mctx, sizeof(*rsa));
	if (rsa == NULL)
		return (ISC_R_NOMEMORY);
	memset(rsa, 0, sizeof(*rsa));
	rsa->object = CK_INVALID_HANDLE;
	rsa->ontoken = true;
	rsa->reqlogon = true;
	key->keydata.pkey = rsa;

	rsa->repr = (CK_ATTRIBUTE *) isc_mem_get(key->mctx, sizeof(*attr) * 2);
	if (rsa->repr == NULL)
		DST_RET(ISC_R_NOMEMORY);
	memset(rsa->repr, 0, sizeof(*attr) * 2);
	rsa->attrcnt = 2;
	attr = rsa->repr;
	attr[0].type = CKA_MODULUS;
	attr[1].type = CKA_PUBLIC_EXPONENT;

	ret = pk11_parse_uri(rsa, label, key->mctx, OP_RSA);
	if (ret != ISC_R_SUCCESS)
		goto err;

	pk11_ctx = (pk11_context_t *) isc_mem_get(key->mctx,
						  sizeof(*pk11_ctx));
	if (pk11_ctx == NULL)
		DST_RET(ISC_R_NOMEMORY);
	ret = pk11_get_session(pk11_ctx, OP_RSA, true, false,
			       rsa->reqlogon, NULL, rsa->slot);
	if (ret != ISC_R_SUCCESS)
		goto err;

	attr = pk11_attribute_bytype(rsa, CKA_LABEL);
	if (attr == NULL) {
		attr = pk11_attribute_bytype(rsa, CKA_ID);
		INSIST(attr != NULL);
		searchTemplate[3].type = CKA_ID;
	}
	searchTemplate[3].pValue = attr->pValue;
	searchTemplate[3].ulValueLen = attr->ulValueLen;

	PK11_RET(pkcs_C_FindObjectsInit,
		 (pk11_ctx->session, searchTemplate, (CK_ULONG) 4),
		 DST_R_CRYPTOFAILURE);
	PK11_RET(pkcs_C_FindObjects,
		 (pk11_ctx->session, &hKey, (CK_ULONG) 1, &cnt),
		 DST_R_CRYPTOFAILURE);
	(void) pkcs_C_FindObjectsFinal(pk11_ctx->session);
	if (cnt == 0)
		DST_RET(ISC_R_NOTFOUND);
	if (cnt > 1)
		DST_RET(ISC_R_EXISTS);

	attr = rsa->repr;
	PK11_RET(pkcs_C_GetAttributeValue,
		 (pk11_ctx->session, hKey, attr, 2),
		 DST_R_CRYPTOFAILURE);
	for (i = 0; i <= 1; i++) {
		attr[i].pValue = isc_mem_get(key->mctx, attr[i].ulValueLen);
		if (attr[i].pValue == NULL)
			DST_RET(ISC_R_NOMEMORY);
		memset(attr[i].pValue, 0, attr[i].ulValueLen);
	}
	PK11_RET(pkcs_C_GetAttributeValue,
		 (pk11_ctx->session, hKey, attr, 2),
		 DST_R_CRYPTOFAILURE);

	keyClass = CKO_PRIVATE_KEY;
	PK11_RET(pkcs_C_FindObjectsInit,
		 (pk11_ctx->session, searchTemplate, (CK_ULONG) 4),
		 DST_R_CRYPTOFAILURE);
	PK11_RET(pkcs_C_FindObjects,
		 (pk11_ctx->session, &rsa->object, (CK_ULONG) 1, &cnt),
		 DST_R_CRYPTOFAILURE);
	(void) pkcs_C_FindObjectsFinal(pk11_ctx->session);
	if (cnt == 0)
		DST_RET(ISC_R_NOTFOUND);
	if (cnt > 1)
		DST_RET(ISC_R_EXISTS);

	if (engine != NULL) {
		key->engine = isc_mem_strdup(key->mctx, engine);
		if (key->engine == NULL)
			DST_RET(ISC_R_NOMEMORY);
	}

	key->label = isc_mem_strdup(key->mctx, label);
	if (key->label == NULL)
		DST_RET(ISC_R_NOMEMORY);

	attr = pk11_attribute_bytype(rsa, CKA_PUBLIC_EXPONENT);
	INSIST(attr != NULL);
	if (pk11_numbits(attr->pValue, attr->ulValueLen) > RSA_MAX_PUBEXP_BITS)
		DST_RET(ISC_R_RANGE);

	attr = pk11_attribute_bytype(rsa, CKA_MODULUS);
	INSIST(attr != NULL);
	key->key_size = pk11_numbits(attr->pValue, attr->ulValueLen);

	pk11_return_session(pk11_ctx);
	isc_safe_memwipe(pk11_ctx, sizeof(*pk11_ctx));
	isc_mem_put(key->mctx, pk11_ctx, sizeof(*pk11_ctx));

	return (ISC_R_SUCCESS);

    err:
	pkcs11rsa_destroy(key);
	if (pk11_ctx != NULL) {
		pk11_return_session(pk11_ctx);
		isc_safe_memwipe(pk11_ctx, sizeof(*pk11_ctx));
		isc_mem_put(key->mctx, pk11_ctx, sizeof(*pk11_ctx));
	}

	return (ret);
}