int kvm_iommu_map_guest(struct kvm *kvm)
{
	int r;

	if (!iommu_present(&pci_bus_type)) {
		printk(KERN_ERR "%s: iommu not found\n", __func__);
		return -ENODEV;
	}

	kvm->arch.iommu_domain = iommu_domain_alloc(&pci_bus_type);
	if (!kvm->arch.iommu_domain)
		return -ENOMEM;

	if (!allow_unsafe_assigned_interrupts &&
	    !iommu_domain_has_cap(kvm->arch.iommu_domain,
				  IOMMU_CAP_INTR_REMAP)) {
		printk(KERN_WARNING "%s: No interrupt remapping support,"
		       " disallowing device assignment."
		       " Re-enble with \"allow_unsafe_assigned_interrupts=1\""
		       " module option.\n", __func__);
		iommu_domain_free(kvm->arch.iommu_domain);
		kvm->arch.iommu_domain = NULL;
		return -EPERM;
	}

	r = kvm_iommu_map_memslots(kvm);
	if (r)
		goto out_unmap;

	return 0;

out_unmap:
	kvm_iommu_unmap_memslots(kvm);
	return r;
}