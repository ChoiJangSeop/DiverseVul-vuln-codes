EC_GROUP *EC_GROUP_new(const EC_METHOD *meth)
	{
	EC_GROUP *ret;

	if (meth == NULL)
		{
		ECerr(EC_F_EC_GROUP_NEW, EC_R_SLOT_FULL);
		return NULL;
		}
	if (meth->group_init == 0)
		{
		ECerr(EC_F_EC_GROUP_NEW, ERR_R_SHOULD_NOT_HAVE_BEEN_CALLED);
		return NULL;
		}

	ret = OPENSSL_malloc(sizeof *ret);
	if (ret == NULL)
		{
		ECerr(EC_F_EC_GROUP_NEW, ERR_R_MALLOC_FAILURE);
		return NULL;
		}

	ret->meth = meth;

	ret->extra_data = NULL;

	ret->generator = NULL;
	BN_init(&ret->order);
	BN_init(&ret->cofactor);

	ret->curve_name = 0;	
	ret->asn1_flag  = 0;
	ret->asn1_form  = POINT_CONVERSION_UNCOMPRESSED;

	ret->seed = NULL;
	ret->seed_len = 0;

	if (!meth->group_init(ret))
		{
		OPENSSL_free(ret);
		return NULL;
		}
	
	return ret;
	}