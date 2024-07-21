
static void kvm_destroy_vm(struct kvm *kvm)
{
	int i;
	struct mm_struct *mm = kvm->mm;

	kvm_destroy_pm_notifier(kvm);
	kvm_uevent_notify_change(KVM_EVENT_DESTROY_VM, kvm);
	kvm_destroy_vm_debugfs(kvm);
	kvm_arch_sync_events(kvm);
	mutex_lock(&kvm_lock);
	list_del(&kvm->vm_list);
	mutex_unlock(&kvm_lock);
	kvm_arch_pre_destroy_vm(kvm);

	kvm_free_irq_routing(kvm);
	for (i = 0; i < KVM_NR_BUSES; i++) {
		struct kvm_io_bus *bus = kvm_get_bus(kvm, i);

		if (bus)
			kvm_io_bus_destroy(bus);
		kvm->buses[i] = NULL;
	}
	kvm_coalesced_mmio_free(kvm);
#if defined(CONFIG_MMU_NOTIFIER) && defined(KVM_ARCH_WANT_MMU_NOTIFIER)
	mmu_notifier_unregister(&kvm->mmu_notifier, kvm->mm);
	/*
	 * At this point, pending calls to invalidate_range_start()
	 * have completed but no more MMU notifiers will run, so
	 * mn_active_invalidate_count may remain unbalanced.
	 * No threads can be waiting in install_new_memslots as the
	 * last reference on KVM has been dropped, but freeing
	 * memslots would deadlock without this manual intervention.
	 */
	WARN_ON(rcuwait_active(&kvm->mn_memslots_update_rcuwait));
	kvm->mn_active_invalidate_count = 0;
#else
	kvm_arch_flush_shadow_all(kvm);
#endif
	kvm_arch_destroy_vm(kvm);
	kvm_destroy_devices(kvm);
	for (i = 0; i < KVM_ADDRESS_SPACE_NUM; i++) {
		kvm_free_memslots(kvm, &kvm->__memslots[i][0]);
		kvm_free_memslots(kvm, &kvm->__memslots[i][1]);
	}
	cleanup_srcu_struct(&kvm->irq_srcu);
	cleanup_srcu_struct(&kvm->srcu);
	kvm_arch_free_vm(kvm);
	preempt_notifier_dec();
	hardware_disable_all();
	mmdrop(mm);
	module_put(kvm_chardev_ops.owner);