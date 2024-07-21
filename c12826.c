static int match(Reinst *pc, const char *sp, const char *bol, int flags, Resub *out)
{
	Resub scratch;
	int i;
	Rune c;

	for (;;) {
		switch (pc->opcode) {
		case I_END:
			return 1;
		case I_JUMP:
			pc = pc->x;
			break;
		case I_SPLIT:
			scratch = *out;
			if (match(pc->x, sp, bol, flags, &scratch)) {
				*out = scratch;
				return 1;
			}
			pc = pc->y;
			break;

		case I_PLA:
			if (!match(pc->x, sp, bol, flags, out))
				return 0;
			pc = pc->y;
			break;
		case I_NLA:
			scratch = *out;
			if (match(pc->x, sp, bol, flags, &scratch))
				return 0;
			pc = pc->y;
			break;

		case I_ANYNL:
			sp += chartorune(&c, sp);
			if (c == 0)
				return 0;
			pc = pc + 1;
			break;
		case I_ANY:
			sp += chartorune(&c, sp);
			if (c == 0)
				return 0;
			if (isnewline(c))
				return 0;
			pc = pc + 1;
			break;
		case I_CHAR:
			sp += chartorune(&c, sp);
			if (c == 0)
				return 0;
			if (flags & REG_ICASE)
				c = canon(c);
			if (c != pc->c)
				return 0;
			pc = pc + 1;
			break;
		case I_CCLASS:
			sp += chartorune(&c, sp);
			if (c == 0)
				return 0;
			if (flags & REG_ICASE) {
				if (!incclasscanon(pc->cc, canon(c)))
					return 0;
			} else {
				if (!incclass(pc->cc, c))
					return 0;
			}
			pc = pc + 1;
			break;
		case I_NCCLASS:
			sp += chartorune(&c, sp);
			if (c == 0)
				return 0;
			if (flags & REG_ICASE) {
				if (incclasscanon(pc->cc, canon(c)))
					return 0;
			} else {
				if (incclass(pc->cc, c))
					return 0;
			}
			pc = pc + 1;
			break;
		case I_REF:
			i = out->sub[pc->n].ep - out->sub[pc->n].sp;
			if (flags & REG_ICASE) {
				if (strncmpcanon(sp, out->sub[pc->n].sp, i))
					return 0;
			} else {
				if (strncmp(sp, out->sub[pc->n].sp, i))
					return 0;
			}
			if (i > 0)
				sp += i;
			pc = pc + 1;
			break;

		case I_BOL:
			if (sp == bol && !(flags & REG_NOTBOL)) {
				pc = pc + 1;
				break;
			}
			if (flags & REG_NEWLINE) {
				if (sp > bol && isnewline(sp[-1])) {
					pc = pc + 1;
					break;
				}
			}
			return 0;
		case I_EOL:
			if (*sp == 0) {
				pc = pc + 1;
				break;
			}
			if (flags & REG_NEWLINE) {
				if (isnewline(*sp)) {
					pc = pc + 1;
					break;
				}
			}
			return 0;
		case I_WORD:
			i = sp > bol && iswordchar(sp[-1]);
			i ^= iswordchar(sp[0]);
			if (!i)
				return 0;
			pc = pc + 1;
			break;
		case I_NWORD:
			i = sp > bol && iswordchar(sp[-1]);
			i ^= iswordchar(sp[0]);
			if (i)
				return 0;
			pc = pc + 1;
			break;

		case I_LPAR:
			out->sub[pc->n].sp = sp;
			pc = pc + 1;
			break;
		case I_RPAR:
			out->sub[pc->n].ep = sp;
			pc = pc + 1;
			break;
		default:
			return 0;
		}
	}
}