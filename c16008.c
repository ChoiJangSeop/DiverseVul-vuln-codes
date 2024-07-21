pci_msix_table_init(struct pci_vdev *dev, int table_entries)
{
	int i, table_size;

	assert(table_entries > 0);
	assert(table_entries <= MAX_MSIX_TABLE_ENTRIES);

	table_size = table_entries * MSIX_TABLE_ENTRY_SIZE;
	dev->msix.table = calloc(1, table_size);

	assert(dev->msix.table != NULL);

	/* set mask bit of vector control register */
	for (i = 0; i < table_entries; i++)
		dev->msix.table[i].vector_control |= PCIM_MSIX_VCTRL_MASK;
}