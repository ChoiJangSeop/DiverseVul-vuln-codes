void sev_vm_destroy(struct kvm *kvm)
{
	struct kvm_sev_info *sev = &to_kvm_svm(kvm)->sev_info;
	struct list_head *head = &sev->regions_list;
	struct list_head *pos, *q;

	if (!sev_guest(kvm))
		return;

	mutex_lock(&kvm->lock);

	/*
	 * Ensure that all guest tagged cache entries are flushed before
	 * releasing the pages back to the system for use. CLFLUSH will
	 * not do this, so issue a WBINVD.
	 */
	wbinvd_on_all_cpus();

	/*
	 * if userspace was terminated before unregistering the memory regions
	 * then lets unpin all the registered memory.
	 */
	if (!list_empty(head)) {
		list_for_each_safe(pos, q, head) {
			__unregister_enc_region_locked(kvm,
				list_entry(pos, struct enc_region, list));
		}
	}

	mutex_unlock(&kvm->lock);

	sev_unbind_asid(kvm, sev->handle);
	sev_asid_free(sev->asid);
}