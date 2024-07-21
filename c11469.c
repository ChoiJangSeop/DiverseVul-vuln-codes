static int u32_destroy_key(struct tc_u_knode *n, bool free_pf)
{
	struct tc_u_hnode *ht = rtnl_dereference(n->ht_down);

	tcf_exts_destroy(&n->exts);
	tcf_exts_put_net(&n->exts);
	if (ht && --ht->refcnt == 0)
		kfree(ht);
#ifdef CONFIG_CLS_U32_PERF
	if (free_pf)
		free_percpu(n->pf);
#endif
#ifdef CONFIG_CLS_U32_MARK
	if (free_pf)
		free_percpu(n->pcpu_success);
#endif
	kfree(n);
	return 0;
}