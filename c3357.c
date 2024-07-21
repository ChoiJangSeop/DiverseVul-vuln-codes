free_auth_mem(void)
{
	symkey *	sk;
	symkey_alloc *	alloc;
	symkey_alloc *	next_alloc;

	while (NULL != (sk = HEAD_DLIST(key_listhead, llink))) {
		freesymkey(sk, &key_hash[KEYHASH(sk->keyid)]);
	}
	free(key_hash);
	key_hash = NULL;
	cache_keyid = 0;
	cache_flags = 0;
	for (alloc = authallocs; alloc != NULL; alloc = next_alloc) {
		next_alloc = alloc->link;
		free(alloc->mem);	
	}
	authfreekeys = NULL;
	authnumfreekeys = 0;
}