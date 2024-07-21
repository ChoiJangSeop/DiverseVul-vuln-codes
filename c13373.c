static int adjust_scalar_min_max_vals(struct bpf_verifier_env *env,
				      struct bpf_insn *insn,
				      struct bpf_reg_state *dst_reg,
				      struct bpf_reg_state src_reg)
{
	struct bpf_reg_state *regs = cur_regs(env);
	u8 opcode = BPF_OP(insn->code);
	bool src_known, dst_known;
	s64 smin_val, smax_val;
	u64 umin_val, umax_val;
	u64 insn_bitness = (BPF_CLASS(insn->code) == BPF_ALU64) ? 64 : 32;
	u32 dst = insn->dst_reg;
	int ret;

	if (insn_bitness == 32) {
		/* Relevant for 32-bit RSH: Information can propagate towards
		 * LSB, so it isn't sufficient to only truncate the output to
		 * 32 bits.
		 */
		coerce_reg_to_size(dst_reg, 4);
		coerce_reg_to_size(&src_reg, 4);
	}

	smin_val = src_reg.smin_value;
	smax_val = src_reg.smax_value;
	umin_val = src_reg.umin_value;
	umax_val = src_reg.umax_value;
	src_known = tnum_is_const(src_reg.var_off);
	dst_known = tnum_is_const(dst_reg->var_off);

	if ((src_known && (smin_val != smax_val || umin_val != umax_val)) ||
	    smin_val > smax_val || umin_val > umax_val) {
		/* Taint dst register if offset had invalid bounds derived from
		 * e.g. dead branches.
		 */
		__mark_reg_unknown(env, dst_reg);
		return 0;
	}

	if (!src_known &&
	    opcode != BPF_ADD && opcode != BPF_SUB && opcode != BPF_AND) {
		__mark_reg_unknown(env, dst_reg);
		return 0;
	}

	switch (opcode) {
	case BPF_ADD:
		ret = sanitize_val_alu(env, insn);
		if (ret < 0) {
			verbose(env, "R%d tried to add from different pointers or scalars\n", dst);
			return ret;
		}
		scalar_min_max_add(dst_reg, &src_reg);
		break;
	case BPF_SUB:
		ret = sanitize_val_alu(env, insn);
		if (ret < 0) {
			verbose(env, "R%d tried to sub from different pointers or scalars\n", dst);
			return ret;
		}
		scalar_min_max_sub(dst_reg, &src_reg);
		break;
	case BPF_MUL:
		scalar_min_max_mul(dst_reg, &src_reg);
		break;
	case BPF_AND:
		if (src_known && dst_known) {
			__mark_reg_known(dst_reg, dst_reg->var_off.value &
						  src_reg.var_off.value);
			break;
		}
		scalar_min_max_and(dst_reg, &src_reg);
		break;
	case BPF_OR:
		if (src_known && dst_known) {
			__mark_reg_known(dst_reg, dst_reg->var_off.value |
						  src_reg.var_off.value);
			break;
		}
		scalar_min_max_or(dst_reg, &src_reg);
		break;
	case BPF_LSH:
		if (umax_val >= insn_bitness) {
			/* Shifts greater than 31 or 63 are undefined.
			 * This includes shifts by a negative number.
			 */
			mark_reg_unknown(env, regs, insn->dst_reg);
			break;
		}
		scalar_min_max_lsh(dst_reg, &src_reg);
		break;
	case BPF_RSH:
		if (umax_val >= insn_bitness) {
			/* Shifts greater than 31 or 63 are undefined.
			 * This includes shifts by a negative number.
			 */
			mark_reg_unknown(env, regs, insn->dst_reg);
			break;
		}
		scalar_min_max_rsh(dst_reg, &src_reg);
		break;
	case BPF_ARSH:
		if (umax_val >= insn_bitness) {
			/* Shifts greater than 31 or 63 are undefined.
			 * This includes shifts by a negative number.
			 */
			mark_reg_unknown(env, regs, insn->dst_reg);
			break;
		}
		scalar_min_max_arsh(dst_reg, &src_reg, insn_bitness);
		break;
	default:
		mark_reg_unknown(env, regs, insn->dst_reg);
		break;
	}

	if (BPF_CLASS(insn->code) != BPF_ALU64) {
		/* 32-bit ALU ops are (32,32)->32 */
		coerce_reg_to_size(dst_reg, 4);
	}

	__reg_deduce_bounds(dst_reg);
	__reg_bound_offset(dst_reg);
	return 0;
}