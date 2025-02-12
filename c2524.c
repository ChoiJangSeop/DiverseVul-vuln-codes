static void destroy_super(struct super_block *s)
{
	int i;
	list_lru_destroy(&s->s_dentry_lru);
	list_lru_destroy(&s->s_inode_lru);
#ifdef CONFIG_SMP
	free_percpu(s->s_files);
#endif
	for (i = 0; i < SB_FREEZE_LEVELS; i++)
		percpu_counter_destroy(&s->s_writers.counter[i]);
	security_sb_free(s);
	WARN_ON(!list_empty(&s->s_mounts));
	kfree(s->s_subtype);
	kfree(s->s_options);
	kfree_rcu(s, rcu);
}