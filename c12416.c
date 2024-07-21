static int check_func_arg(struct bpf_verifier_env *env, u32 arg,
			  struct bpf_call_arg_meta *meta,
			  const struct bpf_func_proto *fn)
{
	u32 regno = BPF_REG_1 + arg;
	struct bpf_reg_state *regs = cur_regs(env), *reg = &regs[regno];
	enum bpf_arg_type arg_type = fn->arg_type[arg];
	enum bpf_reg_type type = reg->type;
	int err = 0;

	if (arg_type == ARG_DONTCARE)
		return 0;

	err = check_reg_arg(env, regno, SRC_OP);
	if (err)
		return err;

	if (arg_type == ARG_ANYTHING) {
		if (is_pointer_value(env, regno)) {
			verbose(env, "R%d leaks addr into helper function\n",
				regno);
			return -EACCES;
		}
		return 0;
	}

	if (type_is_pkt_pointer(type) &&
	    !may_access_direct_pkt_data(env, meta, BPF_READ)) {
		verbose(env, "helper access to the packet is not allowed\n");
		return -EACCES;
	}

	if (base_type(arg_type) == ARG_PTR_TO_MAP_VALUE ||
	    base_type(arg_type) == ARG_PTR_TO_UNINIT_MAP_VALUE) {
		err = resolve_map_arg_type(env, meta, &arg_type);
		if (err)
			return err;
	}

	if (register_is_null(reg) && type_may_be_null(arg_type))
		/* A NULL register has a SCALAR_VALUE type, so skip
		 * type checking.
		 */
		goto skip_type_check;

	err = check_reg_type(env, regno, arg_type, fn->arg_btf_id[arg]);
	if (err)
		return err;

	switch ((u32)type) {
	case SCALAR_VALUE:
	/* Pointer types where reg offset is explicitly allowed: */
	case PTR_TO_PACKET:
	case PTR_TO_PACKET_META:
	case PTR_TO_MAP_KEY:
	case PTR_TO_MAP_VALUE:
	case PTR_TO_MEM:
	case PTR_TO_MEM | MEM_RDONLY:
	case PTR_TO_BUF:
	case PTR_TO_BUF | MEM_RDONLY:
	case PTR_TO_STACK:
		break;
	/* All the rest must be rejected: */
	default:
		err = __check_ptr_off_reg(env, reg, regno,
					  type == PTR_TO_BTF_ID);
		if (err < 0)
			return err;
		break;
	}

skip_type_check:
	if (reg->ref_obj_id) {
		if (meta->ref_obj_id) {
			verbose(env, "verifier internal error: more than one arg with ref_obj_id R%d %u %u\n",
				regno, reg->ref_obj_id,
				meta->ref_obj_id);
			return -EFAULT;
		}
		meta->ref_obj_id = reg->ref_obj_id;
	}

	if (arg_type == ARG_CONST_MAP_PTR) {
		/* bpf_map_xxx(map_ptr) call: remember that map_ptr */
		if (meta->map_ptr) {
			/* Use map_uid (which is unique id of inner map) to reject:
			 * inner_map1 = bpf_map_lookup_elem(outer_map, key1)
			 * inner_map2 = bpf_map_lookup_elem(outer_map, key2)
			 * if (inner_map1 && inner_map2) {
			 *     timer = bpf_map_lookup_elem(inner_map1);
			 *     if (timer)
			 *         // mismatch would have been allowed
			 *         bpf_timer_init(timer, inner_map2);
			 * }
			 *
			 * Comparing map_ptr is enough to distinguish normal and outer maps.
			 */
			if (meta->map_ptr != reg->map_ptr ||
			    meta->map_uid != reg->map_uid) {
				verbose(env,
					"timer pointer in R1 map_uid=%d doesn't match map pointer in R2 map_uid=%d\n",
					meta->map_uid, reg->map_uid);
				return -EINVAL;
			}
		}
		meta->map_ptr = reg->map_ptr;
		meta->map_uid = reg->map_uid;
	} else if (arg_type == ARG_PTR_TO_MAP_KEY) {
		/* bpf_map_xxx(..., map_ptr, ..., key) call:
		 * check that [key, key + map->key_size) are within
		 * stack limits and initialized
		 */
		if (!meta->map_ptr) {
			/* in function declaration map_ptr must come before
			 * map_key, so that it's verified and known before
			 * we have to check map_key here. Otherwise it means
			 * that kernel subsystem misconfigured verifier
			 */
			verbose(env, "invalid map_ptr to access map->key\n");
			return -EACCES;
		}
		err = check_helper_mem_access(env, regno,
					      meta->map_ptr->key_size, false,
					      NULL);
	} else if (base_type(arg_type) == ARG_PTR_TO_MAP_VALUE ||
		   base_type(arg_type) == ARG_PTR_TO_UNINIT_MAP_VALUE) {
		if (type_may_be_null(arg_type) && register_is_null(reg))
			return 0;

		/* bpf_map_xxx(..., map_ptr, ..., value) call:
		 * check [value, value + map->value_size) validity
		 */
		if (!meta->map_ptr) {
			/* kernel subsystem misconfigured verifier */
			verbose(env, "invalid map_ptr to access map->value\n");
			return -EACCES;
		}
		meta->raw_mode = (arg_type == ARG_PTR_TO_UNINIT_MAP_VALUE);
		err = check_helper_mem_access(env, regno,
					      meta->map_ptr->value_size, false,
					      meta);
	} else if (arg_type == ARG_PTR_TO_PERCPU_BTF_ID) {
		if (!reg->btf_id) {
			verbose(env, "Helper has invalid btf_id in R%d\n", regno);
			return -EACCES;
		}
		meta->ret_btf = reg->btf;
		meta->ret_btf_id = reg->btf_id;
	} else if (arg_type == ARG_PTR_TO_SPIN_LOCK) {
		if (meta->func_id == BPF_FUNC_spin_lock) {
			if (process_spin_lock(env, regno, true))
				return -EACCES;
		} else if (meta->func_id == BPF_FUNC_spin_unlock) {
			if (process_spin_lock(env, regno, false))
				return -EACCES;
		} else {
			verbose(env, "verifier internal error\n");
			return -EFAULT;
		}
	} else if (arg_type == ARG_PTR_TO_TIMER) {
		if (process_timer_func(env, regno, meta))
			return -EACCES;
	} else if (arg_type == ARG_PTR_TO_FUNC) {
		meta->subprogno = reg->subprogno;
	} else if (arg_type_is_mem_ptr(arg_type)) {
		/* The access to this pointer is only checked when we hit the
		 * next is_mem_size argument below.
		 */
		meta->raw_mode = (arg_type == ARG_PTR_TO_UNINIT_MEM);
	} else if (arg_type_is_mem_size(arg_type)) {
		bool zero_size_allowed = (arg_type == ARG_CONST_SIZE_OR_ZERO);

		/* This is used to refine r0 return value bounds for helpers
		 * that enforce this value as an upper bound on return values.
		 * See do_refine_retval_range() for helpers that can refine
		 * the return value. C type of helper is u32 so we pull register
		 * bound from umax_value however, if negative verifier errors
		 * out. Only upper bounds can be learned because retval is an
		 * int type and negative retvals are allowed.
		 */
		meta->msize_max_value = reg->umax_value;

		/* The register is SCALAR_VALUE; the access check
		 * happens using its boundaries.
		 */
		if (!tnum_is_const(reg->var_off))
			/* For unprivileged variable accesses, disable raw
			 * mode so that the program is required to
			 * initialize all the memory that the helper could
			 * just partially fill up.
			 */
			meta = NULL;

		if (reg->smin_value < 0) {
			verbose(env, "R%d min value is negative, either use unsigned or 'var &= const'\n",
				regno);
			return -EACCES;
		}

		if (reg->umin_value == 0) {
			err = check_helper_mem_access(env, regno - 1, 0,
						      zero_size_allowed,
						      meta);
			if (err)
				return err;
		}

		if (reg->umax_value >= BPF_MAX_VAR_SIZ) {
			verbose(env, "R%d unbounded memory access, use 'var &= const' or 'if (var < const)'\n",
				regno);
			return -EACCES;
		}
		err = check_helper_mem_access(env, regno - 1,
					      reg->umax_value,
					      zero_size_allowed, meta);
		if (!err)
			err = mark_chain_precision(env, regno);
	} else if (arg_type_is_alloc_size(arg_type)) {
		if (!tnum_is_const(reg->var_off)) {
			verbose(env, "R%d is not a known constant'\n",
				regno);
			return -EACCES;
		}
		meta->mem_size = reg->var_off.value;
	} else if (arg_type_is_int_ptr(arg_type)) {
		int size = int_ptr_type_to_size(arg_type);

		err = check_helper_mem_access(env, regno, size, false, meta);
		if (err)
			return err;
		err = check_ptr_alignment(env, reg, 0, size, true);
	} else if (arg_type == ARG_PTR_TO_CONST_STR) {
		struct bpf_map *map = reg->map_ptr;
		int map_off;
		u64 map_addr;
		char *str_ptr;

		if (!bpf_map_is_rdonly(map)) {
			verbose(env, "R%d does not point to a readonly map'\n", regno);
			return -EACCES;
		}

		if (!tnum_is_const(reg->var_off)) {
			verbose(env, "R%d is not a constant address'\n", regno);
			return -EACCES;
		}

		if (!map->ops->map_direct_value_addr) {
			verbose(env, "no direct value access support for this map type\n");
			return -EACCES;
		}

		err = check_map_access(env, regno, reg->off,
				       map->value_size - reg->off, false);
		if (err)
			return err;

		map_off = reg->off + reg->var_off.value;
		err = map->ops->map_direct_value_addr(map, &map_addr, map_off);
		if (err) {
			verbose(env, "direct value access on string failed\n");
			return err;
		}

		str_ptr = (char *)(long)(map_addr);
		if (!strnchr(str_ptr + map_off, map->value_size - map_off, 0)) {
			verbose(env, "string is not zero-terminated\n");
			return -EINVAL;
		}
	}

	return err;
}