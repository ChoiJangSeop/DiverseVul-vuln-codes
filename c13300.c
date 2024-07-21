static int check_ld_imm(struct bpf_verifier_env *env, struct bpf_insn *insn)
{
	struct bpf_insn_aux_data *aux = cur_aux(env);
	struct bpf_reg_state *regs = cur_regs(env);
	struct bpf_reg_state *dst_reg;
	struct bpf_map *map;
	int err;

	if (BPF_SIZE(insn->code) != BPF_DW) {
		verbose(env, "invalid BPF_LD_IMM insn\n");
		return -EINVAL;
	}
	if (insn->off != 0) {
		verbose(env, "BPF_LD_IMM64 uses reserved fields\n");
		return -EINVAL;
	}

	err = check_reg_arg(env, insn->dst_reg, DST_OP);
	if (err)
		return err;

	dst_reg = &regs[insn->dst_reg];
	if (insn->src_reg == 0) {
		u64 imm = ((u64)(insn + 1)->imm << 32) | (u32)insn->imm;

		dst_reg->type = SCALAR_VALUE;
		__mark_reg_known(&regs[insn->dst_reg], imm);
		return 0;
	}

	if (insn->src_reg == BPF_PSEUDO_BTF_ID) {
		mark_reg_known_zero(env, regs, insn->dst_reg);

		dst_reg->type = aux->btf_var.reg_type;
		switch (dst_reg->type) {
		case PTR_TO_MEM:
			dst_reg->mem_size = aux->btf_var.mem_size;
			break;
		case PTR_TO_BTF_ID:
		case PTR_TO_PERCPU_BTF_ID:
			dst_reg->btf = aux->btf_var.btf;
			dst_reg->btf_id = aux->btf_var.btf_id;
			break;
		default:
			verbose(env, "bpf verifier is misconfigured\n");
			return -EFAULT;
		}
		return 0;
	}

	if (insn->src_reg == BPF_PSEUDO_FUNC) {
		struct bpf_prog_aux *aux = env->prog->aux;
		u32 subprogno = find_subprog(env,
					     env->insn_idx + insn->imm + 1);

		if (!aux->func_info) {
			verbose(env, "missing btf func_info\n");
			return -EINVAL;
		}
		if (aux->func_info_aux[subprogno].linkage != BTF_FUNC_STATIC) {
			verbose(env, "callback function not static\n");
			return -EINVAL;
		}

		dst_reg->type = PTR_TO_FUNC;
		dst_reg->subprogno = subprogno;
		return 0;
	}

	map = env->used_maps[aux->map_index];
	mark_reg_known_zero(env, regs, insn->dst_reg);
	dst_reg->map_ptr = map;

	if (insn->src_reg == BPF_PSEUDO_MAP_VALUE ||
	    insn->src_reg == BPF_PSEUDO_MAP_IDX_VALUE) {
		dst_reg->type = PTR_TO_MAP_VALUE;
		dst_reg->off = aux->map_off;
		if (map_value_has_spin_lock(map))
			dst_reg->id = ++env->id_gen;
	} else if (insn->src_reg == BPF_PSEUDO_MAP_FD ||
		   insn->src_reg == BPF_PSEUDO_MAP_IDX) {
		dst_reg->type = CONST_PTR_TO_MAP;
	} else {
		verbose(env, "bpf verifier is misconfigured\n");
		return -EINVAL;
	}

	return 0;
}