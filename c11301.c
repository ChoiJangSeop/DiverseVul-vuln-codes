static int FNAME(cmpxchg_gpte)(struct kvm_vcpu *vcpu, struct kvm_mmu *mmu,
			       pt_element_t __user *ptep_user, unsigned index,
			       pt_element_t orig_pte, pt_element_t new_pte)
{
	signed char r;

	if (!user_access_begin(ptep_user, sizeof(pt_element_t)))
		return -EFAULT;

#ifdef CMPXCHG
	asm volatile("1:" LOCK_PREFIX CMPXCHG " %[new], %[ptr]\n"
		     "setnz %b[r]\n"
		     "2:"
		     _ASM_EXTABLE_TYPE_REG(1b, 2b, EX_TYPE_EFAULT_REG, %k[r])
		     : [ptr] "+m" (*ptep_user),
		       [old] "+a" (orig_pte),
		       [r] "=q" (r)
		     : [new] "r" (new_pte)
		     : "memory");
#else
	asm volatile("1:" LOCK_PREFIX "cmpxchg8b %[ptr]\n"
		     "setnz %b[r]\n"
		     "2:"
		     _ASM_EXTABLE_TYPE_REG(1b, 2b, EX_TYPE_EFAULT_REG, %k[r])
		     : [ptr] "+m" (*ptep_user),
		       [old] "+A" (orig_pte),
		       [r] "=q" (r)
		     : [new_lo] "b" ((u32)new_pte),
		       [new_hi] "c" ((u32)(new_pte >> 32))
		     : "memory");
#endif

	user_access_end();
	return r;
}