int BN_mul(BIGNUM *r, const BIGNUM *a, const BIGNUM *b, BN_CTX *ctx)
	{
	int ret=0;
	int top,al,bl;
	BIGNUM *rr;
#if defined(BN_MUL_COMBA) || defined(BN_RECURSION)
	int i;
#endif
#ifdef BN_RECURSION
	BIGNUM *t=NULL;
	int j=0,k;
#endif

#ifdef BN_COUNT
	fprintf(stderr,"BN_mul %d * %d\n",a->top,b->top);
#endif

	bn_check_top(a);
	bn_check_top(b);
	bn_check_top(r);

	al=a->top;
	bl=b->top;

	if ((al == 0) || (bl == 0))
		{
		BN_zero(r);
		return(1);
		}
	top=al+bl;

	BN_CTX_start(ctx);
	if ((r == a) || (r == b))
		{
		if ((rr = BN_CTX_get(ctx)) == NULL) goto err;
		}
	else
		rr = r;
	rr->neg=a->neg^b->neg;

#if defined(BN_MUL_COMBA) || defined(BN_RECURSION)
	i = al-bl;
#endif
#ifdef BN_MUL_COMBA
	if (i == 0)
		{
# if 0
		if (al == 4)
			{
			if (bn_wexpand(rr,8) == NULL) goto err;
			rr->top=8;
			bn_mul_comba4(rr->d,a->d,b->d);
			goto end;
			}
# endif
		if (al == 8)
			{
			if (bn_wexpand(rr,16) == NULL) goto err;
			rr->top=16;
			bn_mul_comba8(rr->d,a->d,b->d);
			goto end;
			}
		}
#endif /* BN_MUL_COMBA */
#ifdef BN_RECURSION
	if ((al >= BN_MULL_SIZE_NORMAL) && (bl >= BN_MULL_SIZE_NORMAL))
		{
		if (i >= -1 && i <= 1)
			{
			int sav_j =0;
			/* Find out the power of two lower or equal
			   to the longest of the two numbers */
			if (i >= 0)
				{
				j = BN_num_bits_word((BN_ULONG)al);
				}
			if (i == -1)
				{
				j = BN_num_bits_word((BN_ULONG)bl);
				}
			sav_j = j;
			j = 1<<(j-1);
			assert(j <= al || j <= bl);
			k = j+j;
			t = BN_CTX_get(ctx);
			if (t == NULL)
				goto err;
			if (al > j || bl > j)
				{
				bn_wexpand(t,k*4);
				bn_wexpand(rr,k*4);
				bn_mul_part_recursive(rr->d,a->d,b->d,
					j,al-j,bl-j,t->d);
				}
			else	/* al <= j || bl <= j */
				{
				bn_wexpand(t,k*2);
				bn_wexpand(rr,k*2);
				bn_mul_recursive(rr->d,a->d,b->d,
					j,al-j,bl-j,t->d);
				}
			rr->top=top;
			goto end;
			}
#if 0
		if (i == 1 && !BN_get_flags(b,BN_FLG_STATIC_DATA))
			{
			BIGNUM *tmp_bn = (BIGNUM *)b;
			if (bn_wexpand(tmp_bn,al) == NULL) goto err;
			tmp_bn->d[bl]=0;
			bl++;
			i--;
			}
		else if (i == -1 && !BN_get_flags(a,BN_FLG_STATIC_DATA))
			{
			BIGNUM *tmp_bn = (BIGNUM *)a;
			if (bn_wexpand(tmp_bn,bl) == NULL) goto err;
			tmp_bn->d[al]=0;
			al++;
			i++;
			}
		if (i == 0)
			{
			/* symmetric and > 4 */
			/* 16 or larger */
			j=BN_num_bits_word((BN_ULONG)al);
			j=1<<(j-1);
			k=j+j;
			t = BN_CTX_get(ctx);
			if (al == j) /* exact multiple */
				{
				if (bn_wexpand(t,k*2) == NULL) goto err;
				if (bn_wexpand(rr,k*2) == NULL) goto err;
				bn_mul_recursive(rr->d,a->d,b->d,al,t->d);
				}
			else
				{
				if (bn_wexpand(t,k*4) == NULL) goto err;
				if (bn_wexpand(rr,k*4) == NULL) goto err;
				bn_mul_part_recursive(rr->d,a->d,b->d,al-j,j,t->d);
				}
			rr->top=top;
			goto end;
			}
#endif
		}
#endif /* BN_RECURSION */
	if (bn_wexpand(rr,top) == NULL) goto err;
	rr->top=top;
	bn_mul_normal(rr->d,a->d,al,b->d,bl);

#if defined(BN_MUL_COMBA) || defined(BN_RECURSION)
end:
#endif
	bn_correct_top(rr);
	if (r != rr) BN_copy(r,rr);
	ret=1;
err:
	bn_check_top(r);
	BN_CTX_end(ctx);
	return(ret);
	}