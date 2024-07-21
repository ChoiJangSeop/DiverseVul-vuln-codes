static int check_reg_type(struct bpf_verifier_env *env, u32 regno,
			  enum bpf_arg_type arg_type,
			  const u32 *arg_btf_id)
{
	struct bpf_reg_state *regs = cur_regs(env), *reg = &regs[regno];
	enum bpf_reg_type expected, type = reg->type;
	const struct bpf_reg_types *compatible;
	int i, j;

	compatible = compatible_reg_types[base_type(arg_type)];
	if (!compatible) {
		verbose(env, "verifier internal error: unsupported arg type %d\n", arg_type);
		return -EFAULT;
	}

	for (i = 0; i < ARRAY_SIZE(compatible->types); i++) {
		expected = compatible->types[i];
		if (expected == NOT_INIT)
			break;

		if (type == expected)
			goto found;
	}

	verbose(env, "R%d type=%s expected=", regno, reg_type_str(env, type));
	for (j = 0; j + 1 < i; j++)
		verbose(env, "%s, ", reg_type_str(env, compatible->types[j]));
	verbose(env, "%s\n", reg_type_str(env, compatible->types[j]));
	return -EACCES;

found:
	if (type == PTR_TO_BTF_ID) {
		if (!arg_btf_id) {
			if (!compatible->btf_id) {
				verbose(env, "verifier internal error: missing arg compatible BTF ID\n");
				return -EFAULT;
			}
			arg_btf_id = compatible->btf_id;
		}

		if (!btf_struct_ids_match(&env->log, reg->btf, reg->btf_id, reg->off,
					  btf_vmlinux, *arg_btf_id)) {
			verbose(env, "R%d is of type %s but %s is expected\n",
				regno, kernel_type_name(reg->btf, reg->btf_id),
				kernel_type_name(btf_vmlinux, *arg_btf_id));
			return -EACCES;
		}

		if (!tnum_is_const(reg->var_off) || reg->var_off.value) {
			verbose(env, "R%d is a pointer to in-kernel struct with non-zero offset\n",
				regno);
			return -EACCES;
		}
	}

	return 0;
}