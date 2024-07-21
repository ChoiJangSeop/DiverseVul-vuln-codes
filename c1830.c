void inet_frags_init_net(struct netns_frags *nf)
{
	nf->nqueues = 0;
	init_frag_mem_limit(nf);
	INIT_LIST_HEAD(&nf->lru_list);
}