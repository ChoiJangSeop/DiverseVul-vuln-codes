static void iommu_enable_dev_iotlb(struct device_domain_info *info)
{
	struct pci_dev *pdev;

	assert_spin_locked(&device_domain_lock);

	if (!info || !dev_is_pci(info->dev))
		return;

	pdev = to_pci_dev(info->dev);
	/* For IOMMU that supports device IOTLB throttling (DIT), we assign
	 * PFSID to the invalidation desc of a VF such that IOMMU HW can gauge
	 * queue depth at PF level. If DIT is not set, PFSID will be treated as
	 * reserved, which should be set to 0.
	 */
	if (!ecap_dit(info->iommu->ecap))
		info->pfsid = 0;
	else {
		struct pci_dev *pf_pdev;

		/* pdev will be returned if device is not a vf */
		pf_pdev = pci_physfn(pdev);
		info->pfsid = PCI_DEVID(pf_pdev->bus->number, pf_pdev->devfn);
	}

#ifdef CONFIG_INTEL_IOMMU_SVM
	/* The PCIe spec, in its wisdom, declares that the behaviour of
	   the device if you enable PASID support after ATS support is
	   undefined. So always enable PASID support on devices which
	   have it, even if we can't yet know if we're ever going to
	   use it. */
	if (info->pasid_supported && !pci_enable_pasid(pdev, info->pasid_supported & ~1))
		info->pasid_enabled = 1;

	if (info->pri_supported && !pci_reset_pri(pdev) && !pci_enable_pri(pdev, 32))
		info->pri_enabled = 1;
#endif
	if (info->ats_supported && !pci_enable_ats(pdev, VTD_PAGE_SHIFT)) {
		info->ats_enabled = 1;
		domain_update_iotlb(info->domain);
		info->ats_qdep = pci_ats_queue_depth(pdev);
	}
}