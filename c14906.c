RZ_API void rz_core_analysis_type_match(RzCore *core, RzAnalysisFunction *fcn, HtUU *loop_table) {
	RzListIter *it;

	rz_return_if_fail(core && core->analysis && fcn);

	if (!core->analysis->esil) {
		eprintf("Please run aeim\n");
		return;
	}

	RzAnalysis *analysis = core->analysis;
	const int mininstrsz = rz_analysis_archinfo(analysis, RZ_ANALYSIS_ARCHINFO_MIN_OP_SIZE);
	const int minopcode = RZ_MAX(1, mininstrsz);
	RzConfigHold *hc = rz_config_hold_new(core->config);
	if (!hc) {
		return;
	}
	RzDebugTrace *dt = NULL;
	RzAnalysisEsilTrace *et = NULL;
	if (!analysis_emul_init(core, hc, &dt, &et) || !fcn) {
		analysis_emul_restore(core, hc, dt, et);
		return;
	}

	// Reserve bigger ht to avoid rehashing
	HtPPOptions opt;
	RzDebugTrace *dtrace = core->dbg->trace;
	opt = dtrace->ht->opt;
	ht_pp_free(dtrace->ht);
	dtrace->ht = ht_pp_new_size(fcn->ninstr, opt.dupvalue, opt.freefn, opt.calcsizeV);
	dtrace->ht->opt = opt;

	HtUP *op_cache = NULL;
	const char *pc = rz_reg_get_name(core->dbg->reg, RZ_REG_NAME_PC);
	if (!pc) {
		goto out_function;
	}
	RzRegItem *r = rz_reg_get(core->dbg->reg, pc, -1);
	if (!r) {
		goto out_function;
	}
	rz_cons_break_push(NULL, NULL);
	rz_list_sort(fcn->bbs, bb_cmpaddr);
	// TODO: The algorithm can be more accurate if blocks are followed by their jmp/fail, not just by address
	RzAnalysisBlock *bb;
	// Create a new context to store the return type propagation state
	struct ReturnTypeAnalysisCtx retctx = {
		.resolved = false,
		.ret_type = NULL,
		.ret_reg = NULL
	};
	struct TypeAnalysisCtx ctx = {
		.retctx = &retctx,
		.cur_idx = 0,
		.prev_dest = NULL,
		.str_flag = false
	};
	rz_list_foreach (fcn->bbs, it, bb) {
		ut64 addr = bb->addr;
		rz_reg_set_value(core->dbg->reg, r, addr);
		ht_up_free(op_cache);
		op_cache = ht_up_new(NULL, free_op_cache_kv, NULL);
		if (!op_cache) {
			break;
		}
		while (1) {
			if (rz_cons_is_breaked()) {
				goto out_function;
			}
			ut64 pcval = rz_reg_getv(analysis->reg, pc);
			if ((addr >= bb->addr + bb->size) || (addr < bb->addr) || pcval != addr) {
				break;
			}
			RzAnalysisOp *aop = op_cache_get(op_cache, core, addr);
			if (!aop) {
				break;
			}
			if (aop->type == RZ_ANALYSIS_OP_TYPE_ILL) {
				addr += minopcode;
				continue;
			}

			// CHECK_ME : why we hold a loop_count here ?
			//          : can we remove it ?
			// when set null, do not track loop count
			if (loop_table) {
				ut64 loop_count = ht_uu_find(loop_table, addr, NULL);
				if (loop_count > LOOP_MAX || aop->type == RZ_ANALYSIS_OP_TYPE_RET) {
					break;
				}
				loop_count += 1;
				ht_uu_update(loop_table, addr, loop_count);
			}

			if (rz_analysis_op_nonlinear(aop->type)) { // skip the instr
				rz_reg_set_value(core->dbg->reg, r, addr + aop->size);
			} else {
				rz_core_esil_step(core, UT64_MAX, NULL, NULL, false);
			}

			RzPVector *ins_traces = analysis->esil->trace->instructions;
			ctx.cur_idx = rz_pvector_len(ins_traces) - 1;
			RzList *fcns = rz_analysis_get_functions_in(analysis, aop->addr);
			if (!fcns) {
				break;
			}
			RzListIter *it;
			RzAnalysisFunction *fcn;
			rz_list_foreach (fcns, it, fcn) {
				propagate_types_among_used_variables(core, op_cache, fcn, bb, aop, &ctx);
			}
			addr += aop->size;
			rz_list_free(fcns);
		}
	}
	// Type propagation for register based args
	void **vit;
	//RzPVector *cloned_vars = (RzPVector *)rz_vector_clone((RzVector *)&fcn->vars);
	rz_pvector_foreach (&fcn->vars, vit) {
		RzAnalysisVar *rvar = *vit;
		if (rvar->kind == RZ_ANALYSIS_VAR_KIND_REG) {
			RzAnalysisVar *lvar = rz_analysis_var_get_dst_var(rvar);
			RzRegItem *i = rz_reg_index_get(analysis->reg, rvar->delta);
			if (!i) {
				continue;
			}
			// Note that every `var_type_set()` call could remove some variables
			// due to the overlaps resolution
			if (lvar) {
				// Propagate local var type = to => register-based var
				var_type_set(analysis, rvar, lvar->type, false);
				// Propagate local var type <= from = register-based var
				var_type_set(analysis, lvar, rvar->type, false);
			}
		}
	}
out_function:
	free(retctx.ret_reg);
	ht_up_free(op_cache);
	rz_cons_break_pop();
	analysis_emul_restore(core, hc, dt, et);
}