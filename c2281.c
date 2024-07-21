void __init kvmclock_init(void)
{
	unsigned long mem;
	int size;

	size = PAGE_ALIGN(sizeof(struct pvclock_vsyscall_time_info)*NR_CPUS);

	if (!kvm_para_available())
		return;

	if (kvmclock && kvm_para_has_feature(KVM_FEATURE_CLOCKSOURCE2)) {
		msr_kvm_system_time = MSR_KVM_SYSTEM_TIME_NEW;
		msr_kvm_wall_clock = MSR_KVM_WALL_CLOCK_NEW;
	} else if (!(kvmclock && kvm_para_has_feature(KVM_FEATURE_CLOCKSOURCE)))
		return;

	printk(KERN_INFO "kvm-clock: Using msrs %x and %x",
		msr_kvm_system_time, msr_kvm_wall_clock);

	mem = memblock_alloc(size, PAGE_SIZE);
	if (!mem)
		return;
	hv_clock = __va(mem);
	memset(hv_clock, 0, size);

	if (kvm_register_clock("primary cpu clock")) {
		hv_clock = NULL;
		memblock_free(mem, size);
		return;
	}
	pv_time_ops.sched_clock = kvm_clock_read;
	x86_platform.calibrate_tsc = kvm_get_tsc_khz;
	x86_platform.get_wallclock = kvm_get_wallclock;
	x86_platform.set_wallclock = kvm_set_wallclock;
#ifdef CONFIG_X86_LOCAL_APIC
	x86_cpuinit.early_percpu_clock_init =
		kvm_setup_secondary_clock;
#endif
	x86_platform.save_sched_clock_state = kvm_save_sched_clock_state;
	x86_platform.restore_sched_clock_state = kvm_restore_sched_clock_state;
	machine_ops.shutdown  = kvm_shutdown;
#ifdef CONFIG_KEXEC
	machine_ops.crash_shutdown  = kvm_crash_shutdown;
#endif
	kvm_get_preset_lpj();
	clocksource_register_hz(&kvm_clock, NSEC_PER_SEC);
	pv_info.paravirt_enabled = 1;
	pv_info.name = "KVM";

	if (kvm_para_has_feature(KVM_FEATURE_CLOCKSOURCE_STABLE_BIT))
		pvclock_set_flags(PVCLOCK_TSC_STABLE_BIT);
}