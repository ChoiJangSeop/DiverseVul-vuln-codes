static void print_bpf_insn(struct bpf_insn *insn)
{
	u8 class = BPF_CLASS(insn->code);

	if (class == BPF_ALU || class == BPF_ALU64) {
		if (BPF_SRC(insn->code) == BPF_X)
			verbose("(%02x) %sr%d %s %sr%d\n",
				insn->code, class == BPF_ALU ? "(u32) " : "",
				insn->dst_reg,
				bpf_alu_string[BPF_OP(insn->code) >> 4],
				class == BPF_ALU ? "(u32) " : "",
				insn->src_reg);
		else
			verbose("(%02x) %sr%d %s %s%d\n",
				insn->code, class == BPF_ALU ? "(u32) " : "",
				insn->dst_reg,
				bpf_alu_string[BPF_OP(insn->code) >> 4],
				class == BPF_ALU ? "(u32) " : "",
				insn->imm);
	} else if (class == BPF_STX) {
		if (BPF_MODE(insn->code) == BPF_MEM)
			verbose("(%02x) *(%s *)(r%d %+d) = r%d\n",
				insn->code,
				bpf_ldst_string[BPF_SIZE(insn->code) >> 3],
				insn->dst_reg,
				insn->off, insn->src_reg);
		else if (BPF_MODE(insn->code) == BPF_XADD)
			verbose("(%02x) lock *(%s *)(r%d %+d) += r%d\n",
				insn->code,
				bpf_ldst_string[BPF_SIZE(insn->code) >> 3],
				insn->dst_reg, insn->off,
				insn->src_reg);
		else
			verbose("BUG_%02x\n", insn->code);
	} else if (class == BPF_ST) {
		if (BPF_MODE(insn->code) != BPF_MEM) {
			verbose("BUG_st_%02x\n", insn->code);
			return;
		}
		verbose("(%02x) *(%s *)(r%d %+d) = %d\n",
			insn->code,
			bpf_ldst_string[BPF_SIZE(insn->code) >> 3],
			insn->dst_reg,
			insn->off, insn->imm);
	} else if (class == BPF_LDX) {
		if (BPF_MODE(insn->code) != BPF_MEM) {
			verbose("BUG_ldx_%02x\n", insn->code);
			return;
		}
		verbose("(%02x) r%d = *(%s *)(r%d %+d)\n",
			insn->code, insn->dst_reg,
			bpf_ldst_string[BPF_SIZE(insn->code) >> 3],
			insn->src_reg, insn->off);
	} else if (class == BPF_LD) {
		if (BPF_MODE(insn->code) == BPF_ABS) {
			verbose("(%02x) r0 = *(%s *)skb[%d]\n",
				insn->code,
				bpf_ldst_string[BPF_SIZE(insn->code) >> 3],
				insn->imm);
		} else if (BPF_MODE(insn->code) == BPF_IND) {
			verbose("(%02x) r0 = *(%s *)skb[r%d + %d]\n",
				insn->code,
				bpf_ldst_string[BPF_SIZE(insn->code) >> 3],
				insn->src_reg, insn->imm);
		} else if (BPF_MODE(insn->code) == BPF_IMM) {
			verbose("(%02x) r%d = 0x%x\n",
				insn->code, insn->dst_reg, insn->imm);
		} else {
			verbose("BUG_ld_%02x\n", insn->code);
			return;
		}
	} else if (class == BPF_JMP) {
		u8 opcode = BPF_OP(insn->code);

		if (opcode == BPF_CALL) {
			verbose("(%02x) call %s#%d\n", insn->code,
				func_id_name(insn->imm), insn->imm);
		} else if (insn->code == (BPF_JMP | BPF_JA)) {
			verbose("(%02x) goto pc%+d\n",
				insn->code, insn->off);
		} else if (insn->code == (BPF_JMP | BPF_EXIT)) {
			verbose("(%02x) exit\n", insn->code);
		} else if (BPF_SRC(insn->code) == BPF_X) {
			verbose("(%02x) if r%d %s r%d goto pc%+d\n",
				insn->code, insn->dst_reg,
				bpf_jmp_string[BPF_OP(insn->code) >> 4],
				insn->src_reg, insn->off);
		} else {
			verbose("(%02x) if r%d %s 0x%x goto pc%+d\n",
				insn->code, insn->dst_reg,
				bpf_jmp_string[BPF_OP(insn->code) >> 4],
				insn->imm, insn->off);
		}
	} else {
		verbose("(%02x) %s\n", insn->code, bpf_class_string[class]);
	}
}