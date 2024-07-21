modify_bar_registration(struct pci_vdev *dev, int idx, int registration)
{
	int error;
	struct inout_port iop;
	struct mem_range mr;

	if (is_pci_gvt(dev)) {
		/* GVT device is the only one who traps the pci bar access and
		 * intercepts the corresponding contents in kernel. It needs
		 * register pci resource only, but no need to register the
		 * region.
		 *
		 * FIXME: This is a short term solution. This patch will be
		 * obsoleted with the migration of using OVMF to do bar
		 * addressing and generate ACPI PCI resource from using
		 * acrn-dm.
		 */
		printf("modify_bar_registration: bypass for pci-gvt\n");
		return;
	}
	switch (dev->bar[idx].type) {
	case PCIBAR_IO:
		bzero(&iop, sizeof(struct inout_port));
		iop.name = dev->name;
		iop.port = dev->bar[idx].addr;
		iop.size = dev->bar[idx].size;
		if (registration) {
			iop.flags = IOPORT_F_INOUT;
			iop.handler = pci_emul_io_handler;
			iop.arg = dev;
			error = register_inout(&iop);
		} else
			error = unregister_inout(&iop);
		break;
	case PCIBAR_MEM32:
	case PCIBAR_MEM64:
		bzero(&mr, sizeof(struct mem_range));
		mr.name = dev->name;
		mr.base = dev->bar[idx].addr;
		mr.size = dev->bar[idx].size;
		if (registration) {
			mr.flags = MEM_F_RW;
			mr.handler = pci_emul_mem_handler;
			mr.arg1 = dev;
			mr.arg2 = idx;
			error = register_mem(&mr);
		} else
			error = unregister_mem(&mr);
		break;
	default:
		error = EINVAL;
		break;
	}
	assert(error == 0);
}