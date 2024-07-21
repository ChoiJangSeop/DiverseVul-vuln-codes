deinit_pci(struct vmctx *ctx)
{
	struct pci_vdev_ops *ops;
	struct businfo *bi;
	struct slotinfo *si;
	struct funcinfo *fi;
	int bus, slot, func;
	size_t lowmem;
	struct mem_range mr;

	/* Release PCI extended config space */
	bzero(&mr, sizeof(struct mem_range));
	mr.name = "PCI ECFG";
	mr.base = PCI_EMUL_ECFG_BASE;
	mr.size = PCI_EMUL_ECFG_SIZE;
	unregister_mem(&mr);

	/* Release PCI hole space */
	lowmem = vm_get_lowmem_size(ctx);
	bzero(&mr, sizeof(struct mem_range));
	mr.name = "PCI hole (32-bit)";
	mr.base = lowmem;
	mr.size = (4ULL * 1024 * 1024 * 1024) - lowmem;
	unregister_mem_fallback(&mr);

	/* ditto for the 64-bit PCI host aperture */
	bzero(&mr, sizeof(struct mem_range));
	mr.name = "PCI hole (64-bit)";
	mr.base = PCI_EMUL_MEMBASE64;
	mr.size = PCI_EMUL_MEMLIMIT64 - PCI_EMUL_MEMBASE64;
	unregister_mem_fallback(&mr);

	for (bus = 0; bus < MAXBUSES; bus++) {
		bi = pci_businfo[bus];
		if (bi == NULL)
			continue;

		for (slot = 0; slot < MAXSLOTS; slot++) {
			si = &bi->slotinfo[slot];
			for (func = 0; func < MAXFUNCS; func++) {
				fi = &si->si_funcs[func];
				if (fi->fi_name == NULL)
					continue;
				ops = pci_emul_finddev(fi->fi_name);
				assert(ops != NULL);

				pr_notice("pci deinit %s\n", fi->fi_name);
				pci_emul_deinit(ctx, ops, bus, slot,
				    func, fi);
			}
		}
	}
}