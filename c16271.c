int fips_drbg_ec_init(DRBG_CTX *dctx)
	{
	const EVP_MD *md;
	const unsigned char *Q_x, *Q_y;
	BIGNUM *x, *y;
	size_t ptlen;
	int md_nid = dctx->type & 0xffff;
	int curve_nid = dctx->type >> 16;
	DRBG_EC_CTX *ectx = &dctx->d.ec;
	md = FIPS_get_digestbynid(md_nid);
	if (!md)
		return -2;

	/* These are taken from SP 800-90 10.3.1 table 4 */
	switch (curve_nid)
		{
		case NID_X9_62_prime256v1:
		dctx->strength = 128;
		dctx->seedlen = 32;
		dctx->blocklength = 30;
		ectx->exbits = 0;
		Q_x = p_256_qx;
		Q_y = p_256_qy;
		ptlen = sizeof(p_256_qx);
		break;

		case NID_secp384r1:
		if (md_nid == NID_sha1)
			return -2;
		dctx->strength = 192;
		dctx->seedlen = 48;
		dctx->blocklength = 46;
		ectx->exbits = 0;
		Q_x = p_384_qx;
		Q_y = p_384_qy;
		ptlen = sizeof(p_384_qx);
		break;

		case NID_secp521r1:
		if (md_nid == NID_sha1 || md_nid == NID_sha224)
			return -2;
		dctx->strength = 256;
		dctx->seedlen = 66;
		dctx->blocklength = 63;
		ectx->exbits = 7;
		Q_x = p_521_qx;
		Q_y = p_521_qy;
		ptlen = sizeof(p_521_qx);
		break;

		default:
		return -2;
		}

	dctx->iflags |= DRBG_CUSTOM_RESEED;
	dctx->reseed_counter = 0;
	dctx->instantiate = drbg_ec_instantiate;
	dctx->reseed = drbg_ec_reseed;
	dctx->generate = drbg_ec_generate;
	dctx->uninstantiate = drbg_ec_uninstantiate;

	ectx->md = md;
	EVP_MD_CTX_init(&ectx->mctx);

	dctx->min_entropy = dctx->strength / 8;
	dctx->max_entropy = 2 << 10;

	dctx->min_nonce = dctx->min_entropy / 2;
	dctx->max_nonce = 2 << 10;

	dctx->max_pers = 2 << 10;
	dctx->max_adin = 2 << 10;

	dctx->reseed_interval = 1<<24;
	dctx->max_request = dctx->reseed_interval * dctx->blocklength;

	/* Setup internal structures */
	ectx->bctx = BN_CTX_new();
	if (!ectx->bctx)
		return 0;
	BN_CTX_start(ectx->bctx);

	ectx->s = BN_new();

	ectx->curve = EC_GROUP_new_by_curve_name(curve_nid);

	ectx->Q = EC_POINT_new(ectx->curve);
	ectx->ptmp = EC_POINT_new(ectx->curve);

	x = BN_CTX_get(ectx->bctx);
	y = BN_CTX_get(ectx->bctx);

	if (!ectx->s || !ectx->curve || !ectx->Q || !y)
		goto err;

	if (!BN_bin2bn(Q_x, ptlen, x) || !BN_bin2bn(Q_y, ptlen, y))
		goto err;
	if (!EC_POINT_set_affine_coordinates_GFp(ectx->curve, ectx->Q,
							x, y, ectx->bctx))
		goto err;

	BN_CTX_end(ectx->bctx);

	return 1;
	err:
	BN_CTX_end(ectx->bctx);
	drbg_ec_uninstantiate(dctx);
	return 0;
	}