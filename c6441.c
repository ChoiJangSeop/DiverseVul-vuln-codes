static int hardware_enable(void *garbage)
{
	int cpu = raw_smp_processor_id();
	u64 phys_addr = __pa(per_cpu(vmxarea, cpu));
	u64 old, test_bits;

	if (read_cr4() & X86_CR4_VMXE)
		return -EBUSY;

	INIT_LIST_HEAD(&per_cpu(vcpus_on_cpu, cpu));
	rdmsrl(MSR_IA32_FEATURE_CONTROL, old);

	test_bits = FEATURE_CONTROL_LOCKED;
	test_bits |= FEATURE_CONTROL_VMXON_ENABLED_OUTSIDE_SMX;
	if (tboot_enabled())
		test_bits |= FEATURE_CONTROL_VMXON_ENABLED_INSIDE_SMX;

	if ((old & test_bits) != test_bits) {
		/* enable and lock */
		wrmsrl(MSR_IA32_FEATURE_CONTROL, old | test_bits);
	}
	write_cr4(read_cr4() | X86_CR4_VMXE); /* FIXME: not cpu hotplug safe */

	if (vmm_exclusive) {
		kvm_cpu_vmxon(phys_addr);
		ept_sync_global();
	}

	return 0;
}