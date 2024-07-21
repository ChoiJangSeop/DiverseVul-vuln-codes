allocsymkey(
	symkey **	bucket,
	keyid_t		id,
	u_short		flags,
	u_short		type,
	u_long		lifetime,
	u_short		secretsize,
	u_char *	secret
	)
{
	symkey *	sk;

	if (authnumfreekeys < 1)
		auth_moremem(-1);
	UNLINK_HEAD_SLIST(sk, authfreekeys, llink.f);
	DEBUG_ENSURE(sk != NULL);
	sk->keyid = id;
	sk->flags = flags;
	sk->type = type;
	sk->secretsize = secretsize;
	sk->secret = secret;
	sk->lifetime = lifetime;
	LINK_SLIST(*bucket, sk, hlink);
	LINK_TAIL_DLIST(key_listhead, sk, llink);
	authnumfreekeys--;
	authnumkeys++;
}