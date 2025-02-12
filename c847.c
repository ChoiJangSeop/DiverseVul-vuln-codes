static int kvm_iommu_unmap_memslots(struct kvm *kvm)
{
	int idx;
	struct kvm_memslots *slots;
	struct kvm_memory_slot *memslot;

	idx = srcu_read_lock(&kvm->srcu);
	slots = kvm_memslots(kvm);

	kvm_for_each_memslot(memslot, slots)
		kvm_iommu_put_pages(kvm, memslot->base_gfn, memslot->npages);

	srcu_read_unlock(&kvm->srcu, idx);

	return 0;
}