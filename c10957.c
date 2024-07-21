static int kvm_xen_shared_info_init(struct kvm *kvm, gfn_t gfn)
{
	struct gfn_to_pfn_cache *gpc = &kvm->arch.xen.shinfo_cache;
	gpa_t gpa = gfn_to_gpa(gfn);
	int wc_ofs, sec_hi_ofs;
	int ret = 0;
	int idx = srcu_read_lock(&kvm->srcu);

	if (gfn == GPA_INVALID) {
		kvm_gfn_to_pfn_cache_destroy(kvm, gpc);
		goto out;
	}

	ret = kvm_gfn_to_pfn_cache_init(kvm, gpc, NULL, false, true, gpa,
					PAGE_SIZE, false);
	if (ret)
		goto out;

	/* Paranoia checks on the 32-bit struct layout */
	BUILD_BUG_ON(offsetof(struct compat_shared_info, wc) != 0x900);
	BUILD_BUG_ON(offsetof(struct compat_shared_info, arch.wc_sec_hi) != 0x924);
	BUILD_BUG_ON(offsetof(struct pvclock_vcpu_time_info, version) != 0);

	/* 32-bit location by default */
	wc_ofs = offsetof(struct compat_shared_info, wc);
	sec_hi_ofs = offsetof(struct compat_shared_info, arch.wc_sec_hi);

#ifdef CONFIG_X86_64
	/* Paranoia checks on the 64-bit struct layout */
	BUILD_BUG_ON(offsetof(struct shared_info, wc) != 0xc00);
	BUILD_BUG_ON(offsetof(struct shared_info, wc_sec_hi) != 0xc0c);

	if (kvm->arch.xen.long_mode) {
		wc_ofs = offsetof(struct shared_info, wc);
		sec_hi_ofs = offsetof(struct shared_info, wc_sec_hi);
	}
#endif

	kvm_write_wall_clock(kvm, gpa + wc_ofs, sec_hi_ofs - wc_ofs);
	kvm_make_all_cpus_request(kvm, KVM_REQ_MASTERCLOCK_UPDATE);

out:
	srcu_read_unlock(&kvm->srcu, idx);
	return ret;
}