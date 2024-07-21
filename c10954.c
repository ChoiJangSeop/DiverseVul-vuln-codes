static void bpf_adj_branches(struct bpf_prog *prog, u32 pos, u32 delta)
{
	struct bpf_insn *insn = prog->insnsi;
	u32 i, insn_cnt = prog->len;
	bool pseudo_call;
	u8 code;
	int off;

	for (i = 0; i < insn_cnt; i++, insn++) {
		code = insn->code;
		if (BPF_CLASS(code) != BPF_JMP)
			continue;
		if (BPF_OP(code) == BPF_EXIT)
			continue;
		if (BPF_OP(code) == BPF_CALL) {
			if (insn->src_reg == BPF_PSEUDO_CALL)
				pseudo_call = true;
			else
				continue;
		} else {
			pseudo_call = false;
		}
		off = pseudo_call ? insn->imm : insn->off;

		/* Adjust offset of jmps if we cross boundaries. */
		if (i < pos && i + off + 1 > pos)
			off += delta;
		else if (i > pos + delta && i + off + 1 <= pos + delta)
			off -= delta;

		if (pseudo_call)
			insn->imm = off;
		else
			insn->off = off;
	}
}