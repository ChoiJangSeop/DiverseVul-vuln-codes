_nc_tparm_analyze(const char *string, char *p_is_s[NUM_PARM], int *popcount)
{
    size_t len2;
    int i;
    int lastpop = -1;
    int len;
    int number = 0;
    int level = -1;
    const char *cp = string;
    static char dummy[] = "";

    if (cp == 0)
	return 0;

    if ((len2 = strlen(cp)) + 2 > TPS(fmt_size)) {
	TPS(fmt_size) += len2 + 2;
	TPS(fmt_buff) = typeRealloc(char, TPS(fmt_size), TPS(fmt_buff));
	if (TPS(fmt_buff) == 0)
	    return 0;
    }

    memset(p_is_s, 0, sizeof(p_is_s[0]) * NUM_PARM);
    *popcount = 0;

    while ((cp - string) < (int) len2) {
	if (*cp == '%') {
	    cp++;
	    cp = parse_format(cp, TPS(fmt_buff), &len);
	    switch (*cp) {
	    default:
		break;

	    case 'd':		/* FALLTHRU */
	    case 'o':		/* FALLTHRU */
	    case 'x':		/* FALLTHRU */
	    case 'X':		/* FALLTHRU */
	    case 'c':		/* FALLTHRU */
#ifdef EXP_XTERM_1005
	    case 'u':
#endif
		if (lastpop <= 0) {
		    tc_BUMP();
		}
		level -= 1;
		lastpop = -1;
		break;

	    case 'l':
	    case 's':
		if (lastpop > 0) {
		    level -= 1;
		    p_is_s[lastpop - 1] = dummy;
		}
		tc_BUMP();
		break;

	    case 'p':
		cp++;
		i = (UChar(*cp) - '0');
		if (i >= 0 && i <= NUM_PARM) {
		    ++level;
		    lastpop = i;
		    if (lastpop > *popcount)
			*popcount = lastpop;
		}
		break;

	    case 'P':
		++cp;
		break;

	    case 'g':
		++level;
		cp++;
		break;

	    case S_QUOTE:
		++level;
		cp += 2;
		lastpop = -1;
		break;

	    case L_BRACE:
		++level;
		cp++;
		while (isdigit(UChar(*cp))) {
		    cp++;
		}
		break;

	    case '+':
	    case '-':
	    case '*':
	    case '/':
	    case 'm':
	    case 'A':
	    case 'O':
	    case '&':
	    case '|':
	    case '^':
	    case '=':
	    case '<':
	    case '>':
		tc_BUMP();
		level -= 1;	/* pop 2, operate, push 1 */
		lastpop = -1;
		break;

	    case '!':
	    case '~':
		tc_BUMP();
		lastpop = -1;
		break;

	    case 'i':
		/* will add 1 to first (usually two) parameters */
		break;
	    }
	}
	if (*cp != '\0')
	    cp++;
    }

    if (number > NUM_PARM)
	number = NUM_PARM;
    return number;
}