static char *parse_note(char *p,
			int flags)
{
	struct SYMBOL *s;
	char *q;
	int pit = 0, len, acc, nostem, chord, j, m, n;

	if (flags & ABC_F_GRACE) {	/* in a grace note sequence */
		s = abc_new(ABC_T_NOTE, NULL);
	} else {
		s = abc_new(ABC_T_NOTE, gchord);
		if (gchord)
			gchord = NULL;
	}
	s->flags |= flags;
	s->u.note.notes[0].color = -1;

	if (!lyric_started) {
		lyric_started = 1;
		s->flags |= ABC_F_LYRIC_START;
	}
	if (*p != 'X' && *p != 'Z'
	 && !(flags & ABC_F_GRACE)) {
		if (!deco_start)
			deco_start = s;
	}
	chord = 0;

	/* rest */
	switch (*p) {
	case 'X':
		s->flags |= ABC_F_INVIS;
	case 'Z':			/* multi-rest */
		s->abc_type = ABC_T_MREST;
		p++;
		len = 1;
		if (isdigit((unsigned char) *p)) {
			len = strtol(p, &q, 10);
			if (len == 0 || len > 100) {
				syntax("Bad number of measures", p);
				len = 1;
			}
			p = q;
		}
		s->u.bar.type = 0;
		s->u.bar.len = len;
		goto add_deco;
	case 'y':			/* space (BarFly) */
		s->abc_type = ABC_T_REST;
		s->flags |= ABC_F_INVIS;
		p++;
		if (isdigit((unsigned char) *p)		/* number of points */
		 || *p == '-') {			/* accept negative offset... */
			s->u.note.notes[0].shhd = strtol(p, &q, 10);
			p = q;
		} else {
			s->u.note.notes[0].shhd = 10;	// default
		}
		goto add_deco;
	case 'x':			/* invisible rest */
		s->flags |= ABC_F_INVIS;
		/* fall thru */
	case 'z':
		s->abc_type = ABC_T_REST;
		p = parse_len(p + 1, ulen, &len);
		s->u.note.notes[0].len = len;
		goto do_brhythm;
	case '[':			/* '[..]' = chord */
		chord = 1;
		p++;
		break;
	}

	q = p;

	/* get pitch, length and possible accidental */
	m = 0;
	nostem = 0;
	for (;;) {
		if (chord) {
			if (m >= MAXHD) {
				syntax("Too many notes in chord", p);
				m--;
			}
			n = 0;
			if (*p == '.') {
				n = SL_DOTTED;
				p++;
			}
			if (*p == '(') {
				p++;
				switch (*p) {
				case '\'':
					n += SL_ABOVE;
					p++;
					break;
				case ',':
					n += SL_BELOW;
					p++;
					break;
				default:
					n += SL_AUTO;
					break;
				}
				s->u.note.notes[m].sl1 = (s->u.note.notes[m].sl1 << 3)
							+ n;
			}
		}
		p = parse_deco(p, &dc, m);	/* note head decorations */
		p = parse_acc_pit(p, &pit, &acc);
		if (*p == '0') {
			nostem = 1;
			p++;
		}
		p = parse_len(p, (flags & ABC_F_GRACE) ?
					BASE_LEN / 8 :	// for grace note alone
					ulen,
				&len);
		s->u.note.notes[m].pit = pit;
		s->pits[m] = pit;
		s->u.note.notes[m].len = len;
		s->u.note.notes[m].acc = acc;
		s->u.note.notes[m].color = -1;

		if (chord) {
			for (;;) {
				if (*p == '.') {
					if (p[1] != '-')
						break;
					p++;
				}
				if (*p == '-') {
					switch (p[1]) {
					case '\'':
						s->u.note.notes[m].ti1 = SL_ABOVE;
						p++;
						break;
					case ',':
						s->u.note.notes[m].ti1 = SL_BELOW;
						p++;
						break;
					default:
						s->u.note.notes[m].ti1 = SL_AUTO;
						break;
					}
				} else if (*p == ')') {
					s->u.note.notes[m].sl2++;
				} else {
					break;
				}
				p++;
			}
		}
		if (acc >= 0)			/* if no error */
			m++;			/* normal case */

		if (!chord)
			break;
		if (*p == ']') {
			p++;
			if (*p == '0') {
				nostem = 1;
				p++;
			}
			if (*p == '/' || isdigit((unsigned char) *p)) {
				p = parse_len(p, ulen, &len);
				for (j = 0; j < m; j++) {
					s->u.note.notes[j].len =
						len * s->u.note.notes[j].len / ulen;
				}
			}
			break;
		}
		if (*p == '\0') {
			syntax("Chord not closed", q);
			break;
		}
	}
	if (nostem)
		s->flags |= ABC_F_STEMLESS;

	if (m == 0)			/* if no note (or error) */
		goto err;

	s->u.note.microscale = microscale;
	s->nhd = m - 1;

do_brhythm:
	if (curvoice->last_note
	 && curvoice->last_note->u.note.brhythm != 0)
		broken_rhythm(s, -curvoice->last_note->u.note.brhythm);
add_deco:
	if (dc.n > 0) {
		memcpy(s->abc_type != ABC_T_MREST ? &s->u.note.dc
				: &s->u.bar.dc,
			&dc, sizeof dc);
		dc.n = 0;
	}

	/* forbid rests in grace note sequences */
	if (s->abc_type != ABC_T_NOTE && (flags & ABC_F_GRACE)) {
		syntax("Not a note in grace note sequence", p);
		goto err;
	}
	return p;

err:
	if ((parse.last_sym = s->abc_prev) == NULL) {
		parse.first_sym = NULL;
	} else {
		s->abc_prev->abc_next = NULL;
		s->abc_prev->flags |= (s->flags & ABC_F_ERROR);
	}
	return p;
}