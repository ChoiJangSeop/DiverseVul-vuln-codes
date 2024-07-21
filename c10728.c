static bool nested_vmcb_checks(struct vcpu_svm *svm, struct vmcb *vmcb12)
{
	bool vmcb12_lma;

	if ((vmcb12->save.efer & EFER_SVME) == 0)
		return false;

	if (((vmcb12->save.cr0 & X86_CR0_CD) == 0) && (vmcb12->save.cr0 & X86_CR0_NW))
		return false;

	if (!kvm_dr6_valid(vmcb12->save.dr6) || !kvm_dr7_valid(vmcb12->save.dr7))
		return false;

	vmcb12_lma = (vmcb12->save.efer & EFER_LME) && (vmcb12->save.cr0 & X86_CR0_PG);

	if (!vmcb12_lma) {
		if (vmcb12->save.cr4 & X86_CR4_PAE) {
			if (vmcb12->save.cr3 & MSR_CR3_LEGACY_PAE_RESERVED_MASK)
				return false;
		} else {
			if (vmcb12->save.cr3 & MSR_CR3_LEGACY_RESERVED_MASK)
				return false;
		}
	} else {
		if (!(vmcb12->save.cr4 & X86_CR4_PAE) ||
		    !(vmcb12->save.cr0 & X86_CR0_PE) ||
		    (vmcb12->save.cr3 & MSR_CR3_LONG_MBZ_MASK))
			return false;
	}
	if (kvm_valid_cr4(&svm->vcpu, vmcb12->save.cr4))
		return false;

	return nested_vmcb_check_controls(&vmcb12->control);
}