static cycle_t kvm_clock_read(void)
{
	struct pvclock_vcpu_time_info *src;
	cycle_t ret;

	src = &get_cpu_var(hv_clock);
	ret = pvclock_clocksource_read(src);
	put_cpu_var(hv_clock);
	return ret;
}