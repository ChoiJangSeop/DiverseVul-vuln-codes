RSA *RSA_generate_key(int bits, unsigned long e_value,
	     void (*callback)(int,int,void *), void *cb_arg)
	{
	RSA *rsa=NULL;
	BIGNUM *r0=NULL,*r1=NULL,*r2=NULL,*r3=NULL,*tmp;
	int bitsp,bitsq,ok= -1,n=0,i;
	BN_CTX *ctx=NULL,*ctx2=NULL;

	ctx=BN_CTX_new();
	if (ctx == NULL) goto err;
	ctx2=BN_CTX_new();
	if (ctx2 == NULL) goto err;
	BN_CTX_start(ctx);
	r0 = BN_CTX_get(ctx);
	r1 = BN_CTX_get(ctx);
	r2 = BN_CTX_get(ctx);
	r3 = BN_CTX_get(ctx);
	if (r3 == NULL) goto err;

	bitsp=(bits+1)/2;
	bitsq=bits-bitsp;
	rsa=RSA_new();
	if (rsa == NULL) goto err;

	/* set e */ 
	rsa->e=BN_new();
	if (rsa->e == NULL) goto err;

#if 1
	/* The problem is when building with 8, 16, or 32 BN_ULONG,
	 * unsigned long can be larger */
	for (i=0; i<sizeof(unsigned long)*8; i++)
		{
		if (e_value & (1<<i))
			BN_set_bit(rsa->e,i);
		}
#else
	if (!BN_set_word(rsa->e,e_value)) goto err;
#endif

	/* generate p and q */
	for (;;)
		{
		rsa->p=BN_generate_prime(NULL,bitsp,0,NULL,NULL,callback,cb_arg);
		if (rsa->p == NULL) goto err;
		if (!BN_sub(r2,rsa->p,BN_value_one())) goto err;
		if (!BN_gcd(r1,r2,rsa->e,ctx)) goto err;
		if (BN_is_one(r1)) break;
		if (callback != NULL) callback(2,n++,cb_arg);
		BN_free(rsa->p);
		}
	if (callback != NULL) callback(3,0,cb_arg);
	for (;;)
		{
		rsa->q=BN_generate_prime(NULL,bitsq,0,NULL,NULL,callback,cb_arg);
		if (rsa->q == NULL) goto err;
		if (!BN_sub(r2,rsa->q,BN_value_one())) goto err;
		if (!BN_gcd(r1,r2,rsa->e,ctx)) goto err;
		if (BN_is_one(r1) && (BN_cmp(rsa->p,rsa->q) != 0))
			break;
		if (callback != NULL) callback(2,n++,cb_arg);
		BN_free(rsa->q);
		}
	if (callback != NULL) callback(3,1,cb_arg);
	if (BN_cmp(rsa->p,rsa->q) < 0)
		{
		tmp=rsa->p;
		rsa->p=rsa->q;
		rsa->q=tmp;
		}

	/* calculate n */
	rsa->n=BN_new();
	if (rsa->n == NULL) goto err;
	if (!BN_mul(rsa->n,rsa->p,rsa->q,ctx)) goto err;

	/* calculate d */
	if (!BN_sub(r1,rsa->p,BN_value_one())) goto err;	/* p-1 */
	if (!BN_sub(r2,rsa->q,BN_value_one())) goto err;	/* q-1 */
	if (!BN_mul(r0,r1,r2,ctx)) goto err;	/* (p-1)(q-1) */

/* should not be needed, since gcd(p-1,e) == 1 and gcd(q-1,e) == 1 */
/*	for (;;)
		{
		if (!BN_gcd(r3,r0,rsa->e,ctx)) goto err;
		if (BN_is_one(r3)) break;

		if (1)
			{
			if (!BN_add_word(rsa->e,2L)) goto err;
			continue;
			}
		RSAerr(RSA_F_RSA_GENERATE_KEY,RSA_R_BAD_E_VALUE);
		goto err;
		}
*/
	rsa->d=BN_mod_inverse(NULL,rsa->e,r0,ctx2);	/* d */
	if (rsa->d == NULL) goto err;

	/* calculate d mod (p-1) */
	rsa->dmp1=BN_new();
	if (rsa->dmp1 == NULL) goto err;
	if (!BN_mod(rsa->dmp1,rsa->d,r1,ctx)) goto err;

	/* calculate d mod (q-1) */
	rsa->dmq1=BN_new();
	if (rsa->dmq1 == NULL) goto err;
	if (!BN_mod(rsa->dmq1,rsa->d,r2,ctx)) goto err;

	/* calculate inverse of q mod p */
	rsa->iqmp=BN_mod_inverse(NULL,rsa->q,rsa->p,ctx2);
	if (rsa->iqmp == NULL) goto err;

	ok=1;
err:
	if (ok == -1)
		{
		RSAerr(RSA_F_RSA_GENERATE_KEY,ERR_LIB_BN);
		ok=0;
		}
	BN_CTX_end(ctx);
	BN_CTX_free(ctx);
	BN_CTX_free(ctx2);
	
	if (!ok)
		{
		if (rsa != NULL) RSA_free(rsa);
		return(NULL);
		}
	else
		return(rsa);
	}