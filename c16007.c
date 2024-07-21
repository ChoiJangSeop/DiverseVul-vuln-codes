pci_populate_msixcap(struct msixcap *msixcap, int msgnum, int barnum,
		     uint32_t msix_tab_size)
{

	assert(msix_tab_size % 4096 == 0);

	bzero(msixcap, sizeof(struct msixcap));
	msixcap->capid = PCIY_MSIX;

	/*
	 * Message Control Register, all fields set to
	 * zero except for the Table Size.
	 * Note: Table size N is encoded as N-1
	 */
	msixcap->msgctrl = msgnum - 1;

	/*
	 * MSI-X BAR setup:
	 * - MSI-X table start at offset 0
	 * - PBA table starts at a 4K aligned offset after the MSI-X table
	 */
	msixcap->table_info = barnum & PCIM_MSIX_BIR_MASK;
	msixcap->pba_info = msix_tab_size | (barnum & PCIM_MSIX_BIR_MASK);
}