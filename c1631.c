static void early_init_amd(struct cpuinfo_x86 *c)
{
	early_init_amd_mc(c);

	/*
	 * c->x86_power is 8000_0007 edx. Bit 8 is TSC runs at constant rate
	 * with P/T states and does not stop in deep C-states
	 */
	if (c->x86_power & (1 << 8)) {
		set_cpu_cap(c, X86_FEATURE_CONSTANT_TSC);
		set_cpu_cap(c, X86_FEATURE_NONSTOP_TSC);
		if (!check_tsc_unstable())
			sched_clock_stable = 1;
	}

#ifdef CONFIG_X86_64
	set_cpu_cap(c, X86_FEATURE_SYSCALL32);
#else
	/*  Set MTRR capability flag if appropriate */
	if (c->x86 == 5)
		if (c->x86_model == 13 || c->x86_model == 9 ||
		    (c->x86_model == 8 && c->x86_mask >= 8))
			set_cpu_cap(c, X86_FEATURE_K6_MTRR);
#endif
#if defined(CONFIG_X86_LOCAL_APIC) && defined(CONFIG_PCI)
	/* check CPU config space for extended APIC ID */
	if (cpu_has_apic && c->x86 >= 0xf) {
		unsigned int val;
		val = read_pci_config(0, 24, 0, 0x68);
		if ((val & ((1 << 17) | (1 << 18))) == ((1 << 17) | (1 << 18)))
			set_cpu_cap(c, X86_FEATURE_EXTD_APICID);
	}
#endif
}