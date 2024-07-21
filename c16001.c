pci_bus_write_dsdt(int bus)
{
	struct businfo *bi;
	struct slotinfo *si;
	struct pci_vdev *dev;
	int count, func, slot;

	/*
	 * If there are no devices on this 'bus' then just return.
	 */
	 bi = pci_businfo[bus];
	if (bi == NULL) {
		/*
		 * Bus 0 is special because it decodes the I/O ports used
		 * for PCI config space access even if there are no devices
		 * on it.
		 */
		if (bus != 0)
			return;
	}

	dsdt_line("  Device (PCI%01X)", bus);
	dsdt_line("  {");
	dsdt_line("    Name (_HID, EisaId (\"PNP0A03\"))");
	dsdt_line("    Name (_ADR, Zero)");

	dsdt_line("    Method (_BBN, 0, NotSerialized)");
	dsdt_line("    {");
	dsdt_line("        Return (0x%08X)", bus);
	dsdt_line("    }");
	dsdt_line("    Name (_CRS, ResourceTemplate ()");
	dsdt_line("    {");
	dsdt_line("      WordBusNumber (ResourceProducer, MinFixed, "
	    "MaxFixed, PosDecode,");
	dsdt_line("        0x0000,             // Granularity");
	dsdt_line("        0x%04X,             // Range Minimum", bus);
	dsdt_line("        0x%04X,             // Range Maximum", bus);
	dsdt_line("        0x0000,             // Translation Offset");
	dsdt_line("        0x0001,             // Length");
	dsdt_line("        ,, )");

	if (bus == 0) {
		dsdt_indent(3);
		dsdt_fixed_ioport(0xCF8, 8);
		dsdt_unindent(3);

		dsdt_line("      WordIO (ResourceProducer, MinFixed, MaxFixed, "
		    "PosDecode, EntireRange,");
		dsdt_line("        0x0000,             // Granularity");
		dsdt_line("        0x0000,             // Range Minimum");
		dsdt_line("        0x0CF7,             // Range Maximum");
		dsdt_line("        0x0000,             // Translation Offset");
		dsdt_line("        0x0CF8,             // Length");
		dsdt_line("        ,, , TypeStatic)");

		dsdt_line("      WordIO (ResourceProducer, MinFixed, MaxFixed, "
		    "PosDecode, EntireRange,");
		dsdt_line("        0x0000,             // Granularity");
		dsdt_line("        0x0D00,             // Range Minimum");
		dsdt_line("        0x%04X,             // Range Maximum",
		    PCI_EMUL_IOBASE - 1);
		dsdt_line("        0x0000,             // Translation Offset");
		dsdt_line("        0x%04X,             // Length",
		    PCI_EMUL_IOBASE - 0x0D00);
		dsdt_line("        ,, , TypeStatic)");

		if (bi == NULL) {
			dsdt_line("    })");
			goto done;
		}
	}
	assert(bi != NULL);

	/* i/o window */
	dsdt_line("      WordIO (ResourceProducer, MinFixed, MaxFixed, "
	    "PosDecode, EntireRange,");
	dsdt_line("        0x0000,             // Granularity");
	dsdt_line("        0x%04X,             // Range Minimum", bi->iobase);
	dsdt_line("        0x%04X,             // Range Maximum",
	    bi->iolimit - 1);
	dsdt_line("        0x0000,             // Translation Offset");
	dsdt_line("        0x%04X,             // Length",
	    bi->iolimit - bi->iobase);
	dsdt_line("        ,, , TypeStatic)");

	/* mmio window (32-bit) */
	dsdt_line("      DWordMemory (ResourceProducer, PosDecode, "
	    "MinFixed, MaxFixed, NonCacheable, ReadWrite,");
	dsdt_line("        0x00000000,         // Granularity");
	dsdt_line("        0x%08X,         // Range Minimum\n", bi->membase32);
	dsdt_line("        0x%08X,         // Range Maximum\n",
	    bi->memlimit32 - 1);
	dsdt_line("        0x00000000,         // Translation Offset");
	dsdt_line("        0x%08X,         // Length\n",
	    bi->memlimit32 - bi->membase32);
	dsdt_line("        ,, , AddressRangeMemory, TypeStatic)");

	/* mmio window (64-bit) */
	dsdt_line("      QWordMemory (ResourceProducer, PosDecode, "
	    "MinFixed, MaxFixed, NonCacheable, ReadWrite,");
	dsdt_line("        0x0000000000000000, // Granularity");
	dsdt_line("        0x%016lX, // Range Minimum\n", bi->membase64);
	dsdt_line("        0x%016lX, // Range Maximum\n",
	    bi->memlimit64 - 1);
	dsdt_line("        0x0000000000000000, // Translation Offset");
	dsdt_line("        0x%016lX, // Length\n",
	    bi->memlimit64 - bi->membase64);
	dsdt_line("        ,, , AddressRangeMemory, TypeStatic)");
	dsdt_line("    })");

	if (!is_rtvm) {
		count = pci_count_lintr(bus);
		if (count != 0) {
			dsdt_indent(2);
			dsdt_line("Name (PPRT, Package ()");
			dsdt_line("{");
			pci_walk_lintr(bus, pci_pirq_prt_entry, NULL);
			dsdt_line("})");
			dsdt_line("Name (APRT, Package ()");
			dsdt_line("{");
			pci_walk_lintr(bus, pci_apic_prt_entry, NULL);
			dsdt_line("})");
			dsdt_line("Method (_PRT, 0, NotSerialized)");
			dsdt_line("{");
			dsdt_line("  If (PICM)");
			dsdt_line("  {");
			dsdt_line("    Return (APRT)");
			dsdt_line("  }");
			dsdt_line("  Else");
			dsdt_line("  {");
			dsdt_line("    Return (PPRT)");
			dsdt_line("  }");
			dsdt_line("}");
			dsdt_unindent(2);
		}
	}

	dsdt_indent(2);
	for (slot = 0; slot < MAXSLOTS; slot++) {
		si = &bi->slotinfo[slot];
		for (func = 0; func < MAXFUNCS; func++) {
			dev = si->si_funcs[func].fi_devi;
			if (dev != NULL &&
			    dev->dev_ops->vdev_write_dsdt != NULL)
				dev->dev_ops->vdev_write_dsdt(dev);
		}
	}
	dsdt_unindent(2);
done:
	dsdt_line("  }");
}