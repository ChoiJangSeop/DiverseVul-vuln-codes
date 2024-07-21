static int r_core_cmd_subst_i(RCore *core, char *cmd, char *colon) {
	const char *quotestr = "`";
	const char *tick = NULL;
	char *ptr, *ptr2, *str;
	char *arroba = NULL;
	int i, ret = 0, pipefd;
	bool usemyblock = false;
	int scr_html = -1;
	int scr_color = -1;
	bool eos = false;
	bool haveQuote = false;

	if (!cmd) {
		return 0;
	}
	cmd = r_str_trim_head_tail (cmd);

	/* quoted / raw command */
	switch (*cmd) {
	case '.':
		if (cmd[1] == '"') { /* interpret */
			return r_cmd_call (core->rcmd, cmd);
		}
		break;
	case '"':
		for (; *cmd; ) {
			int pipefd = -1;
			ut64 oseek = UT64_MAX;
			char *line, *p;
			haveQuote = *cmd == '"';
			if (haveQuote) {
			//	*cmd = 0;
				cmd++;
				p = find_eoq (cmd + 1);
				if (!p || !*p) {
					eprintf ("Missing \" in (%s).", cmd);
					return false;
				}
				*p++ = 0;
				if (!*p) {
					eos = true;
				}
			} else {
				char *sc = strchr (cmd, ';');
				if (sc) {
					*sc = 0;
				}
				r_core_cmd0 (core, cmd);
				if (!sc) {
					break;
				}
				cmd = sc + 1;
				continue;
			}
			if (p[0]) {
				// workaround :D
				if (p[0] == '@') {
					p--;
				}
				while (p[1] == ';' || IS_WHITESPACE (p[1])) {
					p++;
				}
				if (p[1] == '@' || (p[1] && p[2] == '@')) {
					char *q = strchr (p + 1, '"');
					if (q) {
						*q = 0;
					}
					haveQuote = q != NULL;
					oseek = core->offset;
					r_core_seek (core,
						     r_num_math (core->num, p + 2), 1);
					if (q) {
						*p = '"';
						p = q;
					} else {
						p = strchr (p + 1, ';');
					}
				}
				if (p && *p && p[1] == '>') {
					str = p + 2;
					while (*str == '>') {
						str++;
					}
					while (IS_WHITESPACE (*str)) {
						str++;
					}
					r_cons_flush ();
					pipefd = r_cons_pipe_open (str, 1, p[2] == '>');
				}
			}
			line = strdup (cmd);
			line = r_str_replace (line, "\\\"", "\"", true);
			if (p && *p && p[1] == '|') {
				str = p + 2;
				while (IS_WHITESPACE (*str)) {
					str++;
				}
				r_core_cmd_pipe (core, cmd, str);
			} else {
				r_cmd_call (core->rcmd, line);
			}
			free (line);
			if (oseek != UT64_MAX) {
				r_core_seek (core, oseek, 1);
				oseek = UT64_MAX;
			}
			if (pipefd != -1) {
				r_cons_flush ();
				r_cons_pipe_close (pipefd);
			}
			if (!p) {
				break;
			}
			if (eos) {
				break;
			}
			if (haveQuote) {
				if (*p == ';') {
					cmd = p + 1;
				} else {
					if (*p == '"') {
						cmd = p + 1;
					} else {
						*p = '"';
						cmd = p;
					}
				}
			} else {
				cmd = p + 1;
			}
		}
		return true;
	case '(':
		if (cmd[1] != '*') {
			return r_cmd_call (core->rcmd, cmd);
		}
	}

// TODO must honor " and `
	/* comments */
	if (*cmd != '#') {
		ptr = (char *)r_str_lastbut (cmd, '#', quotestr);
		if (ptr && (ptr[1] == ' ' || ptr[1] == '\t')) {
			*ptr = '\0';
		}
	}

	/* multiple commands */
	// TODO: must honor " and ` boundaries
	//ptr = strrchr (cmd, ';');
	if (*cmd != '#') {
		ptr = (char *)r_str_lastbut (cmd, ';', quotestr);
		if (colon && ptr) {
			int ret ;
			*ptr = '\0';
			if (r_core_cmd_subst (core, cmd) == -1) {
				return -1;
			}
			cmd = ptr + 1;
			ret = r_core_cmd_subst (core, cmd);
			*ptr = ';';
			return ret;
			//r_cons_flush ();
		}
	}

	// TODO must honor " and `
	/* pipe console to shell process */
	//ptr = strchr (cmd, '|');
	ptr = (char *)r_str_lastbut (cmd, '|', quotestr);
	if (ptr) {
		char *ptr2 = strchr (cmd, '`');
		if (!ptr2 || (ptr2 && ptr2 > ptr)) {
			if (!tick || (tick && tick > ptr)) {
				*ptr = '\0';
				cmd = r_str_clean (cmd);
				if (!strcmp (ptr + 1, "?")) { // "|?"
					// TODO: should be disable scr.color in pd| ?
					eprintf ("Usage: <r2command> | <program|H|>\n");
					eprintf (" pd|?   - show this help\n");
					eprintf (" pd|    - disable scr.html and scr.color\n");
					eprintf (" pd|H   - enable scr.html, respect scr.color\n");
					return ret;
				} else if (!strcmp (ptr + 1, "H")) { // "|H"
					scr_html = r_config_get_i (core->config, "scr.html");
					r_config_set_i (core->config, "scr.html", true);
				} else if (ptr[1]) { // "| grep .."
					int value = core->num->value;
					if (*cmd) {
						r_core_cmd_pipe (core, cmd, ptr + 1);
					} else {
						r_io_system (core->io, ptr + 1);
					}
					core->num->value = value;
					return 0;
				} else { // "|"
					scr_html = r_config_get_i (core->config, "scr.html");
					r_config_set_i (core->config, "scr.html", 0);
					scr_color = r_config_get_i (core->config, "scr.color");
					r_config_set_i (core->config, "scr.color", false);
				}
			}
		}
	}

	// TODO must honor " and `
	/* bool conditions */
	ptr = (char *)r_str_lastbut (cmd, '&', quotestr);
	//ptr = strchr (cmd, '&');
	while (ptr && ptr[1] == '&') {
		*ptr = '\0';
		ret = r_cmd_call (core->rcmd, cmd);
		if (ret == -1) {
			eprintf ("command error(%s)\n", cmd);
			if (scr_html != -1) {
				r_config_set_i (core->config, "scr.html", scr_html);
			}
			if (scr_color != -1) {
				r_config_set_i (core->config, "scr.color", scr_color);
			}
			return ret;
		}
		for (cmd = ptr + 2; cmd && *cmd == ' '; cmd++);
		ptr = strchr (cmd, '&');
	}

	/* Out Of Band Input */
	free (core->oobi);
	core->oobi = NULL;

	ptr = strstr (cmd, "?*");
	if (ptr) {
		char *prech = ptr - 1;
		if (*prech != '~') {
			ptr[1] = 0;
			if (*cmd != '#' && strlen (cmd) < 5) {
				r_cons_break_push (NULL, NULL);
				recursive_help (core, cmd);
				r_cons_break_pop ();
				r_cons_grep_parsecmd (ptr + 2, "`");
				if (scr_html != -1) {
					r_config_set_i (core->config, "scr.html", scr_html);
				}
			if (scr_color != -1) {
				r_config_set_i (core->config, "scr.color", scr_color);
			}
				return 0;
			}
		}
	}

#if 0
	ptr = strchr (cmd, '<');
	if (ptr) {
		ptr[0] = '\0';
		if (r_cons_singleton()->is_interactive) {
			if (ptr[1] == '<') {
				/* this is a bit mess */
				//const char *oprompt = strdup (r_line_singleton ()->prompt);
				//oprompt = ">";
				for (str = ptr + 2; str[0] == ' '; str++) {
					//nothing to see here
				}
				eprintf ("==> Reading from stdin until '%s'\n", str);
				free (core->oobi);
				core->oobi = malloc (1);
				if (core->oobi) {
					core->oobi[0] = '\0';
				}
				core->oobi_len = 0;
				for (;;) {
					char buf[1024];
					int ret;
					write (1, "> ", 2);
					fgets (buf, sizeof (buf) - 1, stdin); // XXX use r_line ??
					if (feof (stdin)) {
						break;
					}
					if (*buf) buf[strlen (buf) - 1]='\0';
					ret = strlen (buf);
					core->oobi_len += ret;
					core->oobi = realloc (core->oobi, core->oobi_len + 1);
					if (core->oobi) {
						if (!strcmp (buf, str)) {
							break;
						}
						strcat ((char *)core->oobi, buf);
					}
				}
				//r_line_set_prompt (oprompt);
			} else {
				for (str = ptr + 1; *str == ' '; str++) {
					//nothing to see here
				}
				if (!*str) {
					goto next;
				}
				eprintf ("Slurping file '%s'\n", str);
				free (core->oobi);
				core->oobi = (ut8*)r_file_slurp (str, &core->oobi_len);
				if (!core->oobi) {
					eprintf ("cannot open file\n");
				} else if (ptr == cmd) {
					return r_core_cmd_buffer (core, (const char *)core->oobi);
				}
			}
		} else {
			eprintf ("Cannot slurp with << in non-interactive mode\n");
			return 0;
		}
	}
next:
#endif
	// TODO must honor " and `
	/* pipe console to file */
	ptr = strchr (cmd, '>');
	if (ptr) {
		int fdn = 1;
		int pipecolor = r_config_get_i (core->config, "scr.pipecolor");
		int use_editor = false;
		int ocolor = r_config_get_i (core->config, "scr.color");
		*ptr = '\0';
		str = r_str_trim_head_tail (ptr + 1 + (ptr[1] == '>'));
		if (!*str) {
			eprintf ("No output?\n");
			goto next2;
		}
		/* r_cons_flush() handles interactive output (to the terminal)
		 * differently (e.g. asking about too long output). This conflicts
		 * with piping to a file. Disable it while piping. */
		if (ptr > (cmd + 1) && ISWHITECHAR (ptr[-2])) {
			char *fdnum = ptr - 1;
			if (*fdnum == 'H') { // "H>"
				scr_html = r_config_get_i (core->config, "scr.html");
				r_config_set_i (core->config, "scr.html", true);
				pipecolor = true;
				*fdnum = 0;
			} else {
				if (IS_DIGIT(*fdnum)) {
					fdn = *fdnum - '0';
				}
				*fdnum = 0;
			}
		}
		r_cons_set_interactive (false);
		if (!strcmp (str, "-")) {
			use_editor = true;
			str = r_file_temp ("dumpedit");
			r_config_set (core->config, "scr.color", "false");
		}
		if (fdn > 0) {
			pipefd = r_cons_pipe_open (str, fdn, ptr[1] == '>');
			if (pipefd != -1) {
				if (!pipecolor) {
					r_config_set_i (core->config, "scr.color", 0);
				}
				ret = r_core_cmd_subst (core, cmd);
				r_cons_flush ();
				r_cons_pipe_close (pipefd);
			}
		}
		r_cons_set_last_interactive ();
		if (!pipecolor) {
			r_config_set_i (core->config, "scr.color", ocolor);
		}
		if (use_editor) {
			const char *editor = r_config_get (core->config, "cfg.editor");
			if (editor && *editor) {
				r_sys_cmdf ("%s '%s'", editor, str);
				r_file_rm (str);
			} else {
				eprintf ("No cfg.editor configured\n");
			}
			r_config_set_i (core->config, "scr.color", ocolor);
			free (str);
		}
		if (scr_html != -1) {
			r_config_set_i (core->config, "scr.html", scr_html);
		}
			if (scr_color != -1) {
				r_config_set_i (core->config, "scr.color", scr_color);
			}
		return ret;
	}
next2:
	/* sub commands */
	ptr = strchr (cmd, '`');
	if (ptr) {
		int empty = 0;
		int oneline = 1;
		if (ptr[1] == '`') {
			memmove (ptr, ptr + 1, strlen (ptr));
			oneline = 0;
			empty = 1;
		}
		ptr2 = strchr (ptr + 1, '`');
		if (empty) {
			/* do nothing */
		} else if (!ptr2) {
			eprintf ("parse: Missing backtick in expression.\n");
			goto fail;
		} else {
			int value = core->num->value;
			*ptr = '\0';
			*ptr2 = '\0';
			if (ptr[1] == '!') {
				str = r_core_cmd_str_pipe (core, ptr + 1);
			} else {
				str = r_core_cmd_str (core, ptr + 1);
			}
			if (!str) {
				goto fail;
			}
			// ignore contents if first char is pipe or comment
			if (*str == '|' || *str == '*') {
				eprintf ("r_core_cmd_subst_i: invalid backticked command\n");
				free (str);
				goto fail;
			}
			if (oneline && str) {
				for (i = 0; str[i]; i++) {
					if (str[i] == '\n') {
						str[i] = ' ';
					}
				}
			}
			str = r_str_append (str, ptr2 + 1);
			cmd = r_str_append (strdup (cmd), str);
			core->num->value = value;
			ret = r_core_cmd_subst (core, cmd);
			free (cmd);
			if (scr_html != -1) {
				r_config_set_i (core->config, "scr.html", scr_html);
			}
			free (str);
			return ret;
		}
	}
	// TODO must honor " and `
	core->fixedblock = false;

	if (r_str_endswith (cmd, "~?") && cmd[2] == '\0') {
		r_cons_grep_help ();
		return true;
	}
	if (*cmd != '.') {
		r_cons_grep_parsecmd (cmd, quotestr);
	}

	/* temporary seek commands */
	if (*cmd!= '(' && *cmd != '"') {
		ptr = strchr (cmd, '@');
		if (ptr == cmd + 1 && *cmd == '?') {
			ptr = NULL;
		}
	} else {
		ptr = NULL;
	}

	core->tmpseek = ptr? true: false;
	int rc = 0;
	if (ptr) {
		char *f, *ptr2 = strchr (ptr + 1, '!');
		ut64 addr = UT64_MAX;
		const char *tmpbits = NULL;
		const char *offstr = NULL;
		ut64 tmpbsz = core->blocksize;
		char *tmpeval = NULL;
		ut64 tmpoff = core->offset;
		char *tmpasm = NULL;
		int tmpfd = -1;
		int sz, len;
		ut8 *buf;

		*ptr = '\0';
		for (ptr++; *ptr == ' '; ptr++) {
			//nothing to see here
		}
		if (*ptr && ptr[1] == ':') {
			/* do nothing here */
		} else {
			ptr--;
		}
		arroba = (ptr[0] && ptr[1] && ptr[2])?
			strchr (ptr + 2, '@'): NULL;
repeat_arroba:
		if (arroba) {
			*arroba = 0;
		}
		if (ptr[1] == '?') {
			helpCmdAt (core);
		} else if (ptr[0] && ptr[1] == ':' && ptr[2]) {
			usemyblock = true;
			switch (ptr[0]) {
			case 'f': // "@f:" // slurp file in block
				f = r_file_slurp (ptr + 2, &sz);
				if (f) {
					buf = malloc (sz);
					if (buf) {
						free (core->block);
						core->block = buf;
						core->blocksize = sz;
						memcpy (core->block, f, sz);
					} else {
						eprintf ("cannot alloc %d", sz);
					}
					free (f);
				} else {
					eprintf ("cannot open '%s'\n", ptr + 3);
				}
				break;
			case 'r': // "@r:" // regname
				if (ptr[1] == ':') {
					ut64 regval;
					char *mander = strdup (ptr + 2);
					char *sep = findSeparator (mander);
					if (sep) {
						char ch = *sep;
						*sep = 0;
						regval = r_debug_reg_get (core->dbg, mander);
						*sep = ch;
						char *numexpr = r_str_newf ("0x%"PFMT64x"%s", regval, sep);
						regval = r_num_math (core->num, numexpr);
						free (numexpr);
					} else {
						regval = r_debug_reg_get (core->dbg, ptr + 2);
					}
					r_core_seek (core, regval, 1);
					free (mander);
				}
				break;
			case 'b': // "@b:" // bits
				tmpbits = strdup (r_config_get (core->config, "asm.bits"));
				r_config_set_i (core->config, "asm.bits",
					r_num_math (core->num, ptr + 2));
				break;
			case 'i': // "@i:"
				{
					ut64 addr = r_num_math (core->num, ptr + 2);
					if (addr) {
						r_core_cmdf (core, "so %s", ptr + 2);
					}
				}
				break;
			case 'e': // "@e:"
				tmpeval = parse_tmp_evals (core, ptr + 2);
				break;
			case 'x': // "@x:" // hexpairs
				if (ptr[1] == ':') {
					buf = malloc (strlen (ptr + 2) + 1);
					if (buf) {
						len = r_hex_str2bin (ptr + 2, buf);
						r_core_block_size (core, R_ABS(len));
						memcpy (core->block, buf, core->blocksize);
						core->fixedblock = true;
						free (buf);
					} else {
						eprintf ("cannot allocate\n");
					}
				} else {
					eprintf ("Invalid @x: syntax\n");
				}
				break;
			case 'k': // "@k"
				 {
					char *out = sdb_querys (core->sdb, NULL, 0, ptr + ((ptr[1])? 2: 1));
					if (out) {
						r_core_seek (core, r_num_math (core->num, out), 1);
						free (out);
					}
				 }
				break;
			case 'o': // "@o:3"
				if (ptr[1] == ':') {
					tmpfd = core->io->raised;
					r_io_raise (core->io, atoi (ptr + 2));
				}
				break;
			case 'a': // "@a:"
				if (ptr[1] == ':') {
					char *q = strchr (ptr + 2, ':');
					tmpasm = strdup (r_config_get (core->config, "asm.arch"));
					if (q) {
						*q++ = 0;
						tmpbits = r_config_get (core->config, "asm.bits");
						r_config_set (core->config, "asm.bits", q);
					}
					r_config_set (core->config, "asm.arch", ptr + 2);
					// TODO: handle asm.bits
				} else {
					eprintf ("Usage: pd 10 @a:arm:32\n");
				}
				break;
			case 's': // "@s:"
				len = strlen (ptr + 2);
				r_core_block_size (core, len);
				memcpy (core->block, ptr + 2, len);
				break;
			default:
				goto ignore;
			}
			*ptr = '@';
			goto next_arroba; //ignore; //return ret;
		}
ignore:
		ptr = r_str_trim_head (ptr + 1);
		ptr--;

		cmd = r_str_clean (cmd);
		if (ptr2) {
			if (strlen (ptr + 1) == 13 && strlen (ptr2 + 1) == 6 &&
			    !memcmp (ptr + 1, "0x", 2) &&
			    !memcmp (ptr2 + 1, "0x", 2)) {
				/* 0xXXXX:0xYYYY */
			} else if (strlen (ptr + 1) == 9 && strlen (ptr2 + 1) == 4) {
				/* XXXX:YYYY */
			} else {
				*ptr2 = '\0';
				if (!ptr2[1]) {
					goto fail;
				}
				r_core_block_size (
					core, r_num_math (core->num, ptr2 + 1));
			}
		}

		offstr = r_str_trim_head (ptr + 1);

		addr = r_num_math (core->num, offstr);
		if (isalpha ((unsigned char)ptr[1]) && !addr) {
			if (!r_flag_get (core->flags, ptr + 1)) {
				eprintf ("Invalid address (%s)\n", ptr + 1);
				goto fail;
			}
		} else {
			char ch = *offstr;
			if (ch == '-' || ch == '+') {
				addr = core->offset + addr;
			}
		}
next_arroba:
		if (arroba) {
			ptr = arroba;
			arroba = NULL;
			goto repeat_arroba;
		}
		if (ptr[1] == '@') {
			// TODO: remove temporally seek (should be done by cmd_foreach)
			if (ptr[2] == '@') {
				char *rule = ptr + 3;
				while (*rule && *rule == ' ') rule++;
				ret = r_core_cmd_foreach3 (core, cmd, rule);
			} else {
				ret = r_core_cmd_foreach (core, cmd, ptr + 2);
			}
			//ret = -1; /* do not run out-of-foreach cmd */
		} else {
			bool tmpseek = false;
			const char *fromvars[] = { "anal.from", "diff.from", "graph.from",
				"io.buffer.from", "lines.from", "search.from", "zoom.from", NULL };
			const char *tovars[] = { "anal.to", "diff.to", "graph.to",
				"io.buffer.to", "lines.to", "search.to", "zoom.to", NULL };
			ut64 curfrom[R_ARRAY_SIZE (fromvars) - 1], curto[R_ARRAY_SIZE (tovars) - 1];

			// @..
			if (ptr[1] == '.' && ptr[2] == '.') {
				char *range = ptr + 3;
				char *p = strchr (range, ' ');
				if (!p) {
					eprintf ("Usage: / ABCD @..0x1000 0x3000\n");
					free (tmpeval);
					free (tmpasm);
					goto fail;
				}
				*p = '\x00';
				ut64 from = r_num_math (core->num, range);
				ut64 to = r_num_math (core->num, p + 1);
				// save current ranges
				for (i = 0; fromvars[i]; i++) {
					curfrom[i] = r_config_get_i (core->config, fromvars[i]);
				}
				for (i = 0; tovars[i]; i++) {
					curto[i] = r_config_get_i (core->config, tovars[i]);
				}
				// set new ranges
				for (i = 0; fromvars[i]; i++) {
					r_config_set_i (core->config, fromvars[i], from);
				}
				for (i = 0; tovars[i]; i++) {
					r_config_set_i (core->config, tovars[i], to);
				}
				tmpseek = true;
			}
			if (usemyblock) {
				if (addr != UT64_MAX) {
					core->offset = addr;
				}
				ret = r_cmd_call (core->rcmd, r_str_trim_head (cmd));
			} else {
				if (addr != UT64_MAX) {
					if (!ptr[1] || r_core_seek (core, addr, 1)) {
						r_core_block_read (core);
						ret = r_cmd_call (core->rcmd, r_str_trim_head (cmd));
					} else {
						ret = 0;
					}
				}
			}
			if (tmpseek) {
				// restore ranges
				for (i = 0; fromvars[i]; i++) {
					r_config_set_i (core->config, fromvars[i], curfrom[i]);
				}
				for (i = 0; tovars[i]; i++) {
					r_config_set_i (core->config, tovars[i], curto[i]);
				}
			}
		}
		if (ptr2) {
			*ptr2 = '!';
			r_core_block_size (core, tmpbsz);
		}
		if (tmpasm) {
			r_config_set (core->config, "asm.arch", tmpasm);
			tmpasm = NULL;
		}
		if (tmpfd != -1) {
			r_io_raise (core->io, tmpfd);
		}
		if (tmpbits) {
			r_config_set (core->config, "asm.bits", tmpbits);
			tmpbits = NULL;
		}
		if (tmpeval) {
			r_core_cmd0 (core, tmpeval);
			R_FREE (tmpeval);
		}
		r_core_seek (core, tmpoff, 1);
		*ptr = '@';
		rc = ret;
		goto beach;
	}

	rc = cmd? r_cmd_call (core->rcmd, r_str_trim_head (cmd)): false;
beach:
	if (scr_html != -1) {
		r_cons_flush ();
		r_config_set_i (core->config, "scr.html", scr_html);
	}
	if (scr_color != -1) {
		r_config_set_i (core->config, "scr.color", scr_color);
	}
	core->fixedblock = false;
	return rc;
fail:
	rc = -1;
	goto beach;
}