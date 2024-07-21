static void calipso_doi_putdef(struct calipso_doi *doi_def)
{
	if (!doi_def)
		return;

	if (!refcount_dec_and_test(&doi_def->refcount))
		return;
	spin_lock(&calipso_doi_list_lock);
	list_del_rcu(&doi_def->list);
	spin_unlock(&calipso_doi_list_lock);

	call_rcu(&doi_def->rcu, calipso_doi_free_rcu);
}