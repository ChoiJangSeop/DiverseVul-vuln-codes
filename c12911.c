static filter_rule *parse_rule_tok(const char **rulestr_ptr,
				   const filter_rule *template, int xflags,
				   const char **pat_ptr, unsigned int *pat_len_ptr)
{
	const uchar *s = (const uchar *)*rulestr_ptr;
	filter_rule *rule;
	unsigned int len;

	if (template->rflags & FILTRULE_WORD_SPLIT) {
		/* Skip over any initial whitespace. */
		while (isspace(*s))
			s++;
		/* Update to point to real start of rule. */
		*rulestr_ptr = (const char *)s;
	}
	if (!*s)
		return NULL;

	rule = new0(filter_rule);

	/* Inherit from the template.  Don't inherit FILTRULES_SIDES; we check
	 * that later. */
	rule->rflags = template->rflags & FILTRULES_FROM_CONTAINER;

	/* Figure out what kind of a filter rule "s" is pointing at.  Note
	 * that if FILTRULE_NO_PREFIXES is set, the rule is either an include
	 * or an exclude based on the inheritance of the FILTRULE_INCLUDE
	 * flag (above).  XFLG_OLD_PREFIXES indicates a compatibility mode
	 * for old include/exclude patterns where just "+ " and "- " are
	 * allowed as optional prefixes.  */
	if (template->rflags & FILTRULE_NO_PREFIXES) {
		if (*s == '!' && template->rflags & FILTRULE_CVS_IGNORE)
			rule->rflags |= FILTRULE_CLEAR_LIST; /* Tentative! */
	} else if (xflags & XFLG_OLD_PREFIXES) {
		if (*s == '-' && s[1] == ' ') {
			rule->rflags &= ~FILTRULE_INCLUDE;
			s += 2;
		} else if (*s == '+' && s[1] == ' ') {
			rule->rflags |= FILTRULE_INCLUDE;
			s += 2;
		} else if (*s == '!')
			rule->rflags |= FILTRULE_CLEAR_LIST; /* Tentative! */
	} else {
		char ch = 0;
		BOOL prefix_specifies_side = False;
		switch (*s) {
		case 'c':
			if ((s = RULE_STRCMP(s, "clear")) != NULL)
				ch = '!';
			break;
		case 'd':
			if ((s = RULE_STRCMP(s, "dir-merge")) != NULL)
				ch = ':';
			break;
		case 'e':
			if ((s = RULE_STRCMP(s, "exclude")) != NULL)
				ch = '-';
			break;
		case 'h':
			if ((s = RULE_STRCMP(s, "hide")) != NULL)
				ch = 'H';
			break;
		case 'i':
			if ((s = RULE_STRCMP(s, "include")) != NULL)
				ch = '+';
			break;
		case 'm':
			if ((s = RULE_STRCMP(s, "merge")) != NULL)
				ch = '.';
			break;
		case 'p':
			if ((s = RULE_STRCMP(s, "protect")) != NULL)
				ch = 'P';
			break;
		case 'r':
			if ((s = RULE_STRCMP(s, "risk")) != NULL)
				ch = 'R';
			break;
		case 's':
			if ((s = RULE_STRCMP(s, "show")) != NULL)
				ch = 'S';
			break;
		default:
			ch = *s;
			if (s[1] == ',')
				s++;
			break;
		}
		switch (ch) {
		case ':':
			rule->rflags |= FILTRULE_PERDIR_MERGE
				      | FILTRULE_FINISH_SETUP;
			/* FALL THROUGH */
		case '.':
			rule->rflags |= FILTRULE_MERGE_FILE;
			break;
		case '+':
			rule->rflags |= FILTRULE_INCLUDE;
			break;
		case '-':
			break;
		case 'S':
			rule->rflags |= FILTRULE_INCLUDE;
			/* FALL THROUGH */
		case 'H':
			rule->rflags |= FILTRULE_SENDER_SIDE;
			prefix_specifies_side = True;
			break;
		case 'R':
			rule->rflags |= FILTRULE_INCLUDE;
			/* FALL THROUGH */
		case 'P':
			rule->rflags |= FILTRULE_RECEIVER_SIDE;
			prefix_specifies_side = True;
			break;
		case '!':
			rule->rflags |= FILTRULE_CLEAR_LIST;
			break;
		default:
			rprintf(FERROR, "Unknown filter rule: `%s'\n", *rulestr_ptr);
			exit_cleanup(RERR_SYNTAX);
		}
		while (ch != '!' && *++s && *s != ' ' && *s != '_') {
			if (template->rflags & FILTRULE_WORD_SPLIT && isspace(*s)) {
				s--;
				break;
			}
			switch (*s) {
			default:
			    invalid:
				rprintf(FERROR,
					"invalid modifier '%c' at position %d in filter rule: %s\n",
					*s, (int)(s - (const uchar *)*rulestr_ptr), *rulestr_ptr);
				exit_cleanup(RERR_SYNTAX);
			case '-':
				if (!BITS_SETnUNSET(rule->rflags, FILTRULE_MERGE_FILE, FILTRULE_NO_PREFIXES))
					goto invalid;
				rule->rflags |= FILTRULE_NO_PREFIXES;
				break;
			case '+':
				if (!BITS_SETnUNSET(rule->rflags, FILTRULE_MERGE_FILE, FILTRULE_NO_PREFIXES))
					goto invalid;
				rule->rflags |= FILTRULE_NO_PREFIXES
					      | FILTRULE_INCLUDE;
				break;
			case '/':
				rule->rflags |= FILTRULE_ABS_PATH;
				break;
			case '!':
				/* Negation really goes with the pattern, so it
				 * isn't useful as a merge-file default. */
				if (rule->rflags & FILTRULE_MERGE_FILE)
					goto invalid;
				rule->rflags |= FILTRULE_NEGATE;
				break;
			case 'C':
				if (rule->rflags & FILTRULE_NO_PREFIXES || prefix_specifies_side)
					goto invalid;
				rule->rflags |= FILTRULE_NO_PREFIXES
					      | FILTRULE_WORD_SPLIT
					      | FILTRULE_NO_INHERIT
					      | FILTRULE_CVS_IGNORE;
				break;
			case 'e':
				if (!(rule->rflags & FILTRULE_MERGE_FILE))
					goto invalid;
				rule->rflags |= FILTRULE_EXCLUDE_SELF;
				break;
			case 'n':
				if (!(rule->rflags & FILTRULE_MERGE_FILE))
					goto invalid;
				rule->rflags |= FILTRULE_NO_INHERIT;
				break;
			case 'p':
				rule->rflags |= FILTRULE_PERISHABLE;
				break;
			case 'r':
				if (prefix_specifies_side)
					goto invalid;
				rule->rflags |= FILTRULE_RECEIVER_SIDE;
				break;
			case 's':
				if (prefix_specifies_side)
					goto invalid;
				rule->rflags |= FILTRULE_SENDER_SIDE;
				break;
			case 'w':
				if (!(rule->rflags & FILTRULE_MERGE_FILE))
					goto invalid;
				rule->rflags |= FILTRULE_WORD_SPLIT;
				break;
			case 'x':
				rule->rflags |= FILTRULE_XATTR;
				saw_xattr_filter = 1;
				break;
			}
		}
		if (*s)
			s++;
	}
	if (template->rflags & FILTRULES_SIDES) {
		if (rule->rflags & FILTRULES_SIDES) {
			/* The filter and template both specify side(s).  This
			 * is dodgy (and won't work correctly if the template is
			 * a one-sided per-dir merge rule), so reject it. */
			rprintf(FERROR,
				"specified-side merge file contains specified-side filter: %s\n",
				*rulestr_ptr);
			exit_cleanup(RERR_SYNTAX);
		}
		rule->rflags |= template->rflags & FILTRULES_SIDES;
	}

	if (template->rflags & FILTRULE_WORD_SPLIT) {
		const uchar *cp = s;
		/* Token ends at whitespace or the end of the string. */
		while (!isspace(*cp) && *cp != '\0')
			cp++;
		len = cp - s;
	} else
		len = strlen((char*)s);

	if (rule->rflags & FILTRULE_CLEAR_LIST) {
		if (!(rule->rflags & FILTRULE_NO_PREFIXES)
		 && !(xflags & XFLG_OLD_PREFIXES) && len) {
			rprintf(FERROR,
				"'!' rule has trailing characters: %s\n", *rulestr_ptr);
			exit_cleanup(RERR_SYNTAX);
		}
		if (len > 1)
			rule->rflags &= ~FILTRULE_CLEAR_LIST;
	} else if (!len && !(rule->rflags & FILTRULE_CVS_IGNORE)) {
		rprintf(FERROR, "unexpected end of filter rule: %s\n", *rulestr_ptr);
		exit_cleanup(RERR_SYNTAX);
	}

	/* --delete-excluded turns an un-modified include/exclude into a sender-side rule.  */
	if (delete_excluded
	 && !(rule->rflags & (FILTRULES_SIDES|FILTRULE_MERGE_FILE|FILTRULE_PERDIR_MERGE)))
		rule->rflags |= FILTRULE_SENDER_SIDE;

	*pat_ptr = (const char *)s;
	*pat_len_ptr = len;
	*rulestr_ptr = *pat_ptr + len;
	return rule;
}