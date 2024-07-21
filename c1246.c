static int FNAME(walk_addr_generic)(struct guest_walker *walker,
				    struct kvm_vcpu *vcpu, struct kvm_mmu *mmu,
				    gva_t addr, u32 access)
{
	pt_element_t pte;
	pt_element_t __user *ptep_user;
	gfn_t table_gfn;
	unsigned index, pt_access, uninitialized_var(pte_access);
	gpa_t pte_gpa;
	bool eperm, present, rsvd_fault;
	int offset, write_fault, user_fault, fetch_fault;

	write_fault = access & PFERR_WRITE_MASK;
	user_fault = access & PFERR_USER_MASK;
	fetch_fault = access & PFERR_FETCH_MASK;

	trace_kvm_mmu_pagetable_walk(addr, write_fault, user_fault,
				     fetch_fault);
walk:
	present = true;
	eperm = rsvd_fault = false;
	walker->level = mmu->root_level;
	pte           = mmu->get_cr3(vcpu);

#if PTTYPE == 64
	if (walker->level == PT32E_ROOT_LEVEL) {
		pte = kvm_pdptr_read_mmu(vcpu, mmu, (addr >> 30) & 3);
		trace_kvm_mmu_paging_element(pte, walker->level);
		if (!is_present_gpte(pte)) {
			present = false;
			goto error;
		}
		--walker->level;
	}
#endif
	ASSERT((!is_long_mode(vcpu) && is_pae(vcpu)) ||
	       (mmu->get_cr3(vcpu) & CR3_NONPAE_RESERVED_BITS) == 0);

	pt_access = ACC_ALL;

	for (;;) {
		gfn_t real_gfn;
		unsigned long host_addr;

		index = PT_INDEX(addr, walker->level);

		table_gfn = gpte_to_gfn(pte);
		offset    = index * sizeof(pt_element_t);
		pte_gpa   = gfn_to_gpa(table_gfn) + offset;
		walker->table_gfn[walker->level - 1] = table_gfn;
		walker->pte_gpa[walker->level - 1] = pte_gpa;

		real_gfn = mmu->translate_gpa(vcpu, gfn_to_gpa(table_gfn),
					      PFERR_USER_MASK|PFERR_WRITE_MASK);
		if (unlikely(real_gfn == UNMAPPED_GVA)) {
			present = false;
			break;
		}
		real_gfn = gpa_to_gfn(real_gfn);

		host_addr = gfn_to_hva(vcpu->kvm, real_gfn);
		if (unlikely(kvm_is_error_hva(host_addr))) {
			present = false;
			break;
		}

		ptep_user = (pt_element_t __user *)((void *)host_addr + offset);
		if (unlikely(copy_from_user(&pte, ptep_user, sizeof(pte)))) {
			present = false;
			break;
		}

		trace_kvm_mmu_paging_element(pte, walker->level);

		if (unlikely(!is_present_gpte(pte))) {
			present = false;
			break;
		}

		if (unlikely(is_rsvd_bits_set(&vcpu->arch.mmu, pte,
					      walker->level))) {
			rsvd_fault = true;
			break;
		}

		if (unlikely(write_fault && !is_writable_pte(pte)
			     && (user_fault || is_write_protection(vcpu))))
			eperm = true;

		if (unlikely(user_fault && !(pte & PT_USER_MASK)))
			eperm = true;

#if PTTYPE == 64
		if (unlikely(fetch_fault && (pte & PT64_NX_MASK)))
			eperm = true;
#endif

		if (!eperm && !rsvd_fault
		    && unlikely(!(pte & PT_ACCESSED_MASK))) {
			int ret;
			trace_kvm_mmu_set_accessed_bit(table_gfn, index,
						       sizeof(pte));
			ret = FNAME(cmpxchg_gpte)(vcpu, mmu, table_gfn,
					index, pte, pte|PT_ACCESSED_MASK);
			if (ret < 0) {
				present = false;
				break;
			} else if (ret)
				goto walk;

			mark_page_dirty(vcpu->kvm, table_gfn);
			pte |= PT_ACCESSED_MASK;
		}

		pte_access = pt_access & FNAME(gpte_access)(vcpu, pte);

		walker->ptes[walker->level - 1] = pte;

		if ((walker->level == PT_PAGE_TABLE_LEVEL) ||
		    ((walker->level == PT_DIRECTORY_LEVEL) &&
				is_large_pte(pte) &&
				(PTTYPE == 64 || is_pse(vcpu))) ||
		    ((walker->level == PT_PDPE_LEVEL) &&
				is_large_pte(pte) &&
				mmu->root_level == PT64_ROOT_LEVEL)) {
			int lvl = walker->level;
			gpa_t real_gpa;
			gfn_t gfn;
			u32 ac;

			gfn = gpte_to_gfn_lvl(pte, lvl);
			gfn += (addr & PT_LVL_OFFSET_MASK(lvl)) >> PAGE_SHIFT;

			if (PTTYPE == 32 &&
			    walker->level == PT_DIRECTORY_LEVEL &&
			    is_cpuid_PSE36())
				gfn += pse36_gfn_delta(pte);

			ac = write_fault | fetch_fault | user_fault;

			real_gpa = mmu->translate_gpa(vcpu, gfn_to_gpa(gfn),
						      ac);
			if (real_gpa == UNMAPPED_GVA)
				return 0;

			walker->gfn = real_gpa >> PAGE_SHIFT;

			break;
		}

		pt_access = pte_access;
		--walker->level;
	}

	if (unlikely(!present || eperm || rsvd_fault))
		goto error;

	if (write_fault && unlikely(!is_dirty_gpte(pte))) {
		int ret;

		trace_kvm_mmu_set_dirty_bit(table_gfn, index, sizeof(pte));
		ret = FNAME(cmpxchg_gpte)(vcpu, mmu, table_gfn, index, pte,
			    pte|PT_DIRTY_MASK);
		if (ret < 0) {
			present = false;
			goto error;
		} else if (ret)
			goto walk;

		mark_page_dirty(vcpu->kvm, table_gfn);
		pte |= PT_DIRTY_MASK;
		walker->ptes[walker->level - 1] = pte;
	}

	walker->pt_access = pt_access;
	walker->pte_access = pte_access;
	pgprintk("%s: pte %llx pte_access %x pt_access %x\n",
		 __func__, (u64)pte, pte_access, pt_access);
	return 1;

error:
	walker->fault.vector = PF_VECTOR;
	walker->fault.error_code_valid = true;
	walker->fault.error_code = 0;
	if (present)
		walker->fault.error_code |= PFERR_PRESENT_MASK;

	walker->fault.error_code |= write_fault | user_fault;

	if (fetch_fault && mmu->nx)
		walker->fault.error_code |= PFERR_FETCH_MASK;
	if (rsvd_fault)
		walker->fault.error_code |= PFERR_RSVD_MASK;

	walker->fault.address = addr;
	walker->fault.nested_page_fault = mmu != vcpu->arch.walk_mmu;

	trace_kvm_mmu_walker_error(walker->fault.error_code);
	return 0;
}