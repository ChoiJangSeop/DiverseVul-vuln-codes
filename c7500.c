int ipmi_si_port_setup(struct si_sm_io *io)
{
	unsigned int addr = io->addr_data;
	int          idx;

	if (!addr)
		return -ENODEV;

	io->io_cleanup = port_cleanup;

	/*
	 * Figure out the actual inb/inw/inl/etc routine to use based
	 * upon the register size.
	 */
	switch (io->regsize) {
	case 1:
		io->inputb = port_inb;
		io->outputb = port_outb;
		break;
	case 2:
		io->inputb = port_inw;
		io->outputb = port_outw;
		break;
	case 4:
		io->inputb = port_inl;
		io->outputb = port_outl;
		break;
	default:
		dev_warn(io->dev, "Invalid register size: %d\n",
			 io->regsize);
		return -EINVAL;
	}

	/*
	 * Some BIOSes reserve disjoint I/O regions in their ACPI
	 * tables.  This causes problems when trying to register the
	 * entire I/O region.  Therefore we must register each I/O
	 * port separately.
	 */
	for (idx = 0; idx < io->io_size; idx++) {
		if (request_region(addr + idx * io->regspacing,
				   io->regsize, DEVICE_NAME) == NULL) {
			/* Undo allocations */
			while (idx--)
				release_region(addr + idx * io->regspacing,
					       io->regsize);
			return -EIO;
		}
	}
	return 0;
}