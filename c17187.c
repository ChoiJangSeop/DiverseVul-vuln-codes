tparam_internal(int use_TPARM_ARG, const char *string, va_list ap)
{
    char *p_is_s[NUM_PARM];
    TPARM_ARG param[NUM_PARM];
    int popcount = 0;
    int number;
    int num_args;
    int len;
    int level;
    int x, y;
    int i;
    const char *cp = string;
    size_t len2;
    bool termcap_hack;
    bool incremented_two;

    if (cp == NULL) {
	TR(TRACE_CALLS, ("%s: format is null", TPS(tname)));
	return NULL;
    }

    TPS(out_used) = 0;
    len2 = strlen(cp);

    /*
     * Find the highest parameter-number referred to in the format string.
     * Use this value to limit the number of arguments copied from the
     * variable-length argument list.
     */
    number = _nc_tparm_analyze(cp, p_is_s, &popcount);
    if (TPS(fmt_buff) == 0) {
	TR(TRACE_CALLS, ("%s: error in analysis", TPS(tname)));
	return NULL;
    }

    incremented_two = FALSE;

    if (number > NUM_PARM)
	number = NUM_PARM;
    if (popcount > NUM_PARM)
	popcount = NUM_PARM;
    num_args = max(popcount, number);

    for (i = 0; i < num_args; i++) {
	/*
	 * A few caps (such as plab_norm) have string-valued parms.
	 * We'll have to assume that the caller knows the difference, since
	 * a char* and an int may not be the same size on the stack.  The
	 * normal prototype for this uses 9 long's, which is consistent with
	 * our va_arg() usage.
	 */
	if (p_is_s[i] != 0) {
	    p_is_s[i] = va_arg(ap, char *);
	    param[i] = 0;
	} else if (use_TPARM_ARG) {
	    param[i] = va_arg(ap, TPARM_ARG);
	} else {
	    param[i] = (TPARM_ARG) va_arg(ap, int);
	}
    }

    /*
     * This is a termcap compatibility hack.  If there are no explicit pop
     * operations in the string, load the stack in such a way that
     * successive pops will grab successive parameters.  That will make
     * the expansion of (for example) \E[%d;%dH work correctly in termcap
     * style, which means tparam() will expand termcap strings OK.
     */
    TPS(stack_ptr) = 0;
    termcap_hack = FALSE;
    if (popcount == 0) {
	termcap_hack = TRUE;
	for (i = number - 1; i >= 0; i--) {
	    if (p_is_s[i])
		spush(p_is_s[i]);
	    else
		npush((int) param[i]);
	}
    }
#ifdef TRACE
    if (USE_TRACEF(TRACE_CALLS)) {
	for (i = 0; i < num_args; i++) {
	    if (p_is_s[i] != 0) {
		save_text(", %s", _nc_visbuf(p_is_s[i]), 0);
	    } else if ((long) param[i] > MAX_OF_TYPE(NCURSES_INT2) ||
		       (long) param[i] < 0) {
		_tracef("BUG: problem with tparm parameter #%d of %d",
			i + 1, num_args);
		break;
	    } else {
		save_number(", %d", (int) param[i], 0);
	    }
	}
	_tracef(T_CALLED("%s(%s%s)"), TPS(tname), _nc_visbuf(cp), TPS(out_buff));
	TPS(out_used) = 0;
	_nc_unlock_global(tracef);
    }
#endif /* TRACE */

    while ((cp - string) < (int) len2) {
	if (*cp != '%') {
	    save_char(UChar(*cp));
	} else {
	    TPS(tparam_base) = cp++;
	    cp = parse_format(cp, TPS(fmt_buff), &len);
	    switch (*cp) {
	    default:
		break;
	    case '%':
		save_char('%');
		break;

	    case 'd':		/* FALLTHRU */
	    case 'o':		/* FALLTHRU */
	    case 'x':		/* FALLTHRU */
	    case 'X':		/* FALLTHRU */
		save_number(TPS(fmt_buff), npop(), len);
		break;

	    case 'c':		/* FALLTHRU */
		save_char(npop());
		break;

#ifdef EXP_XTERM_1005
	    case 'u':
		{
		    unsigned char target[10];
		    unsigned source = (unsigned) npop();
		    int rc = _nc_conv_to_utf8(target, source, (unsigned)
					      sizeof(target));
		    int n;
		    for (n = 0; n < rc; ++n) {
			save_char(target[n]);
		    }
		}
		break;
#endif
	    case 'l':
		npush((int) strlen(spop()));
		break;

	    case 's':
		save_text(TPS(fmt_buff), spop(), len);
		break;

	    case 'p':
		cp++;
		i = (UChar(*cp) - '1');
		if (i >= 0 && i < NUM_PARM) {
		    if (p_is_s[i]) {
			spush(p_is_s[i]);
		    } else {
			npush((int) param[i]);
		    }
		}
		break;

	    case 'P':
		cp++;
		if (isUPPER(*cp)) {
		    i = (UChar(*cp) - 'A');
		    TPS(static_vars)[i] = npop();
		} else if (isLOWER(*cp)) {
		    i = (UChar(*cp) - 'a');
		    TPS(dynamic_var)[i] = npop();
		}
		break;

	    case 'g':
		cp++;
		if (isUPPER(*cp)) {
		    i = (UChar(*cp) - 'A');
		    npush(TPS(static_vars)[i]);
		} else if (isLOWER(*cp)) {
		    i = (UChar(*cp) - 'a');
		    npush(TPS(dynamic_var)[i]);
		}
		break;

	    case S_QUOTE:
		cp++;
		npush(UChar(*cp));
		cp++;
		break;

	    case L_BRACE:
		number = 0;
		cp++;
		while (isdigit(UChar(*cp))) {
		    number = (number * 10) + (UChar(*cp) - '0');
		    cp++;
		}
		npush(number);
		break;

	    case '+':
		npush(npop() + npop());
		break;

	    case '-':
		y = npop();
		x = npop();
		npush(x - y);
		break;

	    case '*':
		npush(npop() * npop());
		break;

	    case '/':
		y = npop();
		x = npop();
		npush(y ? (x / y) : 0);
		break;

	    case 'm':
		y = npop();
		x = npop();
		npush(y ? (x % y) : 0);
		break;

	    case 'A':
		y = npop();
		x = npop();
		npush(y && x);
		break;

	    case 'O':
		y = npop();
		x = npop();
		npush(y || x);
		break;

	    case '&':
		npush(npop() & npop());
		break;

	    case '|':
		npush(npop() | npop());
		break;

	    case '^':
		npush(npop() ^ npop());
		break;

	    case '=':
		y = npop();
		x = npop();
		npush(x == y);
		break;

	    case '<':
		y = npop();
		x = npop();
		npush(x < y);
		break;

	    case '>':
		y = npop();
		x = npop();
		npush(x > y);
		break;

	    case '!':
		npush(!npop());
		break;

	    case '~':
		npush(~npop());
		break;

	    case 'i':
		/*
		 * Increment the first two parameters -- if they are numbers
		 * rather than strings.  As a side effect, assign into the
		 * stack; if this is termcap, then the stack was populated
		 * using the termcap hack above rather than via the terminfo
		 * 'p' case.
		 */
		if (!incremented_two) {
		    incremented_two = TRUE;
		    if (p_is_s[0] == 0) {
			param[0]++;
			if (termcap_hack)
			    TPS(stack)[0].data.num = (int) param[0];
		    }
		    if (p_is_s[1] == 0) {
			param[1]++;
			if (termcap_hack)
			    TPS(stack)[1].data.num = (int) param[1];
		    }
		}
		break;

	    case '?':
		break;

	    case 't':
		x = npop();
		if (!x) {
		    /* scan forward for %e or %; at level zero */
		    cp++;
		    level = 0;
		    while (*cp) {
			if (*cp == '%') {
			    cp++;
			    if (*cp == '?')
				level++;
			    else if (*cp == ';') {
				if (level > 0)
				    level--;
				else
				    break;
			    } else if (*cp == 'e' && level == 0)
				break;
			}

			if (*cp)
			    cp++;
		    }
		}
		break;

	    case 'e':
		/* scan forward for a %; at level zero */
		cp++;
		level = 0;
		while (*cp) {
		    if (*cp == '%') {
			cp++;
			if (*cp == '?')
			    level++;
			else if (*cp == ';') {
			    if (level > 0)
				level--;
			    else
				break;
			}
		    }

		    if (*cp)
			cp++;
		}
		break;

	    case ';':
		break;

	    }			/* endswitch (*cp) */
	}			/* endelse (*cp == '%') */

	if (*cp == '\0')
	    break;

	cp++;
    }				/* endwhile (*cp) */

    get_space((size_t) 1);
    TPS(out_buff)[TPS(out_used)] = '\0';

    T((T_RETURN("%s"), _nc_visbuf(TPS(out_buff))));
    return (TPS(out_buff));
}