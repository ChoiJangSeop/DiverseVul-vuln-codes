struct ucounts *alloc_ucounts(struct user_namespace *ns, kuid_t uid)
{
	struct hlist_head *hashent = ucounts_hashentry(ns, uid);
	struct ucounts *ucounts, *new;
	bool wrapped;

	spin_lock_irq(&ucounts_lock);
	ucounts = find_ucounts(ns, uid, hashent);
	if (!ucounts) {
		spin_unlock_irq(&ucounts_lock);

		new = kzalloc(sizeof(*new), GFP_KERNEL);
		if (!new)
			return NULL;

		new->ns = ns;
		new->uid = uid;
		atomic_set(&new->count, 1);

		spin_lock_irq(&ucounts_lock);
		ucounts = find_ucounts(ns, uid, hashent);
		if (ucounts) {
			kfree(new);
		} else {
			hlist_add_head(&new->node, hashent);
			spin_unlock_irq(&ucounts_lock);
			return new;
		}
	}
	wrapped = !get_ucounts_or_wrap(ucounts);
	spin_unlock_irq(&ucounts_lock);
	if (wrapped) {
		put_ucounts(ucounts);
		return NULL;
	}
	return ucounts;
}