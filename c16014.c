pci_emul_add_msixcap(struct pci_vdev *dev, int msgnum, int barnum)
{
	uint32_t tab_size;
	struct msixcap msixcap;

	assert(msgnum >= 1 && msgnum <= MAX_MSIX_TABLE_ENTRIES);
	assert(barnum >= 0 && barnum <= PCIR_MAX_BAR_0);

	tab_size = msgnum * MSIX_TABLE_ENTRY_SIZE;

	/* Align table size to nearest 4K */
	tab_size = roundup2(tab_size, 4096);

	dev->msix.table_bar = barnum;
	dev->msix.pba_bar   = barnum;
	dev->msix.table_offset = 0;
	dev->msix.table_count = msgnum;
	dev->msix.pba_offset = tab_size;
	dev->msix.pba_size = PBA_SIZE(msgnum);

	pci_msix_table_init(dev, msgnum);

	pci_populate_msixcap(&msixcap, msgnum, barnum, tab_size);

	/* allocate memory for MSI-X Table and PBA */
	pci_emul_alloc_bar(dev, barnum, PCIBAR_MEM32,
				tab_size + dev->msix.pba_size);

	return (pci_emul_add_capability(dev, (u_char *)&msixcap,
					sizeof(msixcap)));
}