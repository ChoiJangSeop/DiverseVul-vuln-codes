static u32 __peek_user_compat(struct task_struct *child, addr_t addr)
{
	struct user32 *dummy32 = NULL;
	per_struct32 *dummy_per32 = NULL;
	addr_t offset;
	__u32 tmp;

	if (addr < (addr_t) &dummy32->regs.acrs) {
		/*
		 * psw and gprs are stored on the stack
		 */
		if (addr == (addr_t) &dummy32->regs.psw.mask) {
			/* Fake a 31 bit psw mask. */
			tmp = (__u32)(task_pt_regs(child)->psw.mask >> 32);
			tmp = PSW32_MASK_MERGE(psw32_user_bits, tmp);
		} else if (addr == (addr_t) &dummy32->regs.psw.addr) {
			/* Fake a 31 bit psw address. */
			tmp = (__u32) task_pt_regs(child)->psw.addr |
				PSW32_ADDR_AMODE31;
		} else {
			/* gpr 0-15 */
			tmp = *(__u32 *)((addr_t) &task_pt_regs(child)->psw +
					 addr*2 + 4);
		}
	} else if (addr < (addr_t) (&dummy32->regs.orig_gpr2)) {
		/*
		 * access registers are stored in the thread structure
		 */
		offset = addr - (addr_t) &dummy32->regs.acrs;
		tmp = *(__u32*)((addr_t) &child->thread.acrs + offset);

	} else if (addr == (addr_t) (&dummy32->regs.orig_gpr2)) {
		/*
		 * orig_gpr2 is stored on the kernel stack
		 */
		tmp = *(__u32*)((addr_t) &task_pt_regs(child)->orig_gpr2 + 4);

	} else if (addr < (addr_t) (&dummy32->regs.fp_regs + 1)) {
		/*
		 * floating point regs. are stored in the thread structure 
		 */
	        offset = addr - (addr_t) &dummy32->regs.fp_regs;
		tmp = *(__u32 *)((addr_t) &child->thread.fp_regs + offset);

	} else if (addr < (addr_t) (&dummy32->regs.per_info + 1)) {
		/*
		 * per_info is found in the thread structure
		 */
		offset = addr - (addr_t) &dummy32->regs.per_info;
		/* This is magic. See per_struct and per_struct32. */
		if ((offset >= (addr_t) &dummy_per32->control_regs &&
		     offset < (addr_t) (&dummy_per32->control_regs + 1)) ||
		    (offset >= (addr_t) &dummy_per32->starting_addr &&
		     offset <= (addr_t) &dummy_per32->ending_addr) ||
		    offset == (addr_t) &dummy_per32->lowcore.words.address)
			offset = offset*2 + 4;
		else
			offset = offset*2;
		tmp = *(__u32 *)((addr_t) &child->thread.per_info + offset);

	} else
		tmp = 0;

	return tmp;
}