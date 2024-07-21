static bool set_reg_profile(RAnal *anal) {
	const char *p = \
		"=PC	pc\n"
		"=SP	sp\n"
		// this is the "new" ABI, the old was reverse order
		"=A0	r12\n"
		"=A1	r13\n"
		"=A2	r14\n"
		"=A3	r15\n"
		"gpr	r0	.16 0   0\n"
		"gpr	r1	.16 2   0\n"
		"gpr	r2	.16 4   0\n"
		"gpr	r3	.16 6   0\n"
		"gpr	r4	.16 8   0\n"
		"gpr	r5	.16 10  0\n"
		"gpr	r6	.16 12  0\n"
		"gpr	r7	.16 14  0\n"
		"gpr	r8	.16 16  0\n"
		"gpr	r9	.16 18  0\n"
		"gpr	r10   .16 20  0\n"
		"gpr	r11   .16 22  0\n"
		"gpr	r12   .16 24  0\n"
		"gpr	r13   .16 26  0\n"
		"gpr	r14   .16 28  0\n"
		"gpr	r15   .16 30  0\n"

		"gpr	pc	.16 0 0\n" // same as r0
		"gpr	sp	.16 2 0\n" // same as r1
		"flg	sr	.16 4 0\n" // same as r2
		"flg	c	.1  4 0\n"
		"flg	z	.1  4.1 0\n"
		"flg	n	.1  4.2 0\n"
		// between is SCG1 SCG0 OSOFF CPUOFF GIE
		"flg	v	.1  4.8 0\n";

	return r_reg_set_profile_string (anal->reg, p);
}