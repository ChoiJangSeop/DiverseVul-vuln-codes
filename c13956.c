static int check_mem_access(struct bpf_verifier_env *env, int insn_idx, u32 regno, int off,
			    int bpf_size, enum bpf_access_type t,
			    int value_regno)
{
	struct bpf_verifier_state *state = env->cur_state;
	struct bpf_reg_state *regs = cur_regs(env);
	struct bpf_reg_state *reg = regs + regno;
	int size, err = 0;

	size = bpf_size_to_bytes(bpf_size);
	if (size < 0)
		return size;

	/* alignment checks will add in reg->off themselves */
	err = check_ptr_alignment(env, reg, off, size);
	if (err)
		return err;

	/* for access checks, reg->off is just part of off */
	off += reg->off;

	if (reg->type == PTR_TO_MAP_VALUE) {
		if (t == BPF_WRITE && value_regno >= 0 &&
		    is_pointer_value(env, value_regno)) {
			verbose(env, "R%d leaks addr into map\n", value_regno);
			return -EACCES;
		}

		err = check_map_access(env, regno, off, size, false);
		if (!err && t == BPF_READ && value_regno >= 0)
			mark_reg_unknown(env, regs, value_regno);

	} else if (reg->type == PTR_TO_CTX) {
		enum bpf_reg_type reg_type = SCALAR_VALUE;

		if (t == BPF_WRITE && value_regno >= 0 &&
		    is_pointer_value(env, value_regno)) {
			verbose(env, "R%d leaks addr into ctx\n", value_regno);
			return -EACCES;
		}
		/* ctx accesses must be at a fixed offset, so that we can
		 * determine what type of data were returned.
		 */
		if (reg->off) {
			verbose(env,
				"dereference of modified ctx ptr R%d off=%d+%d, ctx+const is allowed, ctx+const+const is not\n",
				regno, reg->off, off - reg->off);
			return -EACCES;
		}
		if (!tnum_is_const(reg->var_off) || reg->var_off.value) {
			char tn_buf[48];

			tnum_strn(tn_buf, sizeof(tn_buf), reg->var_off);
			verbose(env,
				"variable ctx access var_off=%s off=%d size=%d",
				tn_buf, off, size);
			return -EACCES;
		}
		err = check_ctx_access(env, insn_idx, off, size, t, &reg_type);
		if (!err && t == BPF_READ && value_regno >= 0) {
			/* ctx access returns either a scalar, or a
			 * PTR_TO_PACKET[_META,_END]. In the latter
			 * case, we know the offset is zero.
			 */
			if (reg_type == SCALAR_VALUE)
				mark_reg_unknown(env, regs, value_regno);
			else
				mark_reg_known_zero(env, regs,
						    value_regno);
			regs[value_regno].id = 0;
			regs[value_regno].off = 0;
			regs[value_regno].range = 0;
			regs[value_regno].type = reg_type;
		}

	} else if (reg->type == PTR_TO_STACK) {
		/* stack accesses must be at a fixed offset, so that we can
		 * determine what type of data were returned.
		 * See check_stack_read().
		 */
		if (!tnum_is_const(reg->var_off)) {
			char tn_buf[48];

			tnum_strn(tn_buf, sizeof(tn_buf), reg->var_off);
			verbose(env, "variable stack access var_off=%s off=%d size=%d",
				tn_buf, off, size);
			return -EACCES;
		}
		off += reg->var_off.value;
		if (off >= 0 || off < -MAX_BPF_STACK) {
			verbose(env, "invalid stack off=%d size=%d\n", off,
				size);
			return -EACCES;
		}

		if (env->prog->aux->stack_depth < -off)
			env->prog->aux->stack_depth = -off;

		if (t == BPF_WRITE)
			err = check_stack_write(env, state, off, size,
						value_regno);
		else
			err = check_stack_read(env, state, off, size,
					       value_regno);
	} else if (reg_is_pkt_pointer(reg)) {
		if (t == BPF_WRITE && !may_access_direct_pkt_data(env, NULL, t)) {
			verbose(env, "cannot write into packet\n");
			return -EACCES;
		}
		if (t == BPF_WRITE && value_regno >= 0 &&
		    is_pointer_value(env, value_regno)) {
			verbose(env, "R%d leaks addr into packet\n",
				value_regno);
			return -EACCES;
		}
		err = check_packet_access(env, regno, off, size, false);
		if (!err && t == BPF_READ && value_regno >= 0)
			mark_reg_unknown(env, regs, value_regno);
	} else {
		verbose(env, "R%d invalid mem access '%s'\n", regno,
			reg_type_str[reg->type]);
		return -EACCES;
	}

	if (!err && size < BPF_REG_SIZE && value_regno >= 0 && t == BPF_READ &&
	    regs[value_regno].type == SCALAR_VALUE) {
		/* b/h/w load zero-extends, mark upper bits as known 0 */
		regs[value_regno].var_off =
			tnum_cast(regs[value_regno].var_off, size);
		__update_reg_bounds(&regs[value_regno]);
	}
	return err;
}