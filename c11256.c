static int parse_line(char *p)
{
	struct SYMBOL *s;
	char *q, c;
	char *dot = NULL;
	struct SYMBOL *last_note_sav = NULL;
	struct decos dc_sav;
	int i, flags, flags_sav = 0, slur;
	static char qtb[10] = {0, 1, 3, 2, 3, 0, 2, 0, 3, 0};

	colnum = 0;
	switch (*p) {
	case '\0':			/* blank line */
		switch (parse.abc_state) {
		case ABC_S_GLOBAL:
			if (parse.last_sym
			 && parse.last_sym->abc_type != ABC_T_NULL)
				abc_new(ABC_T_NULL, NULL);
		case ABC_S_HEAD:	/*fixme: may have blank lines in headers?*/
			return 0;
		}
		return 1;
	case '%':
		if (p[1] == '%') {
			s = abc_new(ABC_T_PSCOM, p);
			p += 2;				/* skip '%%' */
			if (strncasecmp(p, "decoration ", 11) == 0) {
				p += 11;
				while (isspace((unsigned char) *p))
					p++;
				switch (*p) {
				case '!':
					char_tb['!'] = CHAR_DECOS;
					char_tb['+'] = CHAR_BAD;
					break;
				case '+':
					char_tb['+'] = CHAR_DECOS;
					char_tb['!'] = CHAR_BAD;
					break;
				}
				return 0;
			}
			if (strncasecmp(p, "linebreak ", 10) == 0) {
				for (i = 0; i < sizeof char_tb; i++) {
					if (char_tb[i] == CHAR_LINEBREAK)
						char_tb[i] = i != '!' ?
								CHAR_BAD :
								CHAR_DECOS;
				}
				p += 10;
				for (;;) {
					while (isspace((unsigned char) *p))
						p++;
					if (*p == '\0')
						break;
					switch (*p) {
					case '!':
					case '$':
					case '*':
					case ';':
					case '?':
					case '@':
						char_tb[(unsigned char) *p++]
								= CHAR_LINEBREAK;
						break;
					case '<':
						if (strncmp(p, "<none>", 6) == 0)
							return 0;
						if (strncmp(p, "<EOL>", 5) == 0) {
							char_tb['\n'] = CHAR_LINEBREAK;
							p += 5;
							break;
						}
						/* fall thru */
					default:
						if (strcmp(p, "lock") != 0)
							syntax("Invalid character in %%%%linebreak",
								p);
						return 0;
					}
				}
				return 0;
			}
			if (strncasecmp(p, "microscale ", 11) == 0) {
				int v;

				p += 11;
				while (isspace((unsigned char) *p))
					p++;
				sscanf(p, "%d", &v);
				if (v < 4 || v >= 256 || v & 1)
					syntax("Invalid value in %%microscale", p);
				else
					microscale = v;
				return 0;
			}
			if (strncasecmp(p, "user ", 5) == 0) {
				p += 5;
				while (isspace((unsigned char) *p))
					p++;
				get_user(p, s);
				return 0;
			}
			return 0;
		}
		/* fall thru */
	case '\\':				/* abc2mtex specific lines */
		return 0;			/* skip */
	}

	/* header fields */
	if (p[1] == ':'
	 && *p != '|' && *p != ':') {		/* not '|:' nor '::' */
		int new_tune;

		new_tune = parse_info(p);

		/* handle BarFly voice definition */
		/* 'V:n <note line ending with a bar>' */
		if (*p != 'V'
		 || parse.abc_state != ABC_S_TUNE)
			return new_tune;		/* (normal return) */
		c = p[strlen(p) - 1];
		if (c != '|' && c != ']')
			return new_tune;
		while (!isspace((unsigned char) *p) && *p != '\0')
			p++;
		while (isspace((unsigned char) *p))
			p++;
	}
	if (parse.abc_state != ABC_S_TUNE)
		return 0;

	/* music */
	flags = 0;
	if (parse.abc_vers <= (2 << 16))
		lyric_started = 0;
	deco_start = deco_cont = NULL;
	slur = 0;
	while (*p != '\0') {
		colnum = p - abc_line;
		switch (char_tb[(unsigned char) *p++]) {
		case CHAR_GCHORD:			/* " */
			if (flags & ABC_F_GRACE)
				goto bad_char;
			p = parse_gchord(p);
			break;
		case CHAR_GR_ST:		/* '{' */
			if (flags & ABC_F_GRACE)
				goto bad_char;
			last_note_sav = curvoice->last_note;
			curvoice->last_note = NULL;
			memcpy(&dc_sav, &dc, sizeof dc);
			dc.n = 0;
			flags_sav = flags;
			flags = ABC_F_GRACE;
			if (*p == '/') {
				flags |= ABC_F_SAPPO;
				p++;
			}
			break;
		case CHAR_GR_EN:		/* '}' */
			if (!(flags & ABC_F_GRACE))
				goto bad_char;
			parse.last_sym->flags |= ABC_F_GR_END;
			if (dc.n != 0)
				syntax("Decoration ignored", p);
			curvoice->last_note = last_note_sav;
			memcpy(&dc, &dc_sav, sizeof dc);
			flags = flags_sav;
			break;
		case CHAR_DECOS:
			if (p[-1] == '!'
			 && char_tb['\n'] == CHAR_LINEBREAK
			 && check_nl(p)) {
				s = abc_new(ABC_T_EOLN, NULL);	/* abc2win EOL */
				s->u.eoln.type = 2;
				break;
			}
			/* fall thru */
		case CHAR_DECO:
			if (p[-1] == '.') {
				if (*p == '(' || *p == '-') {
					dot = p;
					break;
				}
//				if (*p == '|') {
//					p = parse_bar(p + 1);
//					parse.last_sym->u.bar.dotted = 1;
//					break;
//				}
			}
			p = parse_deco(p - 1, &dc, -1);
			break;
		case CHAR_LINEBREAK:
			s = abc_new(ABC_T_EOLN, NULL);
//			s->u.eoln.type = 0;
			break;
		case CHAR_NOTE:
			p = parse_note(p - 1, flags);
			flags &= ABC_F_GRACE;
			parse.last_sym->u.note.slur_st = slur;
			slur = 0;
			if (parse.last_sym->u.note.notes[0].len > 0) /* if not space */
				curvoice->last_note = parse.last_sym;
			break;
		case CHAR_SLASH:		/* '/' */
			if (flags & ABC_F_GRACE)
				goto bad_char;
			if (char_tb[(unsigned char) p[-1]] != CHAR_BAR)
				goto bad_char;
			q = p;
			while (*q == '/')
				q++;
			if (char_tb[(unsigned char) *q] != CHAR_BAR)
				goto bad_char;
			s = abc_new(ABC_T_MREP, NULL);
			s->u.bar.type = 0;
			s->u.bar.len = q - p + 1;
			syntax("Non standard measure repeat syntax", p - 1);
			p = q;
			break;
		case CHAR_BSLASH:		/* '\\' */
			if (*p == '\0')
				break;
			syntax("'\\' ignored", p - 1);
			break;
		case CHAR_OBRA:			/* '[' */
			if (*p == '|' || *p == ']' || *p == ':'
			 || isdigit((unsigned char) *p) || *p == '"'
			 || *p == ' ') {
				if (flags & ABC_F_GRACE)
					goto bad_char;
				p = parse_bar(p);
				break;
			}
			if (p[1] != ':') {
				p = parse_note(p - 1, flags); /* chord */
				flags &= ABC_F_GRACE;
				parse.last_sym->u.note.slur_st = slur;
				slur = 0;
				curvoice->last_note = parse.last_sym;
				break;
			}

			/* embedded information field */
#if 0
/*fixme:OK for [I:staff n], ?? for other headers*/
			if (flags & ABC_F_GRACE)
				goto bad_char;
#endif
			while (p[2] == ' ') {		/* remove the spaces */
				p[2] = ':';
				p[1] = *p;
				p++;
			}
			c = ']';
			q = p;
			while (*p != '\0' && *p != c)
				p++;
			if (*p == '\0') {
				syntax("Escape sequence [..] not closed", q);
				c = '\0';
			} else {
				*p = '\0';
			}
			parse_info(q);
			*p = c;
			if (c != '\0')
				p++;
			break;
		case CHAR_BAR:			/* '|', ':' or ']' */
			if (flags & ABC_F_GRACE)
				goto bad_char;
			p = parse_bar(p);
			break;
		case CHAR_OPAR:			/* '(' */
			if (*p > '0' && *p <= '9') {
				int pplet, qplet, rplet;

				pplet = strtol(p, &q, 10);
				p = q;
				if ((unsigned) pplet < sizeof qtb / sizeof qtb[0])
					qplet = qtb[pplet];
				else
					qplet = qtb[0];
				rplet = pplet;
				if (*p == ':') {
					p++;
					if (isdigit((unsigned char) *p)) {
						qplet = strtol(p, &q, 10);
						p = q;
					}
					if (*p == ':') {
						p++;
						if (isdigit((unsigned char) *p)) {
							rplet = strtol(p, &q, 10);
							p = q;
						}
					}
				}
				if (rplet < 1) {
					syntax("Invalid 'r' in tuplet", p);
					break;
				}
				if (pplet >= 128 || qplet >= 128 || rplet >= 128) {
					syntax("Invalid 'p:q:r' in tuplet", p);
					break;
				}
				if (qplet == 0)
					qplet = meter % 3 == 0 ? 3 : 2;
				s = abc_new(ABC_T_TUPLET, NULL);
				s->u.tuplet.p_plet = pplet;
				s->u.tuplet.q_plet = qplet;
				s->u.tuplet.r_plet = rplet;
				s->flags |= flags;
				break;
			}
			if (*p == '&') {
				if (flags & ABC_F_GRACE)
					goto bad_char;
				p++;
				if (vover != 0) {
					syntax("Nested voice overlay", p - 1);
					break;
				}
				s = abc_new(ABC_T_V_OVER, NULL);
				s->u.v_over.type = V_OVER_S;
				s->u.v_over.voice = curvoice - voice_tb;
				vover = -1;		/* multi-bars */
				break;
			}
			slur <<= 4;
			if (p == dot + 1 && dc.n == 0)
				slur |= SL_DOTTED;
			switch (*p) {
			case '\'':
				slur += SL_ABOVE;
				p++;
				break;
			case ',':
				slur += SL_BELOW;
				p++;
				break;
			default:
				slur += SL_AUTO;
				break;
			}
			break;
		case CHAR_CPAR:			/* ')' */
			switch (parse.last_sym->abc_type) {
			case ABC_T_NOTE:
			case ABC_T_REST:
				break;
			default:
				goto bad_char;
			}
			parse.last_sym->u.note.slur_end++;
			break;
		case CHAR_VOV:			/* '&' */
			if (flags & ABC_F_GRACE)
				goto bad_char;
			if (*p != ')'
			 || vover == 0) {		/*??*/
				if (!curvoice->last_note) {
					syntax("Bad start of voice overlay", p);
					break;
				}
				s = abc_new(ABC_T_V_OVER, NULL);
				/*s->u.v_over.type = V_OVER_V; */
				vover_new();
				s->u.v_over.voice = curvoice - voice_tb;
				if (vover == 0)
					vover = 1;	/* single bar */
				break;
			}
			p++;
			vover = 0;
			s = abc_new(ABC_T_V_OVER, NULL);
			s->u.v_over.type = V_OVER_E;
			s->u.v_over.voice = curvoice->mvoice;
			curvoice->last_note = NULL;	/* ?? */
			curvoice = &voice_tb[curvoice->mvoice];
			break;
		case CHAR_SPAC:			/* ' ' and '\t' */
			flags |= ABC_F_SPACE;
			break;
		case CHAR_MINUS: {		/* '-' */
			int tie_pos;

			if (!curvoice->last_note
			 || curvoice->last_note->abc_type != ABC_T_NOTE)
				goto bad_char;
			if (p == dot + 1 && dc.n == 0)
				tie_pos = SL_DOTTED;
			else
				tie_pos = 0;
			switch (*p) {
			case '\'':
				tie_pos += SL_ABOVE;
				p++;
				break;
			case ',':
				tie_pos += SL_BELOW;
				p++;
				break;
			default:
				tie_pos += SL_AUTO;
				break;
			}
			for (i = 0; i <= curvoice->last_note->nhd; i++) {
				if (curvoice->last_note->u.note.notes[i].ti1 == 0)
					curvoice->last_note->u.note.notes[i].ti1 = tie_pos;
				else if (curvoice->last_note->nhd == 0)
					syntax("Too many ties", p);
			}
			break;
		    }
		case CHAR_BRHY:			/* '>' and '<' */
			if (!curvoice->last_note)
				goto bad_char;
			i = 1;
			while (*p == p[-1]) {
				i++;
				p++;
			}
			if (i > 3) {
				syntax("Bad broken rhythm", p - 1);
				i = 3;
			}
			if (p[-1] == '<')
				i = -i;
			broken_rhythm(curvoice->last_note, i);
			curvoice->last_note->u.note.brhythm = i;
			break;
		case CHAR_IGN:			/* '*' & '`' */
			break;
		default:
		bad_char:
			syntax((flags & ABC_F_GRACE)
					? "Bad character in grace note sequence"
					: "Bad character",
				p - 1);
			break;
		}
	}

/*fixme: may we have grace notes across lines?*/
	if (flags & ABC_F_GRACE) {
		syntax("EOLN in grace note sequence", p - 1);
		if (curvoice->last_note)
			curvoice->last_note->flags |= ABC_F_GR_END;
		curvoice->last_note = last_note_sav;
		memcpy(&dc, &dc_sav, sizeof dc);
	}

	/* add eoln */
	s = abc_new(ABC_T_EOLN, NULL);
	if (flags & ABC_F_SPACE)
		s->flags |= ABC_F_SPACE;
	if (p[-1] == '\\'
	 || char_tb['\n'] != CHAR_LINEBREAK)
		s->u.eoln.type = 1;		/* no break */

	return 0;
}