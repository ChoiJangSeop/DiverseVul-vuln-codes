void psi_trigger_replace(void **trigger_ptr, struct psi_trigger *new)
{
	struct psi_trigger *old = *trigger_ptr;

	if (static_branch_likely(&psi_disabled))
		return;

	rcu_assign_pointer(*trigger_ptr, new);
	if (old)
		kref_put(&old->refcount, psi_trigger_destroy);
}