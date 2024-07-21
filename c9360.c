pkcs11rsa_verify(dst_context_t *dctx, const isc_region_t *sig) {
	CK_RV rv;
	CK_MECHANISM mech = { CKM_RSA_PKCS, NULL, 0 };
	CK_OBJECT_HANDLE hKey = CK_INVALID_HANDLE;
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
	CK_BYTE digest[MAX_DER_SIZE + ISC_SHA512_DIGESTLENGTH];
	CK_BYTE *der;
	CK_ULONG derlen;
	CK_ULONG hashlen;
	CK_ULONG dgstlen;
	pk11_context_t *pk11_ctx = dctx->ctxdata.pk11_ctx;
	dst_key_t *key = dctx->key;
	pk11_object_t *rsa = key->keydata.pkey;
	isc_result_t ret = ISC_R_SUCCESS;
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
	REQUIRE(rsa != NULL);

	switch (key->key_alg) {
#ifndef PK11_MD5_DISABLE
	case DST_ALG_RSAMD5:
		der = md5_der;
		derlen = sizeof(md5_der);
		hashlen = ISC_MD5_DIGESTLENGTH;
		break;
#endif
	case DST_ALG_RSASHA1:
	case DST_ALG_NSEC3RSASHA1:
		der = sha1_der;
		derlen = sizeof(sha1_der);
		hashlen = ISC_SHA1_DIGESTLENGTH;
		break;
	case DST_ALG_RSASHA256:
		der = sha256_der;
		derlen = sizeof(sha256_der);
		hashlen = ISC_SHA256_DIGESTLENGTH;
		break;
	case DST_ALG_RSASHA512:
		der = sha512_der;
		derlen = sizeof(sha512_der);
		hashlen = ISC_SHA512_DIGESTLENGTH;
		break;
	default:
		INSIST(0);
		ISC_UNREACHABLE();
	}
	dgstlen = derlen + hashlen;
	INSIST(dgstlen <= sizeof(digest));
	memmove(digest, der, derlen);

	PK11_RET(pkcs_C_DigestFinal,
		 (pk11_ctx->session, digest + derlen, &hashlen),
		 DST_R_SIGNFAILURE);

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
					 attr->ulValueLen)
				> RSA_MAX_PUBEXP_BITS)
				DST_RET(DST_R_VERIFYFAILURE);
			break;
		}
	pk11_ctx->object = CK_INVALID_HANDLE;
	pk11_ctx->ontoken = false;
	PK11_RET(pkcs_C_CreateObject,
		 (pk11_ctx->session,
		  keyTemplate, (CK_ULONG) 7,
		  &hKey),
		 ISC_R_FAILURE);

	PK11_RET(pkcs_C_VerifyInit,
		 (pk11_ctx->session, &mech, hKey),
		 ISC_R_FAILURE);

	PK11_RET(pkcs_C_Verify,
		 (pk11_ctx->session,
		  digest, dgstlen,
		  (CK_BYTE_PTR) sig->base, (CK_ULONG) sig->length),
		 DST_R_VERIFYFAILURE);

    err:
	if (hKey != CK_INVALID_HANDLE)
		(void) pkcs_C_DestroyObject(pk11_ctx->session, hKey);
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
	dctx->ctxdata.pk11_ctx = NULL;

	return (ret);
}