int ASN1_item_verify(const ASN1_ITEM *it, X509_ALGOR *a, ASN1_BIT_STRING *signature,
	     void *asn, EVP_PKEY *pkey)
	{
	EVP_MD_CTX ctx;
	const EVP_MD *type = NULL;
	unsigned char *buf_in=NULL;
	int ret= -1,inl;

	int mdnid, pknid;

	EVP_MD_CTX_init(&ctx);

	/* Convert signature OID into digest and public key OIDs */
	if (!OBJ_find_sigid_algs(OBJ_obj2nid(a->algorithm), &mdnid, &pknid))
		{
		ASN1err(ASN1_F_ASN1_ITEM_VERIFY,ASN1_R_UNKNOWN_SIGNATURE_ALGORITHM);
		goto err;
		}
	type=EVP_get_digestbynid(mdnid);
	if (type == NULL)
		{
		ASN1err(ASN1_F_ASN1_ITEM_VERIFY,ASN1_R_UNKNOWN_MESSAGE_DIGEST_ALGORITHM);
		goto err;
		}

	/* Check public key OID matches public key type */
	if (EVP_PKEY_type(pknid) != pkey->ameth->pkey_id)
		{
		ASN1err(ASN1_F_ASN1_ITEM_VERIFY,ASN1_R_WRONG_PUBLIC_KEY_TYPE);
		goto err;
		}

	if (!EVP_VerifyInit_ex(&ctx,type, NULL))
		{
		ASN1err(ASN1_F_ASN1_ITEM_VERIFY,ERR_R_EVP_LIB);
		ret=0;
		goto err;
		}

	inl = ASN1_item_i2d(asn, &buf_in, it);
	
	if (buf_in == NULL)
		{
		ASN1err(ASN1_F_ASN1_ITEM_VERIFY,ERR_R_MALLOC_FAILURE);
		goto err;
		}

	EVP_VerifyUpdate(&ctx,(unsigned char *)buf_in,inl);

	OPENSSL_cleanse(buf_in,(unsigned int)inl);
	OPENSSL_free(buf_in);

	if (EVP_VerifyFinal(&ctx,(unsigned char *)signature->data,
			(unsigned int)signature->length,pkey) <= 0)
		{
		ASN1err(ASN1_F_ASN1_ITEM_VERIFY,ERR_R_EVP_LIB);
		ret=0;
		goto err;
		}
	/* we don't need to zero the 'ctx' because we just checked
	 * public information */
	/* memset(&ctx,0,sizeof(ctx)); */
	ret=1;
err:
	EVP_MD_CTX_cleanup(&ctx);
	return(ret);
	}