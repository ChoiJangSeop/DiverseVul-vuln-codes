static bool set_reg_profile(RAnal *anal) {
	const char *p =
		"=PC	pc\n"
		"=SP	r14\n" // XXX
		"=BP	srp\n" // XXX
		"=A0	r0\n"
		"=A1	r1\n"
		"=A2	r2\n"
		"=A3	r3\n"
		"gpr	sp	.32	56	0\n" // r14
		"gpr	acr	.32	60	0\n" // r15
		"gpr	pc	.32	64	0\n" // r16 // out of context
		"gpr	srp	.32	68	0\n" // like rbp on x86 // out of context
		// GPR
		"gpr	r0	.32	0	0\n"
		"gpr	r1	.32	4	0\n"
		"gpr	r2	.32	8	0\n"
		"gpr	r3	.32	12	0\n"
		"gpr	r4	.32	16	0\n"
		"gpr	r5	.32	20	0\n"
		"gpr	r6	.32	24	0\n"
		"gpr	r7	.32	28	0\n"
		"gpr	r8	.32	32	0\n"
		"gpr	r9	.32	36	0\n"
		"gpr	r10	.32	40	0\n"
		"gpr	r11	.32	44	0\n"
		"gpr	r12	.32	48	0\n"
		"gpr	r13	.32	52	0\n"

		// STACK POINTER
		"gpr	r14	.32	56	0\n"
		"gpr	r15	.32	60	0\n"
		// ADD P REGISTERS
		;
	return r_reg_set_profile_string (anal->reg, p);
}