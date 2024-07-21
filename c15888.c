static void cmd_anal_esil(RCore *core, const char *input) {
	RAnalEsil *esil = core->anal->esil;
	ut64 addr = core->offset;
	ut64 adr ;
	char *n, *n1;
	int off;
	int stacksize = r_config_get_i (core->config, "esil.stack.depth");
	int iotrap = r_config_get_i (core->config, "esil.iotrap");
	int romem = r_config_get_i (core->config, "esil.romem");
	int stats = r_config_get_i (core->config, "esil.stats");
	int noNULL = r_config_get_i (core->config, "esil.noNULL");
	ut64 until_addr = UT64_MAX;
	unsigned int addrsize = r_config_get_i (core->config, "esil.addr.size");

	const char *until_expr = NULL;
	RAnalOp *op;

	switch (input[0]) {
	case 'p': // "aep"
		switch (input[1]) {
		case 'c':
			if (input[2] == ' ') {
				// seek to this address
				r_core_cmdf (core, "ar PC=%s", input + 3);
				r_core_cmd0 (core, ".ar*");
			} else {
				eprintf ("Missing argument\n");
			}
			break;
		case 0:
			r_anal_pin_list (core->anal);
			break;
		case '-':
			if (input[2])
				addr = r_num_math (core->num, input + 2);
			r_anal_pin_unset (core->anal, addr);
			break;
		case ' ':
			r_anal_pin (core->anal, addr, input + 2);
			break;
		default:
			r_core_cmd_help (core, help_msg_aep);
			break;
		}
		break;
	case 'r': // "aer"
		// 'aer' is an alias for 'ar'
		cmd_anal_reg (core, input + 1);
		break;
	case '*':
		// XXX: this is wip, not working atm
		if (core->anal->esil) {
			r_cons_printf ("trap: %d\n", core->anal->esil->trap);
			r_cons_printf ("trap-code: %d\n", core->anal->esil->trap_code);
		} else {
			eprintf ("esil vm not initialized. run `aei`\n");
		}
		break;
	case ' ':
		//r_anal_esil_eval (core->anal, input+1);
		if (!esil) {
			if (!(core->anal->esil = esil = r_anal_esil_new (stacksize, iotrap, addrsize)))
				return;
		}
		r_anal_esil_setup (esil, core->anal, romem, stats, noNULL); // setup io
		r_anal_esil_set_pc (esil, core->offset);
		r_anal_esil_parse (esil, input + 1);
		r_anal_esil_dumpstack (esil);
		r_anal_esil_stack_free (esil);
		break;
	case 's': // "aes"
		// "aes" "aeso" "aesu" "aesue"
		// aes -> single step
		// aesb -> single step back
		// aeso -> single step over
		// aesu -> until address
		// aesue -> until esil expression
		switch (input[1]) {
		case '?':
			eprintf ("See: ae?~aes\n");
			break;
		case 'l': // "aesl"
		{
			ut64 pc = r_debug_reg_get (core->dbg, "PC");
			RAnalOp *op = r_core_anal_op (core, pc);
// TODO: honor hint
			if (!op) {
				break;
			}
			r_core_esil_step (core, UT64_MAX, NULL, NULL);
			r_debug_reg_set (core->dbg, "PC", pc + op->size);
			r_anal_esil_set_pc (esil, pc + op->size);
			r_core_cmd0 (core, ".ar*");
		} break;
		case 'b': // "aesb"
			if (!r_core_esil_step_back (core)) {
				eprintf ("cannnot step back\n");
			}
			r_core_cmd0 (core, ".ar*");
			break;
		case 'u': // "aesu"
			if (input[2] == 'e') {
				until_expr = input + 3;
			} else {
				until_addr = r_num_math (core->num, input + 2);
			}
			r_core_esil_step (core, until_addr, until_expr, NULL);
			r_core_cmd0 (core, ".ar*");
			break;
		case 'o': // "aeso"
			// step over
			op = r_core_anal_op (core, r_reg_getv (core->anal->reg,
				r_reg_get_name (core->anal->reg, R_REG_NAME_PC)));
			if (op && op->type == R_ANAL_OP_TYPE_CALL) {
				until_addr = op->addr + op->size;
			}
			r_core_esil_step (core, until_addr, until_expr, NULL);
			r_anal_op_free (op);
			r_core_cmd0 (core, ".ar*");
			break;
		case 'p': //"aesp"
			n = strchr (input, ' ');
			n1 = n ? strchr (n + 1, ' ') : NULL;
			if ((!n || !n1) || (!(n + 1) || !(n1 + 1))) {
				eprintf ("aesp [offset] [num]\n");
				break;
			}
			adr = r_num_math (core->num, n + 1);
			off = r_num_math (core->num, n1 + 1);
			cmd_aespc (core, adr, off);
			break;
		case ' ':
			n = strchr (input, ' ');
			if (!(n + 1)) {
				r_core_esil_step (core, until_addr, until_expr, NULL);
				break;
			}
			off = r_num_math (core->num, n + 1);
			cmd_aespc (core, -1, off);
			break;
		default:
			r_core_esil_step (core, until_addr, until_expr, NULL);
			r_core_cmd0 (core, ".ar*");
			break;
		}
		break;
	case 'c': // "aec"
		if (input[1] == '?') { // "aec?"
			r_core_cmd_help (core, help_msg_aec);
		} else if (input[1] == 's') { // "aecs"
			const char *pc = r_reg_get_name (core->anal->reg, R_REG_NAME_PC);
			ut64 newaddr;
			int ret;
			for (;;) {
				op = r_core_anal_op (core, addr);
				if (!op) {
					break;
				}
				if (op->type == R_ANAL_OP_TYPE_SWI) {
					eprintf ("syscall at 0x%08" PFMT64x "\n", addr);
					break;
				}
				if (op->type == R_ANAL_OP_TYPE_TRAP) {
					eprintf ("trap at 0x%08" PFMT64x "\n", addr);
					break;
				}
				ret = r_core_esil_step (core, UT64_MAX, NULL, NULL);
				r_anal_op_free (op);
				op = NULL;
				if (core->anal->esil->trap || core->anal->esil->trap_code) {
					break;
				}
				if (!ret)
					break;
				r_core_cmd0 (core, ".ar*");
				newaddr = r_num_get (core->num, pc);
				if (addr == newaddr) {
					addr++;
					break;
				} else {
					addr = newaddr;
				}
			}
			if (op) {
				r_anal_op_free (op);
			}
		} else {
			// "aec"  -> continue until ^C
			// "aecu" -> until address
			// "aecue" -> until esil expression
			if (input[1] == 'u' && input[2] == 'e')
				until_expr = input + 3;
			else if (input[1] == 'u')
				until_addr = r_num_math (core->num, input + 2);
			else until_expr = "0";
			r_core_esil_step (core, until_addr, until_expr, NULL);
			r_core_cmd0 (core, ".ar*");
		}
		break;
	case 'i': // "aei"
		switch (input[1]) {
		case 's':
		case 'm': // "aeim"
			cmd_esil_mem (core, input + 2);
			break;
		case 'p': // initialize pc = $$
			r_core_cmd0 (core, "ar PC=$$");
			break;
		case '?':
			cmd_esil_mem (core, "?");
			break;
		case '-':
			if (esil) {
				sdb_reset (esil->stats);
			}
			r_anal_esil_free (esil);
			core->anal->esil = NULL;
			break;
		case 0:				//lolololol
			r_anal_esil_free (esil);
			// reinitialize
			{
				const char *pc = r_reg_get_name (core->anal->reg, R_REG_NAME_PC);
				if (r_reg_getv (core->anal->reg, pc) == 0LL) {
					r_core_cmd0 (core, "ar PC=$$");
				}
			}
			if (!(esil = core->anal->esil = r_anal_esil_new (stacksize, iotrap, addrsize))) {
				return;
			}
			r_anal_esil_setup (esil, core->anal, romem, stats, noNULL); // setup io
			esil->verbose = (int)r_config_get_i (core->config, "esil.verbose");
			/* restore user settings for interrupt handling */
			{
				const char *s = r_config_get (core->config, "cmd.esil.intr");
				if (s) {
					char *my = strdup (s);
					if (my) {
						r_config_set (core->config, "cmd.esil.intr", my);
						free (my);
					}
				}
			}
			break;
		}
		break;
	case 'k': // "aek"
		switch (input[1]) {
		case '\0':
			input = "123*";
			/* fall through */
		case ' ':
			if (esil && esil->stats) {
				char *out = sdb_querys (esil->stats, NULL, 0, input + 2);
				if (out) {
					r_cons_println (out);
					free (out);
				}
			} else {
				eprintf ("esil.stats is empty. Run 'aei'\n");
			}
			break;
		case '-':
			if (esil) {
				sdb_reset (esil->stats);
			}
			break;
		}
		break;
	case 'f': // "aef"
	{
		RListIter *iter;
		RAnalBlock *bb;
		RAnalFunction *fcn = r_anal_get_fcn_in (core->anal,
							core->offset, R_ANAL_FCN_TYPE_FCN | R_ANAL_FCN_TYPE_SYM);
		if (fcn) {
			// emulate every instruction in the function recursively across all the basic blocks
			r_list_foreach (fcn->bbs, iter, bb) {
				ut64 pc = bb->addr;
				ut64 end = bb->addr + bb->size;
				RAnalOp op;
				ut8 *buf;
				int ret, bbs = end - pc;
				if (bbs < 1 || bbs > 0xfffff) {
					eprintf ("Invalid block size\n");
				}
		//		eprintf ("[*] Emulating 0x%08"PFMT64x" basic block 0x%08" PFMT64x " - 0x%08" PFMT64x "\r[", fcn->addr, pc, end);
				buf = calloc (1, bbs + 1);
				r_io_read_at (core->io, pc, buf, bbs);
				int left;
				while (pc < end) {
					left = R_MIN (end - pc, 32);
					r_asm_set_pc (core->assembler, pc);
					ret = r_anal_op (core->anal, &op, addr, buf, left, R_ANAL_OP_MASK_ALL); // read overflow
					if (ret) {
						r_reg_set_value_by_role (core->anal->reg, R_REG_NAME_PC, pc);
						r_anal_esil_parse (esil, R_STRBUF_SAFEGET (&op.esil));
						r_anal_esil_dumpstack (esil);
						r_anal_esil_stack_free (esil);
						pc += op.size;
					} else {
						pc += 4; // XXX
					}
				}
			}
		} else {
			eprintf ("Cannot find function at 0x%08" PFMT64x "\n", core->offset);
		}
	} break;
	case 't': // "aet"
		switch (input[1]) {
		case 'r': // "aetr"
		{
			// anal ESIL to REIL.
			RAnalEsil *esil = r_anal_esil_new (stacksize, iotrap, addrsize);
			if (!esil)
				return;
			r_anal_esil_to_reil_setup (esil, core->anal, romem, stats);
			r_anal_esil_set_pc (esil, core->offset);
			r_anal_esil_parse (esil, input + 2);
			r_anal_esil_dumpstack (esil);
			r_anal_esil_free (esil);
			break;
		}
		case 's': // "aets"
			switch (input[2]) {
			case 0:
				r_anal_esil_session_list (esil);
				break;
			case '+':
				r_anal_esil_session_add (esil);
				break;
			default:
				r_core_cmd_help (core, help_msg_aets);
				break;
			}
			break;
		default:
			eprintf ("Unknown command. Use `aetr`.\n");
			break;
		}
		break;
	case 'A': // "aeA"
		if (input[1] == '?') {
			r_core_cmd_help (core, help_msg_aea);
		} else if (input[1] == 'r') {
			cmd_aea (core, 1 + (1<<1), core->offset, r_num_math (core->num, input+2));
		} else if (input[1] == 'w') {
			cmd_aea (core, 1 + (1<<2), core->offset, r_num_math (core->num, input+2));
		} else if (input[1] == 'n') {
			cmd_aea (core, 1 + (1<<3), core->offset, r_num_math (core->num, input+2));
		} else if (input[1] == 'j') {
			cmd_aea (core, 1 + (1<<4), core->offset, r_num_math (core->num, input+2));
		} else if (input[1] == '*') {
			cmd_aea (core, 1 + (1<<5), core->offset, r_num_math (core->num, input+2));
		} else if (input[1] == 'f') {
			RAnalFunction *fcn = r_anal_get_fcn_in (core->anal, core->offset, -1);
			if (fcn) {
				cmd_aea (core, 1, fcn->addr, r_anal_fcn_size (fcn));
			}
		} else {
			cmd_aea (core, 1, core->offset, (int)r_num_math (core->num, input+2));
		}
		break;
	case 'a': // "aea"
		if (input[1] == '?') {
			r_core_cmd_help (core, help_msg_aea);
		} else if (input[1] == 'r') {
			cmd_aea (core, 1<<1, core->offset, r_num_math (core->num, input+2));
		} else if (input[1] == 'w') {
			cmd_aea (core, 1<<2, core->offset, r_num_math (core->num, input+2));
		} else if (input[1] == 'n') {
			cmd_aea (core, 1<<3, core->offset, r_num_math (core->num, input+2));
		} else if (input[1] == 'j') {
			cmd_aea (core, 1<<4, core->offset, r_num_math (core->num, input+2));
		} else if (input[1] == '*') {
			cmd_aea (core, 1<<5, core->offset, r_num_math (core->num, input+2));
		} else if (input[1] == 'f') {
			RAnalFunction *fcn = r_anal_get_fcn_in (core->anal, core->offset, -1);
                        // "aeafj"
			if (fcn) {
				switch (input[2]) {
				case 'j': // "aeafj"
					cmd_aea (core, 1<<4, fcn->addr, r_anal_fcn_size (fcn));
					break;
				default:
					cmd_aea (core, 1, fcn->addr, r_anal_fcn_size (fcn));
					break;
				}
				break;
			}
		} else {
			const char *arg = input[1]? input + 2: "";
			ut64 len = r_num_math (core->num, arg);
			cmd_aea (core, 0, core->offset, len);
		}
		break;
	case 'x': { // "aex"
		char *hex;
		int ret, bufsz;

		input = r_str_trim_ro (input + 1);
		hex = strdup (input);
		if (!hex) {
			break;
		}

		RAnalOp aop = R_EMPTY;
		bufsz = r_hex_str2bin (hex, (ut8*)hex);
		ret = r_anal_op (core->anal, &aop, core->offset,
			(const ut8*)hex, bufsz, R_ANAL_OP_MASK_ALL);
		if (ret>0) {
			const char *str = R_STRBUF_SAFEGET (&aop.esil);
			char *str2 = r_str_newf (" %s", str);
			cmd_anal_esil (core, str2);
			free (str2);
		}
		r_anal_op_fini (&aop);
		break;
	}
	case '?': // "ae?"
		if (input[1] == '?') {
			r_core_cmd_help (core, help_detail_ae);
			break;
		}
		/* fallthrough */
	default:
		r_core_cmd_help (core, help_msg_ae);
		break;
	}
}