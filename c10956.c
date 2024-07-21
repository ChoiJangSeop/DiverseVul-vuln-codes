void kvm_write_wall_clock(struct kvm *kvm, gpa_t wall_clock, int sec_hi_ofs)
{
	int version;
	int r;
	struct pvclock_wall_clock wc;
	u32 wc_sec_hi;
	u64 wall_nsec;

	if (!wall_clock)
		return;

	r = kvm_read_guest(kvm, wall_clock, &version, sizeof(version));
	if (r)
		return;

	if (version & 1)
		++version;  /* first time write, random junk */

	++version;

	if (kvm_write_guest(kvm, wall_clock, &version, sizeof(version)))
		return;

	/*
	 * The guest calculates current wall clock time by adding
	 * system time (updated by kvm_guest_time_update below) to the
	 * wall clock specified here.  We do the reverse here.
	 */
	wall_nsec = ktime_get_real_ns() - get_kvmclock_ns(kvm);

	wc.nsec = do_div(wall_nsec, 1000000000);
	wc.sec = (u32)wall_nsec; /* overflow in 2106 guest time */
	wc.version = version;

	kvm_write_guest(kvm, wall_clock, &wc, sizeof(wc));

	if (sec_hi_ofs) {
		wc_sec_hi = wall_nsec >> 32;
		kvm_write_guest(kvm, wall_clock + sec_hi_ofs,
				&wc_sec_hi, sizeof(wc_sec_hi));
	}

	version++;
	kvm_write_guest(kvm, wall_clock, &version, sizeof(version));
}