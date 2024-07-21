
static void kvm_invalidate_memslot(struct kvm *kvm,
				   struct kvm_memory_slot *old,
				   struct kvm_memory_slot *invalid_slot)
{
	/*
	 * Mark the current slot INVALID.  As with all memslot modifications,
	 * this must be done on an unreachable slot to avoid modifying the
	 * current slot in the active tree.
	 */
	kvm_copy_memslot(invalid_slot, old);
	invalid_slot->flags |= KVM_MEMSLOT_INVALID;
	kvm_replace_memslot(kvm, old, invalid_slot);

	/*
	 * Activate the slot that is now marked INVALID, but don't propagate
	 * the slot to the now inactive slots. The slot is either going to be
	 * deleted or recreated as a new slot.
	 */
	kvm_swap_active_memslots(kvm, old->as_id);

	/*
	 * From this point no new shadow pages pointing to a deleted, or moved,
	 * memslot will be created.  Validation of sp->gfn happens in:
	 *	- gfn_to_hva (kvm_read_guest, gfn_to_pfn)
	 *	- kvm_is_visible_gfn (mmu_check_root)
	 */
	kvm_arch_flush_shadow_memslot(kvm, old);

	/* Was released by kvm_swap_active_memslots, reacquire. */
	mutex_lock(&kvm->slots_arch_lock);

	/*
	 * Copy the arch-specific field of the newly-installed slot back to the
	 * old slot as the arch data could have changed between releasing
	 * slots_arch_lock in install_new_memslots() and re-acquiring the lock
	 * above.  Writers are required to retrieve memslots *after* acquiring
	 * slots_arch_lock, thus the active slot's data is guaranteed to be fresh.
	 */
	old->arch = invalid_slot->arch;