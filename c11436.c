static int kdb_local(kdb_reason_t reason, int error, struct pt_regs *regs,
		     kdb_dbtrap_t db_result)
{
	char *cmdbuf;
	int diag;
	struct task_struct *kdb_current =
		kdb_curr_task(raw_smp_processor_id());

	KDB_DEBUG_STATE("kdb_local 1", reason);
	kdb_go_count = 0;
	if (reason == KDB_REASON_DEBUG) {
		/* special case below */
	} else {
		kdb_printf("\nEntering kdb (current=0x%px, pid %d) ",
			   kdb_current, kdb_current ? kdb_current->pid : 0);
#if defined(CONFIG_SMP)
		kdb_printf("on processor %d ", raw_smp_processor_id());
#endif
	}

	switch (reason) {
	case KDB_REASON_DEBUG:
	{
		/*
		 * If re-entering kdb after a single step
		 * command, don't print the message.
		 */
		switch (db_result) {
		case KDB_DB_BPT:
			kdb_printf("\nEntering kdb (0x%px, pid %d) ",
				   kdb_current, kdb_current->pid);
#if defined(CONFIG_SMP)
			kdb_printf("on processor %d ", raw_smp_processor_id());
#endif
			kdb_printf("due to Debug @ " kdb_machreg_fmt "\n",
				   instruction_pointer(regs));
			break;
		case KDB_DB_SS:
			break;
		case KDB_DB_SSBPT:
			KDB_DEBUG_STATE("kdb_local 4", reason);
			return 1;	/* kdba_db_trap did the work */
		default:
			kdb_printf("kdb: Bad result from kdba_db_trap: %d\n",
				   db_result);
			break;
		}

	}
		break;
	case KDB_REASON_ENTER:
		if (KDB_STATE(KEYBOARD))
			kdb_printf("due to Keyboard Entry\n");
		else
			kdb_printf("due to KDB_ENTER()\n");
		break;
	case KDB_REASON_KEYBOARD:
		KDB_STATE_SET(KEYBOARD);
		kdb_printf("due to Keyboard Entry\n");
		break;
	case KDB_REASON_ENTER_SLAVE:
		/* drop through, slaves only get released via cpu switch */
	case KDB_REASON_SWITCH:
		kdb_printf("due to cpu switch\n");
		break;
	case KDB_REASON_OOPS:
		kdb_printf("Oops: %s\n", kdb_diemsg);
		kdb_printf("due to oops @ " kdb_machreg_fmt "\n",
			   instruction_pointer(regs));
		kdb_dumpregs(regs);
		break;
	case KDB_REASON_SYSTEM_NMI:
		kdb_printf("due to System NonMaskable Interrupt\n");
		break;
	case KDB_REASON_NMI:
		kdb_printf("due to NonMaskable Interrupt @ "
			   kdb_machreg_fmt "\n",
			   instruction_pointer(regs));
		break;
	case KDB_REASON_SSTEP:
	case KDB_REASON_BREAK:
		kdb_printf("due to %s @ " kdb_machreg_fmt "\n",
			   reason == KDB_REASON_BREAK ?
			   "Breakpoint" : "SS trap", instruction_pointer(regs));
		/*
		 * Determine if this breakpoint is one that we
		 * are interested in.
		 */
		if (db_result != KDB_DB_BPT) {
			kdb_printf("kdb: error return from kdba_bp_trap: %d\n",
				   db_result);
			KDB_DEBUG_STATE("kdb_local 6", reason);
			return 0;	/* Not for us, dismiss it */
		}
		break;
	case KDB_REASON_RECURSE:
		kdb_printf("due to Recursion @ " kdb_machreg_fmt "\n",
			   instruction_pointer(regs));
		break;
	default:
		kdb_printf("kdb: unexpected reason code: %d\n", reason);
		KDB_DEBUG_STATE("kdb_local 8", reason);
		return 0;	/* Not for us, dismiss it */
	}

	while (1) {
		/*
		 * Initialize pager context.
		 */
		kdb_nextline = 1;
		KDB_STATE_CLEAR(SUPPRESS);
		kdb_grepping_flag = 0;
		/* ensure the old search does not leak into '/' commands */
		kdb_grep_string[0] = '\0';

		cmdbuf = cmd_cur;
		*cmdbuf = '\0';
		*(cmd_hist[cmd_head]) = '\0';

do_full_getstr:
		/* PROMPT can only be set if we have MEM_READ permission. */
		snprintf(kdb_prompt_str, CMD_BUFLEN, kdbgetenv("PROMPT"),
			 raw_smp_processor_id());
		if (defcmd_in_progress)
			strncat(kdb_prompt_str, "[defcmd]", CMD_BUFLEN);

		/*
		 * Fetch command from keyboard
		 */
		cmdbuf = kdb_getstr(cmdbuf, CMD_BUFLEN, kdb_prompt_str);
		if (*cmdbuf != '\n') {
			if (*cmdbuf < 32) {
				if (cmdptr == cmd_head) {
					strscpy(cmd_hist[cmd_head], cmd_cur,
						CMD_BUFLEN);
					*(cmd_hist[cmd_head] +
					  strlen(cmd_hist[cmd_head])-1) = '\0';
				}
				if (!handle_ctrl_cmd(cmdbuf))
					*(cmd_cur+strlen(cmd_cur)-1) = '\0';
				cmdbuf = cmd_cur;
				goto do_full_getstr;
			} else {
				strscpy(cmd_hist[cmd_head], cmd_cur,
					CMD_BUFLEN);
			}

			cmd_head = (cmd_head+1) % KDB_CMD_HISTORY_COUNT;
			if (cmd_head == cmd_tail)
				cmd_tail = (cmd_tail+1) % KDB_CMD_HISTORY_COUNT;
		}

		cmdptr = cmd_head;
		diag = kdb_parse(cmdbuf);
		if (diag == KDB_NOTFOUND) {
			drop_newline(cmdbuf);
			kdb_printf("Unknown kdb command: '%s'\n", cmdbuf);
			diag = 0;
		}
		if (diag == KDB_CMD_GO
		 || diag == KDB_CMD_CPU
		 || diag == KDB_CMD_SS
		 || diag == KDB_CMD_KGDB)
			break;

		if (diag)
			kdb_cmderror(diag);
	}
	KDB_DEBUG_STATE("kdb_local 9", diag);
	return diag;
}