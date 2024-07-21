static void ecprint(FILE *out, EC_GROUP *grp, EC_POINT *pt)
	{
	BIGNUM *x, *y;
	x = BN_new();
	y = BN_new();
	EC_POINT_get_affine_coordinates_GFp(grp, pt, x, y, NULL);
	bnprint(out, "\tPoint X: ", x);
	bnprint(out, "\tPoint Y: ", y);
	BN_free(x);
	BN_free(y);
	}