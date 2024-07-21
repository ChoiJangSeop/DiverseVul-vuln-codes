void cipso_v4_doi_putdef(struct cipso_v4_doi *doi_def)
{
	if (!doi_def)
		return;

	if (!refcount_dec_and_test(&doi_def->refcount))
		return;
	spin_lock(&cipso_v4_doi_list_lock);
	list_del_rcu(&doi_def->list);
	spin_unlock(&cipso_v4_doi_list_lock);

	cipso_v4_cache_invalidate();
	call_rcu(&doi_def->rcu, cipso_v4_doi_free_rcu);
}