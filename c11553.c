void __init pt_regs_check(void)
{
	BUILD_BUG_ON(offsetof(struct pt_regs, gpr) !=
		     offsetof(struct user_pt_regs, gpr));
	BUILD_BUG_ON(offsetof(struct pt_regs, nip) !=
		     offsetof(struct user_pt_regs, nip));
	BUILD_BUG_ON(offsetof(struct pt_regs, msr) !=
		     offsetof(struct user_pt_regs, msr));
	BUILD_BUG_ON(offsetof(struct pt_regs, orig_gpr3) !=
		     offsetof(struct user_pt_regs, orig_gpr3));
	BUILD_BUG_ON(offsetof(struct pt_regs, ctr) !=
		     offsetof(struct user_pt_regs, ctr));
	BUILD_BUG_ON(offsetof(struct pt_regs, link) !=
		     offsetof(struct user_pt_regs, link));
	BUILD_BUG_ON(offsetof(struct pt_regs, xer) !=
		     offsetof(struct user_pt_regs, xer));
	BUILD_BUG_ON(offsetof(struct pt_regs, ccr) !=
		     offsetof(struct user_pt_regs, ccr));
#ifdef __powerpc64__
	BUILD_BUG_ON(offsetof(struct pt_regs, softe) !=
		     offsetof(struct user_pt_regs, softe));
#else
	BUILD_BUG_ON(offsetof(struct pt_regs, mq) !=
		     offsetof(struct user_pt_regs, mq));
#endif
	BUILD_BUG_ON(offsetof(struct pt_regs, trap) !=
		     offsetof(struct user_pt_regs, trap));
	BUILD_BUG_ON(offsetof(struct pt_regs, dar) !=
		     offsetof(struct user_pt_regs, dar));
	BUILD_BUG_ON(offsetof(struct pt_regs, dear) !=
		     offsetof(struct user_pt_regs, dar));
	BUILD_BUG_ON(offsetof(struct pt_regs, dsisr) !=
		     offsetof(struct user_pt_regs, dsisr));
	BUILD_BUG_ON(offsetof(struct pt_regs, esr) !=
		     offsetof(struct user_pt_regs, dsisr));
	BUILD_BUG_ON(offsetof(struct pt_regs, result) !=
		     offsetof(struct user_pt_regs, result));

	BUILD_BUG_ON(sizeof(struct user_pt_regs) > sizeof(struct pt_regs));

	// Now check that the pt_regs offsets match the uapi #defines
	#define CHECK_REG(_pt, _reg) \
		BUILD_BUG_ON(_pt != (offsetof(struct user_pt_regs, _reg) / \
				     sizeof(unsigned long)));

	CHECK_REG(PT_R0,  gpr[0]);
	CHECK_REG(PT_R1,  gpr[1]);
	CHECK_REG(PT_R2,  gpr[2]);
	CHECK_REG(PT_R3,  gpr[3]);
	CHECK_REG(PT_R4,  gpr[4]);
	CHECK_REG(PT_R5,  gpr[5]);
	CHECK_REG(PT_R6,  gpr[6]);
	CHECK_REG(PT_R7,  gpr[7]);
	CHECK_REG(PT_R8,  gpr[8]);
	CHECK_REG(PT_R9,  gpr[9]);
	CHECK_REG(PT_R10, gpr[10]);
	CHECK_REG(PT_R11, gpr[11]);
	CHECK_REG(PT_R12, gpr[12]);
	CHECK_REG(PT_R13, gpr[13]);
	CHECK_REG(PT_R14, gpr[14]);
	CHECK_REG(PT_R15, gpr[15]);
	CHECK_REG(PT_R16, gpr[16]);
	CHECK_REG(PT_R17, gpr[17]);
	CHECK_REG(PT_R18, gpr[18]);
	CHECK_REG(PT_R19, gpr[19]);
	CHECK_REG(PT_R20, gpr[20]);
	CHECK_REG(PT_R21, gpr[21]);
	CHECK_REG(PT_R22, gpr[22]);
	CHECK_REG(PT_R23, gpr[23]);
	CHECK_REG(PT_R24, gpr[24]);
	CHECK_REG(PT_R25, gpr[25]);
	CHECK_REG(PT_R26, gpr[26]);
	CHECK_REG(PT_R27, gpr[27]);
	CHECK_REG(PT_R28, gpr[28]);
	CHECK_REG(PT_R29, gpr[29]);
	CHECK_REG(PT_R30, gpr[30]);
	CHECK_REG(PT_R31, gpr[31]);
	CHECK_REG(PT_NIP, nip);
	CHECK_REG(PT_MSR, msr);
	CHECK_REG(PT_ORIG_R3, orig_gpr3);
	CHECK_REG(PT_CTR, ctr);
	CHECK_REG(PT_LNK, link);
	CHECK_REG(PT_XER, xer);
	CHECK_REG(PT_CCR, ccr);
#ifdef CONFIG_PPC64
	CHECK_REG(PT_SOFTE, softe);
#else
	CHECK_REG(PT_MQ, mq);
#endif
	CHECK_REG(PT_TRAP, trap);
	CHECK_REG(PT_DAR, dar);
	CHECK_REG(PT_DSISR, dsisr);
	CHECK_REG(PT_RESULT, result);
	#undef CHECK_REG

	BUILD_BUG_ON(PT_REGS_COUNT != sizeof(struct user_pt_regs) / sizeof(unsigned long));

	/*
	 * PT_DSCR isn't a real reg, but it's important that it doesn't overlap the
	 * real registers.
	 */
	BUILD_BUG_ON(PT_DSCR < sizeof(struct user_pt_regs) / sizeof(unsigned long));
}