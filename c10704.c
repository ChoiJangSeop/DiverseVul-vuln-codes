static int FNAME(fetch)(struct kvm_vcpu *vcpu, gpa_t addr,
			 struct guest_walker *gw, u32 error_code,
			 int max_level, kvm_pfn_t pfn, bool map_writable,
			 bool prefault)
{
	bool nx_huge_page_workaround_enabled = is_nx_huge_page_enabled();
	bool write_fault = error_code & PFERR_WRITE_MASK;
	bool exec = error_code & PFERR_FETCH_MASK;
	bool huge_page_disallowed = exec && nx_huge_page_workaround_enabled;
	struct kvm_mmu_page *sp = NULL;
	struct kvm_shadow_walk_iterator it;
	unsigned direct_access, access = gw->pt_access;
	int top_level, level, req_level, ret;
	gfn_t base_gfn = gw->gfn;

	direct_access = gw->pte_access;

	top_level = vcpu->arch.mmu->root_level;
	if (top_level == PT32E_ROOT_LEVEL)
		top_level = PT32_ROOT_LEVEL;
	/*
	 * Verify that the top-level gpte is still there.  Since the page
	 * is a root page, it is either write protected (and cannot be
	 * changed from now on) or it is invalid (in which case, we don't
	 * really care if it changes underneath us after this point).
	 */
	if (FNAME(gpte_changed)(vcpu, gw, top_level))
		goto out_gpte_changed;

	if (WARN_ON(!VALID_PAGE(vcpu->arch.mmu->root_hpa)))
		goto out_gpte_changed;

	for (shadow_walk_init(&it, vcpu, addr);
	     shadow_walk_okay(&it) && it.level > gw->level;
	     shadow_walk_next(&it)) {
		gfn_t table_gfn;

		clear_sp_write_flooding_count(it.sptep);
		drop_large_spte(vcpu, it.sptep);

		sp = NULL;
		if (!is_shadow_present_pte(*it.sptep)) {
			table_gfn = gw->table_gfn[it.level - 2];
			sp = kvm_mmu_get_page(vcpu, table_gfn, addr, it.level-1,
					      false, access);
		}

		/*
		 * Verify that the gpte in the page we've just write
		 * protected is still there.
		 */
		if (FNAME(gpte_changed)(vcpu, gw, it.level - 1))
			goto out_gpte_changed;

		if (sp)
			link_shadow_page(vcpu, it.sptep, sp);
	}

	level = kvm_mmu_hugepage_adjust(vcpu, gw->gfn, max_level, &pfn,
					huge_page_disallowed, &req_level);

	trace_kvm_mmu_spte_requested(addr, gw->level, pfn);

	for (; shadow_walk_okay(&it); shadow_walk_next(&it)) {
		clear_sp_write_flooding_count(it.sptep);

		/*
		 * We cannot overwrite existing page tables with an NX
		 * large page, as the leaf could be executable.
		 */
		if (nx_huge_page_workaround_enabled)
			disallowed_hugepage_adjust(*it.sptep, gw->gfn, it.level,
						   &pfn, &level);

		base_gfn = gw->gfn & ~(KVM_PAGES_PER_HPAGE(it.level) - 1);
		if (it.level == level)
			break;

		validate_direct_spte(vcpu, it.sptep, direct_access);

		drop_large_spte(vcpu, it.sptep);

		if (!is_shadow_present_pte(*it.sptep)) {
			sp = kvm_mmu_get_page(vcpu, base_gfn, addr,
					      it.level - 1, true, direct_access);
			link_shadow_page(vcpu, it.sptep, sp);
			if (huge_page_disallowed && req_level >= it.level)
				account_huge_nx_page(vcpu->kvm, sp);
		}
	}

	ret = mmu_set_spte(vcpu, it.sptep, gw->pte_access, write_fault,
			   it.level, base_gfn, pfn, prefault, map_writable);
	if (ret == RET_PF_SPURIOUS)
		return ret;

	FNAME(pte_prefetch)(vcpu, gw, it.sptep);
	++vcpu->stat.pf_fixed;
	return ret;

out_gpte_changed:
	return RET_PF_RETRY;
}