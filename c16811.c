static int ecdsa_sign_setup(EC_KEY *eckey, BN_CTX *ctx_in, BIGNUM **kinvp,
		BIGNUM **rp)
{
	BN_CTX   *ctx = NULL;
	BIGNUM	 *k = NULL, *r = NULL, *order = NULL, *X = NULL;
	EC_POINT *tmp_point=NULL;
	const EC_GROUP *group;
	int 	 ret = 0;

	if (eckey == NULL || (group = EC_KEY_get0_group(eckey)) == NULL)
	{
		ECDSAerr(ECDSA_F_ECDSA_SIGN_SETUP, ERR_R_PASSED_NULL_PARAMETER);
		return 0;
	}

	if (ctx_in == NULL) 
	{
		if ((ctx = BN_CTX_new()) == NULL)
		{
			ECDSAerr(ECDSA_F_ECDSA_SIGN_SETUP,ERR_R_MALLOC_FAILURE);
			return 0;
		}
	}
	else
		ctx = ctx_in;

	k     = BN_new();	/* this value is later returned in *kinvp */
	r     = BN_new();	/* this value is later returned in *rp    */
	order = BN_new();
	X     = BN_new();
	if (!k || !r || !order || !X)
	{
		ECDSAerr(ECDSA_F_ECDSA_SIGN_SETUP, ERR_R_MALLOC_FAILURE);
		goto err;
	}
	if ((tmp_point = EC_POINT_new(group)) == NULL)
	{
		ECDSAerr(ECDSA_F_ECDSA_SIGN_SETUP, ERR_R_EC_LIB);
		goto err;
	}
	if (!EC_GROUP_get_order(group, order, ctx))
	{
		ECDSAerr(ECDSA_F_ECDSA_SIGN_SETUP, ERR_R_EC_LIB);
		goto err;
	}
	
	do
	{
		/* get random k */	
		do
			if (!BN_rand_range(k, order))
			{
				ECDSAerr(ECDSA_F_ECDSA_SIGN_SETUP,
				 ECDSA_R_RANDOM_NUMBER_GENERATION_FAILED);	
				goto err;
			}
		while (BN_is_zero(k));

		/* We do not want timing information to leak the length of k,
		 * so we compute G*k using an equivalent scalar of fixed
		 * bit-length. */

		if (!BN_add(k, k, order)) goto err;
		if (BN_num_bits(k) <= BN_num_bits(order))
			if (!BN_add(k, k, order)) goto err;

		/* compute r the x-coordinate of generator * k */
		if (!EC_POINT_mul(group, tmp_point, k, NULL, NULL, ctx))
		{
			ECDSAerr(ECDSA_F_ECDSA_SIGN_SETUP, ERR_R_EC_LIB);
			goto err;
		}
		if (EC_METHOD_get_field_type(EC_GROUP_method_of(group)) == NID_X9_62_prime_field)
		{
			if (!EC_POINT_get_affine_coordinates_GFp(group,
				tmp_point, X, NULL, ctx))
			{
				ECDSAerr(ECDSA_F_ECDSA_SIGN_SETUP,ERR_R_EC_LIB);
				goto err;
			}
		}
#ifndef OPENSSL_NO_EC2M
		else /* NID_X9_62_characteristic_two_field */
		{
			if (!EC_POINT_get_affine_coordinates_GF2m(group,
				tmp_point, X, NULL, ctx))
			{
				ECDSAerr(ECDSA_F_ECDSA_SIGN_SETUP,ERR_R_EC_LIB);
				goto err;
			}
		}
#endif
		if (!BN_nnmod(r, X, order, ctx))
		{
			ECDSAerr(ECDSA_F_ECDSA_SIGN_SETUP, ERR_R_BN_LIB);
			goto err;
		}
	}
	while (BN_is_zero(r));

	/* compute the inverse of k */
	if (!BN_mod_inverse(k, k, order, ctx))
	{
		ECDSAerr(ECDSA_F_ECDSA_SIGN_SETUP, ERR_R_BN_LIB);
		goto err;	
	}
	/* clear old values if necessary */
	if (*rp != NULL)
		BN_clear_free(*rp);
	if (*kinvp != NULL) 
		BN_clear_free(*kinvp);
	/* save the pre-computed values  */
	*rp    = r;
	*kinvp = k;
	ret = 1;
err:
	if (!ret)
	{
		if (k != NULL) BN_clear_free(k);
		if (r != NULL) BN_clear_free(r);
	}
	if (ctx_in == NULL) 
		BN_CTX_free(ctx);
	if (order != NULL)
		BN_free(order);
	if (tmp_point != NULL) 
		EC_POINT_free(tmp_point);
	if (X)
		BN_clear_free(X);
	return(ret);
}