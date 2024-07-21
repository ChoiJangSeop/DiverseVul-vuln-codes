static int do_check(struct bpf_verifier_env *env)
{
	struct bpf_verifier_state *state = &env->cur_state;
	struct bpf_insn *insns = env->prog->insnsi;
	struct bpf_reg_state *regs = state->regs;
	int insn_cnt = env->prog->len;
	int insn_idx, prev_insn_idx = 0;
	int insn_processed = 0;
	bool do_print_state = false;

	init_reg_state(regs);
	insn_idx = 0;
	env->varlen_map_value_access = false;
	for (;;) {
		struct bpf_insn *insn;
		u8 class;
		int err;

		if (insn_idx >= insn_cnt) {
			verbose("invalid insn idx %d insn_cnt %d\n",
				insn_idx, insn_cnt);
			return -EFAULT;
		}

		insn = &insns[insn_idx];
		class = BPF_CLASS(insn->code);

		if (++insn_processed > BPF_COMPLEXITY_LIMIT_INSNS) {
			verbose("BPF program is too large. Processed %d insn\n",
				insn_processed);
			return -E2BIG;
		}

		err = is_state_visited(env, insn_idx);
		if (err < 0)
			return err;
		if (err == 1) {
			/* found equivalent state, can prune the search */
			if (log_level) {
				if (do_print_state)
					verbose("\nfrom %d to %d: safe\n",
						prev_insn_idx, insn_idx);
				else
					verbose("%d: safe\n", insn_idx);
			}
			goto process_bpf_exit;
		}

		if (log_level && do_print_state) {
			verbose("\nfrom %d to %d:", prev_insn_idx, insn_idx);
			print_verifier_state(&env->cur_state);
			do_print_state = false;
		}

		if (log_level) {
			verbose("%d: ", insn_idx);
			print_bpf_insn(insn);
		}

		err = ext_analyzer_insn_hook(env, insn_idx, prev_insn_idx);
		if (err)
			return err;

		if (class == BPF_ALU || class == BPF_ALU64) {
			err = check_alu_op(env, insn);
			if (err)
				return err;

		} else if (class == BPF_LDX) {
			enum bpf_reg_type *prev_src_type, src_reg_type;

			/* check for reserved fields is already done */

			/* check src operand */
			err = check_reg_arg(regs, insn->src_reg, SRC_OP);
			if (err)
				return err;

			err = check_reg_arg(regs, insn->dst_reg, DST_OP_NO_MARK);
			if (err)
				return err;

			src_reg_type = regs[insn->src_reg].type;

			/* check that memory (src_reg + off) is readable,
			 * the state of dst_reg will be updated by this func
			 */
			err = check_mem_access(env, insn->src_reg, insn->off,
					       BPF_SIZE(insn->code), BPF_READ,
					       insn->dst_reg);
			if (err)
				return err;

			if (BPF_SIZE(insn->code) != BPF_W &&
			    BPF_SIZE(insn->code) != BPF_DW) {
				insn_idx++;
				continue;
			}

			prev_src_type = &env->insn_aux_data[insn_idx].ptr_type;

			if (*prev_src_type == NOT_INIT) {
				/* saw a valid insn
				 * dst_reg = *(u32 *)(src_reg + off)
				 * save type to validate intersecting paths
				 */
				*prev_src_type = src_reg_type;

			} else if (src_reg_type != *prev_src_type &&
				   (src_reg_type == PTR_TO_CTX ||
				    *prev_src_type == PTR_TO_CTX)) {
				/* ABuser program is trying to use the same insn
				 * dst_reg = *(u32*) (src_reg + off)
				 * with different pointer types:
				 * src_reg == ctx in one branch and
				 * src_reg == stack|map in some other branch.
				 * Reject it.
				 */
				verbose("same insn cannot be used with different pointers\n");
				return -EINVAL;
			}

		} else if (class == BPF_STX) {
			enum bpf_reg_type *prev_dst_type, dst_reg_type;

			if (BPF_MODE(insn->code) == BPF_XADD) {
				err = check_xadd(env, insn);
				if (err)
					return err;
				insn_idx++;
				continue;
			}

			/* check src1 operand */
			err = check_reg_arg(regs, insn->src_reg, SRC_OP);
			if (err)
				return err;
			/* check src2 operand */
			err = check_reg_arg(regs, insn->dst_reg, SRC_OP);
			if (err)
				return err;

			dst_reg_type = regs[insn->dst_reg].type;

			/* check that memory (dst_reg + off) is writeable */
			err = check_mem_access(env, insn->dst_reg, insn->off,
					       BPF_SIZE(insn->code), BPF_WRITE,
					       insn->src_reg);
			if (err)
				return err;

			prev_dst_type = &env->insn_aux_data[insn_idx].ptr_type;

			if (*prev_dst_type == NOT_INIT) {
				*prev_dst_type = dst_reg_type;
			} else if (dst_reg_type != *prev_dst_type &&
				   (dst_reg_type == PTR_TO_CTX ||
				    *prev_dst_type == PTR_TO_CTX)) {
				verbose("same insn cannot be used with different pointers\n");
				return -EINVAL;
			}

		} else if (class == BPF_ST) {
			if (BPF_MODE(insn->code) != BPF_MEM ||
			    insn->src_reg != BPF_REG_0) {
				verbose("BPF_ST uses reserved fields\n");
				return -EINVAL;
			}
			/* check src operand */
			err = check_reg_arg(regs, insn->dst_reg, SRC_OP);
			if (err)
				return err;

			/* check that memory (dst_reg + off) is writeable */
			err = check_mem_access(env, insn->dst_reg, insn->off,
					       BPF_SIZE(insn->code), BPF_WRITE,
					       -1);
			if (err)
				return err;

		} else if (class == BPF_JMP) {
			u8 opcode = BPF_OP(insn->code);

			if (opcode == BPF_CALL) {
				if (BPF_SRC(insn->code) != BPF_K ||
				    insn->off != 0 ||
				    insn->src_reg != BPF_REG_0 ||
				    insn->dst_reg != BPF_REG_0) {
					verbose("BPF_CALL uses reserved fields\n");
					return -EINVAL;
				}

				err = check_call(env, insn->imm, insn_idx);
				if (err)
					return err;

			} else if (opcode == BPF_JA) {
				if (BPF_SRC(insn->code) != BPF_K ||
				    insn->imm != 0 ||
				    insn->src_reg != BPF_REG_0 ||
				    insn->dst_reg != BPF_REG_0) {
					verbose("BPF_JA uses reserved fields\n");
					return -EINVAL;
				}

				insn_idx += insn->off + 1;
				continue;

			} else if (opcode == BPF_EXIT) {
				if (BPF_SRC(insn->code) != BPF_K ||
				    insn->imm != 0 ||
				    insn->src_reg != BPF_REG_0 ||
				    insn->dst_reg != BPF_REG_0) {
					verbose("BPF_EXIT uses reserved fields\n");
					return -EINVAL;
				}

				/* eBPF calling convetion is such that R0 is used
				 * to return the value from eBPF program.
				 * Make sure that it's readable at this time
				 * of bpf_exit, which means that program wrote
				 * something into it earlier
				 */
				err = check_reg_arg(regs, BPF_REG_0, SRC_OP);
				if (err)
					return err;

				if (is_pointer_value(env, BPF_REG_0)) {
					verbose("R0 leaks addr as return value\n");
					return -EACCES;
				}

process_bpf_exit:
				insn_idx = pop_stack(env, &prev_insn_idx);
				if (insn_idx < 0) {
					break;
				} else {
					do_print_state = true;
					continue;
				}
			} else {
				err = check_cond_jmp_op(env, insn, &insn_idx);
				if (err)
					return err;
			}
		} else if (class == BPF_LD) {
			u8 mode = BPF_MODE(insn->code);

			if (mode == BPF_ABS || mode == BPF_IND) {
				err = check_ld_abs(env, insn);
				if (err)
					return err;

			} else if (mode == BPF_IMM) {
				err = check_ld_imm(env, insn);
				if (err)
					return err;

				insn_idx++;
			} else {
				verbose("invalid BPF_LD mode\n");
				return -EINVAL;
			}
			reset_reg_range_values(regs, insn->dst_reg);
		} else {
			verbose("unknown insn class %d\n", class);
			return -EINVAL;
		}

		insn_idx++;
	}

	verbose("processed %d insns\n", insn_processed);
	return 0;
}