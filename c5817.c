static int check_stack_boundary(struct bpf_verifier_env *env, int regno,
				int access_size, bool zero_size_allowed,
				struct bpf_call_arg_meta *meta)
{
	struct bpf_verifier_state *state = env->cur_state;
	struct bpf_reg_state *regs = state->regs;
	int off, i, slot, spi;

	if (regs[regno].type != PTR_TO_STACK) {
		/* Allow zero-byte read from NULL, regardless of pointer type */
		if (zero_size_allowed && access_size == 0 &&
		    register_is_null(regs[regno]))
			return 0;

		verbose(env, "R%d type=%s expected=%s\n", regno,
			reg_type_str[regs[regno].type],
			reg_type_str[PTR_TO_STACK]);
		return -EACCES;
	}

	/* Only allow fixed-offset stack reads */
	if (!tnum_is_const(regs[regno].var_off)) {
		char tn_buf[48];

		tnum_strn(tn_buf, sizeof(tn_buf), regs[regno].var_off);
		verbose(env, "invalid variable stack read R%d var_off=%s\n",
			regno, tn_buf);
	}
	off = regs[regno].off + regs[regno].var_off.value;
	if (off >= 0 || off < -MAX_BPF_STACK || off + access_size > 0 ||
	    access_size < 0 || (access_size == 0 && !zero_size_allowed)) {
		verbose(env, "invalid stack type R%d off=%d access_size=%d\n",
			regno, off, access_size);
		return -EACCES;
	}

	if (env->prog->aux->stack_depth < -off)
		env->prog->aux->stack_depth = -off;

	if (meta && meta->raw_mode) {
		meta->access_size = access_size;
		meta->regno = regno;
		return 0;
	}

	for (i = 0; i < access_size; i++) {
		slot = -(off + i) - 1;
		spi = slot / BPF_REG_SIZE;
		if (state->allocated_stack <= slot ||
		    state->stack[spi].slot_type[slot % BPF_REG_SIZE] !=
			STACK_MISC) {
			verbose(env, "invalid indirect read from stack off %d+%d size %d\n",
				off, i, access_size);
			return -EACCES;
		}
	}
	return 0;
}