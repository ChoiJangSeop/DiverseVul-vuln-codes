pkcs11rsa_createctx_verify(dst_key_t *key, unsigned int maxbits,
			   dst_context_t *dctx) {
	CK_RV rv;
	CK_MECHANISM mech = { 0, NULL, 0 };
	CK_OBJECT_CLASS keyClass = CKO_PUBLIC_KEY;
	CK_KEY_TYPE keyType = CKK_RSA;
	CK_ATTRIBUTE keyTemplate[] =
	{
		{ CKA_CLASS, &keyClass, (CK_ULONG) sizeof(keyClass) },
		{ CKA_KEY_TYPE, &keyType, (CK_ULONG) sizeof(keyType) },
		{ CKA_TOKEN, &falsevalue, (CK_ULONG) sizeof(falsevalue) },
		{ CKA_PRIVATE, &falsevalue, (CK_ULONG) sizeof(falsevalue) },
		{ CKA_VERIFY, &truevalue, (CK_ULONG) sizeof(truevalue) },
		{ CKA_MODULUS, NULL, 0 },
		{ CKA_PUBLIC_EXPONENT, NULL, 0 },
	};
	CK_ATTRIBUTE *attr;
	pk11_object_t *rsa;
	pk11_context_t *pk11_ctx;
	isc_result_t ret;
	unsigned int i;

#ifndef PK11_MD5_DISABLE
	REQUIRE(key->key_alg == DST_ALG_RSAMD5 ||
		key->key_alg == DST_ALG_RSASHA1 ||
		key->key_alg == DST_ALG_NSEC3RSASHA1 ||
		key->key_alg == DST_ALG_RSASHA256 ||
		key->key_alg == DST_ALG_RSASHA512);
#else
	REQUIRE(key->key_alg == DST_ALG_RSASHA1 ||
		key->key_alg == DST_ALG_NSEC3RSASHA1 ||
		key->key_alg == DST_ALG_RSASHA256 ||
		key->key_alg == DST_ALG_RSASHA512);
#endif

	/*
	 * Reject incorrect RSA key lengths.
	 */
	switch (dctx->key->key_alg) {
	case DST_ALG_RSAMD5:
	case DST_ALG_RSASHA1:
	case DST_ALG_NSEC3RSASHA1:
		/* From RFC 3110 */
		if (dctx->key->key_size > 4096)
			return (ISC_R_FAILURE);
		break;
	case DST_ALG_RSASHA256:
		/* From RFC 5702 */
		if ((dctx->key->key_size < 512) ||
		    (dctx->key->key_size > 4096))
			return (ISC_R_FAILURE);
		break;
	case DST_ALG_RSASHA512:
		/* From RFC 5702 */
		if ((dctx->key->key_size < 1024) ||
		    (dctx->key->key_size > 4096))
			return (ISC_R_FAILURE);
		break;
	default:
		INSIST(0);
		ISC_UNREACHABLE();
	}

	rsa = key->keydata.pkey;

	pk11_ctx = (pk11_context_t *) isc_mem_get(dctx->mctx,
						  sizeof(*pk11_ctx));
	if (pk11_ctx == NULL)
		return (ISC_R_NOMEMORY);
	ret = pk11_get_session(pk11_ctx, OP_RSA, true, false,
			       rsa->reqlogon, NULL,
			       pk11_get_best_token(OP_RSA));
	if (ret != ISC_R_SUCCESS)
		goto err;

	for (attr = pk11_attribute_first(rsa);
	     attr != NULL;
	     attr = pk11_attribute_next(rsa, attr))
		switch (attr->type) {
		case CKA_MODULUS:
			INSIST(keyTemplate[5].type == attr->type);
			keyTemplate[5].pValue = isc_mem_get(dctx->mctx,
							    attr->ulValueLen);
			if (keyTemplate[5].pValue == NULL)
				DST_RET(ISC_R_NOMEMORY);
			memmove(keyTemplate[5].pValue, attr->pValue,
				attr->ulValueLen);
			keyTemplate[5].ulValueLen = attr->ulValueLen;
			break;
		case CKA_PUBLIC_EXPONENT:
			INSIST(keyTemplate[6].type == attr->type);
			keyTemplate[6].pValue = isc_mem_get(dctx->mctx,
							    attr->ulValueLen);
			if (keyTemplate[6].pValue == NULL)
				DST_RET(ISC_R_NOMEMORY);
			memmove(keyTemplate[6].pValue, attr->pValue,
				attr->ulValueLen);
			keyTemplate[6].ulValueLen = attr->ulValueLen;
			if (pk11_numbits(attr->pValue,
					 attr->ulValueLen) > maxbits &&
			    maxbits != 0)
				DST_RET(DST_R_VERIFYFAILURE);
			break;
		}
	pk11_ctx->object = CK_INVALID_HANDLE;
	pk11_ctx->ontoken = false;
	PK11_RET(pkcs_C_CreateObject,
		 (pk11_ctx->session,
		  keyTemplate, (CK_ULONG) 7,
		  &pk11_ctx->object),
		 ISC_R_FAILURE);

	switch (dctx->key->key_alg) {
#ifndef PK11_MD5_DISABLE
	case DST_ALG_RSAMD5:
		mech.mechanism = CKM_MD5_RSA_PKCS;
		break;
#endif
	case DST_ALG_RSASHA1:
	case DST_ALG_NSEC3RSASHA1:
		mech.mechanism = CKM_SHA1_RSA_PKCS;
		break;
	case DST_ALG_RSASHA256:
		mech.mechanism = CKM_SHA256_RSA_PKCS;
		break;
	case DST_ALG_RSASHA512:
		mech.mechanism = CKM_SHA512_RSA_PKCS;
		break;
	default:
		INSIST(0);
		ISC_UNREACHABLE();
	}

	PK11_RET(pkcs_C_VerifyInit,
		 (pk11_ctx->session, &mech, pk11_ctx->object),
		 ISC_R_FAILURE);

	dctx->ctxdata.pk11_ctx = pk11_ctx;

	for (i = 5; i <= 6; i++)
		if (keyTemplate[i].pValue != NULL) {
			isc_safe_memwipe(keyTemplate[i].pValue,
					 keyTemplate[i].ulValueLen);
			isc_mem_put(dctx->mctx,
				    keyTemplate[i].pValue,
				    keyTemplate[i].ulValueLen);
		}

	return (ISC_R_SUCCESS);

    err:
	if (!pk11_ctx->ontoken && (pk11_ctx->object != CK_INVALID_HANDLE))
		(void) pkcs_C_DestroyObject(pk11_ctx->session,
					    pk11_ctx->object);
	for (i = 5; i <= 6; i++)
		if (keyTemplate[i].pValue != NULL) {
			isc_safe_memwipe(keyTemplate[i].pValue,
					 keyTemplate[i].ulValueLen);
			isc_mem_put(dctx->mctx,
				    keyTemplate[i].pValue,
				    keyTemplate[i].ulValueLen);
		}
	pk11_return_session(pk11_ctx);
	isc_safe_memwipe(pk11_ctx, sizeof(*pk11_ctx));
	isc_mem_put(dctx->mctx, pk11_ctx, sizeof(*pk11_ctx));

	return (ret);
}