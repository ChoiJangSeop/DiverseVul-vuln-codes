RZ_API void rz_core_analysis_esil(RzCore *core, const char *str, const char *target) {
	bool cfg_analysis_strings = rz_config_get_i(core->config, "analysis.strings");
	bool emu_lazy = rz_config_get_i(core->config, "emu.lazy");
	bool gp_fixed = rz_config_get_i(core->config, "analysis.gpfixed");
	RzAnalysisEsil *ESIL = core->analysis->esil;
	ut64 refptr = 0LL;
	const char *pcname;
	RzAnalysisOp op = RZ_EMPTY;
	ut8 *buf = NULL;
	bool end_address_set = false;
	int iend;
	int minopsize = 4; // XXX this depends on asm->mininstrsize
	bool archIsArm = false;
	ut64 addr = core->offset;
	ut64 start = addr;
	ut64 end = 0LL;
	ut64 cur;

	if (!strcmp(str, "?")) {
		eprintf("Usage: aae[f] [len] [addr] - analyze refs in function, section or len bytes with esil\n");
		eprintf("  aae $SS @ $S             - analyze the whole section\n");
		eprintf("  aae $SS str.Hello @ $S   - find references for str.Hellow\n");
		eprintf("  aaef                     - analyze functions discovered with esil\n");
		return;
	}
#define CHECKREF(x) ((refptr && (x) == refptr) || !refptr)
	if (target) {
		const char *expr = rz_str_trim_head_ro(target);
		if (*expr) {
			refptr = ntarget = rz_num_math(core->num, expr);
			if (!refptr) {
				ntarget = refptr = addr;
			}
		} else {
			ntarget = UT64_MAX;
			refptr = 0LL;
		}
	} else {
		ntarget = UT64_MAX;
		refptr = 0LL;
	}
	RzAnalysisFunction *fcn = NULL;
	if (!strcmp(str, "f")) {
		fcn = rz_analysis_get_fcn_in(core->analysis, core->offset, 0);
		if (fcn) {
			start = rz_analysis_function_min_addr(fcn);
			addr = fcn->addr;
			end = rz_analysis_function_max_addr(fcn);
			end_address_set = true;
		}
	}

	if (!end_address_set) {
		if (str[0] == ' ') {
			end = addr + rz_num_math(core->num, str + 1);
		} else {
			RzIOMap *map = rz_io_map_get(core->io, addr);
			if (map) {
				end = map->itv.addr + map->itv.size;
			} else {
				end = addr + core->blocksize;
			}
		}
	}

	iend = end - start;
	if (iend < 0) {
		return;
	}
	if (iend > MAX_SCAN_SIZE) {
		eprintf("Warning: Not going to analyze 0x%08" PFMT64x " bytes.\n", (ut64)iend);
		return;
	}
	buf = malloc((size_t)iend + 2);
	if (!buf) {
		perror("malloc");
		return;
	}
	esilbreak_last_read = UT64_MAX;
	rz_io_read_at(core->io, start, buf, iend + 1);
	if (!ESIL) {
		rz_core_analysis_esil_reinit(core);
		ESIL = core->analysis->esil;
		if (!ESIL) {
			eprintf("ESIL not initialized\n");
			return;
		}
		rz_core_analysis_esil_init_mem(core, NULL, UT64_MAX, UT32_MAX);
	}
	const char *spname = rz_reg_get_name(core->analysis->reg, RZ_REG_NAME_SP);
	EsilBreakCtx ctx = {
		&op,
		fcn,
		spname,
		rz_reg_getv(core->analysis->reg, spname)
	};
	ESIL->cb.hook_reg_write = &esilbreak_reg_write;
	//this is necessary for the hook to read the id of analop
	ESIL->user = &ctx;
	ESIL->cb.hook_mem_read = &esilbreak_mem_read;
	ESIL->cb.hook_mem_write = &esilbreak_mem_write;

	if (fcn && fcn->reg_save_area) {
		rz_reg_setv(core->analysis->reg, ctx.spname, ctx.initial_sp - fcn->reg_save_area);
	}
	//eprintf ("Analyzing ESIL refs from 0x%"PFMT64x" - 0x%"PFMT64x"\n", addr, end);
	// TODO: backup/restore register state before/after analysis
	pcname = rz_reg_get_name(core->analysis->reg, RZ_REG_NAME_PC);
	if (!pcname || !*pcname) {
		eprintf("Cannot find program counter register in the current profile.\n");
		return;
	}
	esil_analysis_stop = false;
	rz_cons_break_push(cccb, core);

	int arch = -1;
	if (!strcmp(core->analysis->cur->arch, "arm")) {
		switch (core->analysis->bits) {
		case 64: arch = RZ_ARCH_ARM64; break;
		case 32: arch = RZ_ARCH_ARM32; break;
		case 16: arch = RZ_ARCH_THUMB; break;
		}
		archIsArm = true;
	}

	ut64 gp = rz_config_get_i(core->config, "analysis.gp");
	const char *gp_reg = NULL;
	if (!strcmp(core->analysis->cur->arch, "mips")) {
		gp_reg = "gp";
		arch = RZ_ARCH_MIPS;
	}

	const char *sn = rz_reg_get_name(core->analysis->reg, RZ_REG_NAME_SN);
	if (!sn) {
		eprintf("Warning: No SN reg alias for current architecture.\n");
	}
	rz_reg_arena_push(core->analysis->reg);

	IterCtx ictx = { start, end, fcn, NULL };
	size_t i = addr - start;
	do {
		if (esil_analysis_stop || rz_cons_is_breaked()) {
			break;
		}
		size_t i_old = i;
		cur = start + i;
		if (!rz_io_is_valid_offset(core->io, cur, 0)) {
			break;
		}
		{
			RzPVector *list = rz_meta_get_all_in(core->analysis, cur, RZ_META_TYPE_ANY);
			void **it;
			rz_pvector_foreach (list, it) {
				RzIntervalNode *node = *it;
				RzAnalysisMetaItem *meta = node->data;
				switch (meta->type) {
				case RZ_META_TYPE_DATA:
				case RZ_META_TYPE_STRING:
				case RZ_META_TYPE_FORMAT:
					i += 4;
					rz_pvector_free(list);
					goto repeat;
				default:
					break;
				}
			}
			rz_pvector_free(list);
		}

		/* realign address if needed */
		rz_core_seek_arch_bits(core, cur);
		int opalign = core->analysis->pcalign;
		if (opalign > 0) {
			cur -= (cur % opalign);
		}

		rz_analysis_op_fini(&op);
		rz_asm_set_pc(core->rasm, cur);
		if (!rz_analysis_op(core->analysis, &op, cur, buf + i, iend - i, RZ_ANALYSIS_OP_MASK_ESIL | RZ_ANALYSIS_OP_MASK_VAL | RZ_ANALYSIS_OP_MASK_HINT)) {
			i += minopsize - 1; //   XXX dupe in op.size below
		}
		// if (op.type & 0x80000000 || op.type == 0) {
		if (op.type == RZ_ANALYSIS_OP_TYPE_ILL || op.type == RZ_ANALYSIS_OP_TYPE_UNK) {
			// i += 2
			rz_analysis_op_fini(&op);
			goto repeat;
		}
		//we need to check again i because buf+i may goes beyond its boundaries
		//because of i+= minopsize - 1
		if (i > iend) {
			goto repeat;
		}
		if (op.size < 1) {
			i += minopsize - 1;
			goto repeat;
		}
		if (emu_lazy) {
			if (op.type & RZ_ANALYSIS_OP_TYPE_REP) {
				i += op.size - 1;
				goto repeat;
			}
			switch (op.type & RZ_ANALYSIS_OP_TYPE_MASK) {
			case RZ_ANALYSIS_OP_TYPE_JMP:
			case RZ_ANALYSIS_OP_TYPE_CJMP:
			case RZ_ANALYSIS_OP_TYPE_CALL:
			case RZ_ANALYSIS_OP_TYPE_RET:
			case RZ_ANALYSIS_OP_TYPE_ILL:
			case RZ_ANALYSIS_OP_TYPE_NOP:
			case RZ_ANALYSIS_OP_TYPE_UJMP:
			case RZ_ANALYSIS_OP_TYPE_IO:
			case RZ_ANALYSIS_OP_TYPE_LEAVE:
			case RZ_ANALYSIS_OP_TYPE_CRYPTO:
			case RZ_ANALYSIS_OP_TYPE_CPL:
			case RZ_ANALYSIS_OP_TYPE_SYNC:
			case RZ_ANALYSIS_OP_TYPE_SWI:
			case RZ_ANALYSIS_OP_TYPE_CMP:
			case RZ_ANALYSIS_OP_TYPE_ACMP:
			case RZ_ANALYSIS_OP_TYPE_NULL:
			case RZ_ANALYSIS_OP_TYPE_CSWI:
			case RZ_ANALYSIS_OP_TYPE_TRAP:
				i += op.size - 1;
				goto repeat;
			//  those require write support
			case RZ_ANALYSIS_OP_TYPE_PUSH:
			case RZ_ANALYSIS_OP_TYPE_POP:
				i += op.size - 1;
				goto repeat;
			}
		}
		if (sn && op.type == RZ_ANALYSIS_OP_TYPE_SWI) {
			rz_flag_space_set(core->flags, RZ_FLAGS_FS_SYSCALLS);
			int snv = (arch == RZ_ARCH_THUMB) ? op.val : (int)rz_reg_getv(core->analysis->reg, sn);
			RzSyscallItem *si = rz_syscall_get(core->analysis->syscall, snv, -1);
			if (si) {
				//	eprintf ("0x%08"PFMT64x" SYSCALL %-4d %s\n", cur, snv, si->name);
				rz_flag_set_next(core->flags, sdb_fmt("syscall.%s", si->name), cur, 1);
				rz_syscall_item_free(si);
			} else {
				//todo were doing less filtering up top because we can't match against 80 on all platforms
				// might get too many of this path now..
				//	eprintf ("0x%08"PFMT64x" SYSCALL %d\n", cur, snv);
				rz_flag_set_next(core->flags, sdb_fmt("syscall.%d", snv), cur, 1);
			}
			rz_flag_space_set(core->flags, NULL);
		}
		const char *esilstr = RZ_STRBUF_SAFEGET(&op.esil);
		i += op.size - 1;
		if (!esilstr || !*esilstr) {
			goto repeat;
		}
		rz_analysis_esil_set_pc(ESIL, cur);
		rz_reg_setv(core->analysis->reg, pcname, cur + op.size);
		if (gp_fixed && gp_reg) {
			rz_reg_setv(core->analysis->reg, gp_reg, gp);
		}
		(void)rz_analysis_esil_parse(ESIL, esilstr);
		// looks like ^C is handled by esil_parse !!!!
		//rz_analysis_esil_dumpstack (ESIL);
		//rz_analysis_esil_stack_free (ESIL);
		switch (op.type) {
		case RZ_ANALYSIS_OP_TYPE_LEA:
			// arm64
			if (core->analysis->cur && arch == RZ_ARCH_ARM64) {
				if (CHECKREF(ESIL->cur)) {
					rz_analysis_xrefs_set(core->analysis, cur, ESIL->cur, RZ_ANALYSIS_REF_TYPE_STRING);
				}
			} else if ((target && op.ptr == ntarget) || !target) {
				if (CHECKREF(ESIL->cur)) {
					if (op.ptr && rz_io_is_valid_offset(core->io, op.ptr, !core->analysis->opt.noncode)) {
						rz_analysis_xrefs_set(core->analysis, cur, op.ptr, RZ_ANALYSIS_REF_TYPE_STRING);
					} else {
						rz_analysis_xrefs_set(core->analysis, cur, ESIL->cur, RZ_ANALYSIS_REF_TYPE_STRING);
					}
				}
			}
			if (cfg_analysis_strings) {
				add_string_ref(core, op.addr, op.ptr);
			}
			break;
		case RZ_ANALYSIS_OP_TYPE_ADD:
			/* TODO: test if this is valid for other archs too */
			if (core->analysis->cur && archIsArm) {
				/* This code is known to work on Thumb, ARM and ARM64 */
				ut64 dst = ESIL->cur;
				if ((target && dst == ntarget) || !target) {
					if (CHECKREF(dst)) {
						rz_analysis_xrefs_set(core->analysis, cur, dst, RZ_ANALYSIS_REF_TYPE_DATA);
					}
				}
				if (cfg_analysis_strings) {
					add_string_ref(core, op.addr, dst);
				}
			} else if ((core->analysis->bits == 32 && core->analysis->cur && arch == RZ_ARCH_MIPS)) {
				ut64 dst = ESIL->cur;
				if (!op.src[0] || !op.src[0]->reg || !op.src[0]->reg->name) {
					break;
				}
				if (!strcmp(op.src[0]->reg->name, "sp")) {
					break;
				}
				if (!strcmp(op.src[0]->reg->name, "zero")) {
					break;
				}
				if ((target && dst == ntarget) || !target) {
					if (dst > 0xffff && op.src[1] && (dst & 0xffff) == (op.src[1]->imm & 0xffff) && myvalid(core->io, dst)) {
						RzFlagItem *f;
						char *str;
						if (CHECKREF(dst) || CHECKREF(cur)) {
							rz_analysis_xrefs_set(core->analysis, cur, dst, RZ_ANALYSIS_REF_TYPE_DATA);
							if (cfg_analysis_strings) {
								add_string_ref(core, op.addr, dst);
							}
							if ((f = rz_core_flag_get_by_spaces(core->flags, dst))) {
								rz_meta_set_string(core->analysis, RZ_META_TYPE_COMMENT, cur, f->name);
							} else if ((str = is_string_at(core, dst, NULL))) {
								char *str2 = sdb_fmt("esilref: '%s'", str);
								// HACK avoid format string inside string used later as format
								// string crashes disasm inside agf under some conditions.
								// https://github.com/rizinorg/rizin/issues/6937
								rz_str_replace_char(str2, '%', '&');
								rz_meta_set_string(core->analysis, RZ_META_TYPE_COMMENT, cur, str2);
								free(str);
							}
						}
					}
				}
			}
			break;
		case RZ_ANALYSIS_OP_TYPE_LOAD: {
			ut64 dst = esilbreak_last_read;
			if (dst != UT64_MAX && CHECKREF(dst)) {
				if (myvalid(core->io, dst)) {
					rz_analysis_xrefs_set(core->analysis, cur, dst, RZ_ANALYSIS_REF_TYPE_DATA);
					if (cfg_analysis_strings) {
						add_string_ref(core, op.addr, dst);
					}
				}
			}
			dst = esilbreak_last_data;
			if (dst != UT64_MAX && CHECKREF(dst)) {
				if (myvalid(core->io, dst)) {
					rz_analysis_xrefs_set(core->analysis, cur, dst, RZ_ANALYSIS_REF_TYPE_DATA);
					if (cfg_analysis_strings) {
						add_string_ref(core, op.addr, dst);
					}
				}
			}
		} break;
		case RZ_ANALYSIS_OP_TYPE_JMP: {
			ut64 dst = op.jump;
			if (CHECKREF(dst)) {
				if (myvalid(core->io, dst)) {
					rz_analysis_xrefs_set(core->analysis, cur, dst, RZ_ANALYSIS_REF_TYPE_CODE);
				}
			}
		} break;
		case RZ_ANALYSIS_OP_TYPE_CALL: {
			ut64 dst = op.jump;
			if (CHECKREF(dst)) {
				if (myvalid(core->io, dst)) {
					rz_analysis_xrefs_set(core->analysis, cur, dst, RZ_ANALYSIS_REF_TYPE_CALL);
				}
				ESIL->old = cur + op.size;
				getpcfromstack(core, ESIL);
			}
		} break;
		case RZ_ANALYSIS_OP_TYPE_UJMP:
		case RZ_ANALYSIS_OP_TYPE_RJMP:
		case RZ_ANALYSIS_OP_TYPE_UCALL:
		case RZ_ANALYSIS_OP_TYPE_ICALL:
		case RZ_ANALYSIS_OP_TYPE_RCALL:
		case RZ_ANALYSIS_OP_TYPE_IRCALL:
		case RZ_ANALYSIS_OP_TYPE_MJMP: {
			ut64 dst = core->analysis->esil->jump_target;
			if (dst == 0 || dst == UT64_MAX) {
				dst = rz_reg_getv(core->analysis->reg, pcname);
			}
			if (CHECKREF(dst)) {
				if (myvalid(core->io, dst)) {
					RzAnalysisXRefType ref =
						(op.type & RZ_ANALYSIS_OP_TYPE_MASK) == RZ_ANALYSIS_OP_TYPE_UCALL
						? RZ_ANALYSIS_REF_TYPE_CALL
						: RZ_ANALYSIS_REF_TYPE_CODE;
					rz_analysis_xrefs_set(core->analysis, cur, dst, ref);
					rz_core_analysis_fcn(core, dst, UT64_MAX, RZ_ANALYSIS_REF_TYPE_NULL, 1);
// analyze function here
#if 0
						if (op.type == RZ_ANALYSIS_OP_TYPE_UCALL || op.type == RZ_ANALYSIS_OP_TYPE_RCALL) {
							eprintf ("0x%08"PFMT64x"  RCALL TO %llx\n", cur, dst);
						}
#endif
				}
			}
		} break;
		default:
			break;
		}
		rz_analysis_esil_stack_free(ESIL);
	repeat:
		if (!rz_analysis_get_block_at(core->analysis, cur)) {
			for (size_t bb_i = i_old + 1; bb_i <= i; bb_i++) {
				if (rz_analysis_get_block_at(core->analysis, start + bb_i)) {
					i = bb_i - 1;
					break;
				}
			}
		}
		if (i > iend) {
			break;
		}
	} while (get_next_i(&ictx, &i));
	free(buf);
	ESIL->cb.hook_mem_read = NULL;
	ESIL->cb.hook_mem_write = NULL;
	ESIL->cb.hook_reg_write = NULL;
	ESIL->user = NULL;
	rz_analysis_op_fini(&op);
	rz_cons_break_pop();
	// restore register
	rz_reg_arena_pop(core->analysis->reg);
}