static int drbg_ec_mul(DRBG_EC_CTX *ectx, BIGNUM *r, const BIGNUM *s, int use_q)
	{
	if (use_q)
		{
		if (!EC_POINT_mul(ectx->curve, ectx->ptmp,
						NULL, ectx->Q, s, ectx->bctx))
			return 0;
		}
	else
		{
		if (!EC_POINT_mul(ectx->curve, ectx->ptmp,
						s, NULL, NULL, ectx->bctx))
			return 0;
		}
	/* Get x coordinate of result */
	if (!EC_POINT_get_affine_coordinates_GFp(ectx->curve, ectx->ptmp, r,
							NULL, ectx->bctx))
		return 0;
	return 1;
	}