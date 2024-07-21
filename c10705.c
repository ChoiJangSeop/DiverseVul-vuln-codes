static int FNAME(walk_addr_generic)(struct guest_walker *walker,
				    struct kvm_vcpu *vcpu, struct kvm_mmu *mmu,
				    gpa_t addr, u32 access)
{
	int ret;
	pt_element_t pte;
	pt_element_t __user *ptep_user;
	gfn_t table_gfn;
	u64 pt_access, pte_access;
	unsigned index, accessed_dirty, pte_pkey;
	unsigned nested_access;
	gpa_t pte_gpa;
	bool have_ad;
	int offset;
	u64 walk_nx_mask = 0;
	const int write_fault = access & PFERR_WRITE_MASK;
	const int user_fault  = access & PFERR_USER_MASK;
	const int fetch_fault = access & PFERR_FETCH_MASK;
	u16 errcode = 0;
	gpa_t real_gpa;
	gfn_t gfn;

	trace_kvm_mmu_pagetable_walk(addr, access);
retry_walk:
	walker->level = mmu->root_level;
	pte           = mmu->get_guest_pgd(vcpu);
	have_ad       = PT_HAVE_ACCESSED_DIRTY(mmu);

#if PTTYPE == 64
	walk_nx_mask = 1ULL << PT64_NX_SHIFT;
	if (walker->level == PT32E_ROOT_LEVEL) {
		pte = mmu->get_pdptr(vcpu, (addr >> 30) & 3);
		trace_kvm_mmu_paging_element(pte, walker->level);
		if (!FNAME(is_present_gpte)(pte))
			goto error;
		--walker->level;
	}
#endif
	walker->max_level = walker->level;
	ASSERT(!(is_long_mode(vcpu) && !is_pae(vcpu)));

	/*
	 * FIXME: on Intel processors, loads of the PDPTE registers for PAE paging
	 * by the MOV to CR instruction are treated as reads and do not cause the
	 * processor to set the dirty flag in any EPT paging-structure entry.
	 */
	nested_access = (have_ad ? PFERR_WRITE_MASK : 0) | PFERR_USER_MASK;

	pte_access = ~0;
	++walker->level;

	do {
		unsigned long host_addr;

		pt_access = pte_access;
		--walker->level;

		index = PT_INDEX(addr, walker->level);
		table_gfn = gpte_to_gfn(pte);
		offset    = index * sizeof(pt_element_t);
		pte_gpa   = gfn_to_gpa(table_gfn) + offset;

		BUG_ON(walker->level < 1);
		walker->table_gfn[walker->level - 1] = table_gfn;
		walker->pte_gpa[walker->level - 1] = pte_gpa;

		real_gpa = mmu->translate_gpa(vcpu, gfn_to_gpa(table_gfn),
					      nested_access,
					      &walker->fault);

		/*
		 * FIXME: This can happen if emulation (for of an INS/OUTS
		 * instruction) triggers a nested page fault.  The exit
		 * qualification / exit info field will incorrectly have
		 * "guest page access" as the nested page fault's cause,
		 * instead of "guest page structure access".  To fix this,
		 * the x86_exception struct should be augmented with enough
		 * information to fix the exit_qualification or exit_info_1
		 * fields.
		 */
		if (unlikely(real_gpa == UNMAPPED_GVA))
			return 0;

		host_addr = kvm_vcpu_gfn_to_hva_prot(vcpu, gpa_to_gfn(real_gpa),
					    &walker->pte_writable[walker->level - 1]);
		if (unlikely(kvm_is_error_hva(host_addr)))
			goto error;

		ptep_user = (pt_element_t __user *)((void *)host_addr + offset);
		if (unlikely(__get_user(pte, ptep_user)))
			goto error;
		walker->ptep_user[walker->level - 1] = ptep_user;

		trace_kvm_mmu_paging_element(pte, walker->level);

		/*
		 * Inverting the NX it lets us AND it like other
		 * permission bits.
		 */
		pte_access = pt_access & (pte ^ walk_nx_mask);

		if (unlikely(!FNAME(is_present_gpte)(pte)))
			goto error;

		if (unlikely(FNAME(is_rsvd_bits_set)(mmu, pte, walker->level))) {
			errcode = PFERR_RSVD_MASK | PFERR_PRESENT_MASK;
			goto error;
		}

		walker->ptes[walker->level - 1] = pte;
	} while (!is_last_gpte(mmu, walker->level, pte));

	pte_pkey = FNAME(gpte_pkeys)(vcpu, pte);
	accessed_dirty = have_ad ? pte_access & PT_GUEST_ACCESSED_MASK : 0;

	/* Convert to ACC_*_MASK flags for struct guest_walker.  */
	walker->pt_access = FNAME(gpte_access)(pt_access ^ walk_nx_mask);
	walker->pte_access = FNAME(gpte_access)(pte_access ^ walk_nx_mask);
	errcode = permission_fault(vcpu, mmu, walker->pte_access, pte_pkey, access);
	if (unlikely(errcode))
		goto error;

	gfn = gpte_to_gfn_lvl(pte, walker->level);
	gfn += (addr & PT_LVL_OFFSET_MASK(walker->level)) >> PAGE_SHIFT;

	if (PTTYPE == 32 && walker->level > PG_LEVEL_4K && is_cpuid_PSE36())
		gfn += pse36_gfn_delta(pte);

	real_gpa = mmu->translate_gpa(vcpu, gfn_to_gpa(gfn), access, &walker->fault);
	if (real_gpa == UNMAPPED_GVA)
		return 0;

	walker->gfn = real_gpa >> PAGE_SHIFT;

	if (!write_fault)
		FNAME(protect_clean_gpte)(mmu, &walker->pte_access, pte);
	else
		/*
		 * On a write fault, fold the dirty bit into accessed_dirty.
		 * For modes without A/D bits support accessed_dirty will be
		 * always clear.
		 */
		accessed_dirty &= pte >>
			(PT_GUEST_DIRTY_SHIFT - PT_GUEST_ACCESSED_SHIFT);

	if (unlikely(!accessed_dirty)) {
		ret = FNAME(update_accessed_dirty_bits)(vcpu, mmu, walker,
							addr, write_fault);
		if (unlikely(ret < 0))
			goto error;
		else if (ret)
			goto retry_walk;
	}

	pgprintk("%s: pte %llx pte_access %x pt_access %x\n",
		 __func__, (u64)pte, walker->pte_access, walker->pt_access);
	return 1;

error:
	errcode |= write_fault | user_fault;
	if (fetch_fault && (mmu->nx ||
			    kvm_read_cr4_bits(vcpu, X86_CR4_SMEP)))
		errcode |= PFERR_FETCH_MASK;

	walker->fault.vector = PF_VECTOR;
	walker->fault.error_code_valid = true;
	walker->fault.error_code = errcode;

#if PTTYPE == PTTYPE_EPT
	/*
	 * Use PFERR_RSVD_MASK in error_code to to tell if EPT
	 * misconfiguration requires to be injected. The detection is
	 * done by is_rsvd_bits_set() above.
	 *
	 * We set up the value of exit_qualification to inject:
	 * [2:0] - Derive from the access bits. The exit_qualification might be
	 *         out of date if it is serving an EPT misconfiguration.
	 * [5:3] - Calculated by the page walk of the guest EPT page tables
	 * [7:8] - Derived from [7:8] of real exit_qualification
	 *
	 * The other bits are set to 0.
	 */
	if (!(errcode & PFERR_RSVD_MASK)) {
		vcpu->arch.exit_qualification &= 0x180;
		if (write_fault)
			vcpu->arch.exit_qualification |= EPT_VIOLATION_ACC_WRITE;
		if (user_fault)
			vcpu->arch.exit_qualification |= EPT_VIOLATION_ACC_READ;
		if (fetch_fault)
			vcpu->arch.exit_qualification |= EPT_VIOLATION_ACC_INSTR;
		vcpu->arch.exit_qualification |= (pte_access & 0x7) << 3;
	}
#endif
	walker->fault.address = addr;
	walker->fault.nested_page_fault = mmu != vcpu->arch.walk_mmu;
	walker->fault.async_page_fault = false;

	trace_kvm_mmu_walker_error(walker->fault.error_code);
	return 0;
}