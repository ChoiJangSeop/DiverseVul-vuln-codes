int EC_GROUP_copy(EC_GROUP *dest, const EC_GROUP *src)
	{
	EC_EXTRA_DATA *d;

	if (dest->meth->group_copy == 0)
		{
		ECerr(EC_F_EC_GROUP_COPY, ERR_R_SHOULD_NOT_HAVE_BEEN_CALLED);
		return 0;
		}
	if (dest->meth != src->meth)
		{
		ECerr(EC_F_EC_GROUP_COPY, EC_R_INCOMPATIBLE_OBJECTS);
		return 0;
		}
	if (dest == src)
		return 1;
	
	EC_EX_DATA_free_all_data(&dest->extra_data);

	for (d = src->extra_data; d != NULL; d = d->next)
		{
		void *t = d->dup_func(d->data);
		
		if (t == NULL)
			return 0;
		if (!EC_EX_DATA_set_data(&dest->extra_data, t, d->dup_func, d->free_func, d->clear_free_func))
			return 0;
		}

	if (src->generator != NULL)
		{
		if (dest->generator == NULL)
			{
			dest->generator = EC_POINT_new(dest);
			if (dest->generator == NULL) return 0;
			}
		if (!EC_POINT_copy(dest->generator, src->generator)) return 0;
		}
	else
		{
		/* src->generator == NULL */
		if (dest->generator != NULL)
			{
			EC_POINT_clear_free(dest->generator);
			dest->generator = NULL;
			}
		}

	if (!BN_copy(&dest->order, &src->order)) return 0;
	if (!BN_copy(&dest->cofactor, &src->cofactor)) return 0;

	dest->curve_name = src->curve_name;
	dest->asn1_flag  = src->asn1_flag;
	dest->asn1_form  = src->asn1_form;

	if (src->seed)
		{
		if (dest->seed)
			OPENSSL_free(dest->seed);
		dest->seed = OPENSSL_malloc(src->seed_len);
		if (dest->seed == NULL)
			return 0;
		if (!memcpy(dest->seed, src->seed, src->seed_len))
			return 0;
		dest->seed_len = src->seed_len;
		}
	else
		{
		if (dest->seed)
			OPENSSL_free(dest->seed);
		dest->seed = NULL;
		dest->seed_len = 0;
		}
	

	return dest->meth->group_copy(dest, src);
	}