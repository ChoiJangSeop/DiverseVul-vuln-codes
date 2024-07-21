parse_enclose(Node** np, OnigToken* tok, int term, UChar** src, UChar* end,
	      ScanEnv* env)
{
  int r = 0, num;
  Node *target, *work1 = NULL, *work2 = NULL;
  OnigOptionType option;
  OnigCodePoint c;
  OnigEncoding enc = env->enc;

#ifdef USE_NAMED_GROUP
  int list_capture;
#endif

  UChar* p = *src;
  PFETCH_READY;

  *np = NULL;
  if (PEND) return ONIGERR_END_PATTERN_WITH_UNMATCHED_PARENTHESIS;

  option = env->option;
  if (PPEEK_IS('?') &&
      IS_SYNTAX_OP2(env->syntax, ONIG_SYN_OP2_QMARK_GROUP_EFFECT)) {
    PINC;
    if (PEND) return ONIGERR_END_PATTERN_IN_GROUP;

    PFETCH(c);
    switch (c) {
    case ':':   /* (?:...) grouping only */
    group:
      r = fetch_token(tok, &p, end, env);
      if (r < 0) return r;
      r = parse_subexp(np, tok, term, &p, end, env);
      if (r < 0) return r;
      *src = p;
      return 1; /* group */
      break;

    case '=':
      *np = onig_node_new_anchor(ANCHOR_PREC_READ);
      break;
    case '!':   /* preceding read */
      *np = onig_node_new_anchor(ANCHOR_PREC_READ_NOT);
      break;
    case '>':   /* (?>...) stop backtrack */
      *np = node_new_enclose(ENCLOSE_STOP_BACKTRACK);
      break;

#ifdef USE_NAMED_GROUP
    case '\'':
      if (IS_SYNTAX_OP2(env->syntax, ONIG_SYN_OP2_QMARK_LT_NAMED_GROUP)) {
	goto named_group1;
      }
      else
	return ONIGERR_UNDEFINED_GROUP_OPTION;
      break;

# ifdef USE_CAPITAL_P_NAMED_GROUP
    case 'P':   /* (?P<name>...) */
      if (IS_SYNTAX_OP2(env->syntax, ONIG_SYN_OP2_QMARK_CAPITAL_P_NAMED_GROUP)) {
	PFETCH(c);
	if (c == '<') goto named_group1;
      }
      return ONIGERR_UNDEFINED_GROUP_OPTION;
      break;
# endif
#endif

    case '<':   /* look behind (?<=...), (?<!...) */
      PFETCH(c);
      if (c == '=')
	*np = onig_node_new_anchor(ANCHOR_LOOK_BEHIND);
      else if (c == '!')
	*np = onig_node_new_anchor(ANCHOR_LOOK_BEHIND_NOT);
#ifdef USE_NAMED_GROUP
      else {    /* (?<name>...) */
	if (IS_SYNTAX_OP2(env->syntax, ONIG_SYN_OP2_QMARK_LT_NAMED_GROUP)) {
	  UChar *name;
	  UChar *name_end;

	  PUNFETCH;
	  c = '<';

	named_group1:
	  list_capture = 0;

	named_group2:
	  name = p;
	  r = fetch_name((OnigCodePoint )c, &p, end, &name_end, env, &num, 0);
	  if (r < 0) return r;

	  num = scan_env_add_mem_entry(env);
	  if (num < 0) return num;
	  if (list_capture != 0 && num >= (int )BIT_STATUS_BITS_NUM)
	    return ONIGERR_GROUP_NUMBER_OVER_FOR_CAPTURE_HISTORY;

	  r = name_add(env->reg, name, name_end, num, env);
	  if (r != 0) return r;
	  *np = node_new_enclose_memory(env->option, 1);
	  CHECK_NULL_RETURN_MEMERR(*np);
	  NENCLOSE(*np)->regnum = num;
	  if (list_capture != 0)
	    BIT_STATUS_ON_AT_SIMPLE(env->capture_history, num);
	  env->num_named++;
	}
	else {
	  return ONIGERR_UNDEFINED_GROUP_OPTION;
	}
      }
#else
      else {
	return ONIGERR_UNDEFINED_GROUP_OPTION;
      }
#endif
      break;

    case '@':
      if (IS_SYNTAX_OP2(env->syntax, ONIG_SYN_OP2_ATMARK_CAPTURE_HISTORY)) {
#ifdef USE_NAMED_GROUP
	if (IS_SYNTAX_OP2(env->syntax, ONIG_SYN_OP2_QMARK_LT_NAMED_GROUP)) {
	  PFETCH(c);
	  if (c == '<' || c == '\'') {
	    list_capture = 1;
	    goto named_group2; /* (?@<name>...) */
	  }
	  PUNFETCH;
	}
#endif
	*np = node_new_enclose_memory(env->option, 0);
	CHECK_NULL_RETURN_MEMERR(*np);
	num = scan_env_add_mem_entry(env);
	if (num < 0) return num;
	if (num >= (int )BIT_STATUS_BITS_NUM)
	  return ONIGERR_GROUP_NUMBER_OVER_FOR_CAPTURE_HISTORY;

	NENCLOSE(*np)->regnum = num;
	BIT_STATUS_ON_AT_SIMPLE(env->capture_history, num);
      }
      else {
	return ONIGERR_UNDEFINED_GROUP_OPTION;
      }
      break;

    case '(':   /* conditional expression: (?(cond)yes), (?(cond)yes|no) */
      if (IS_SYNTAX_OP2(env->syntax, ONIG_SYN_OP2_QMARK_LPAREN_CONDITION)) {
	UChar *name = NULL;
	UChar *name_end;
	PFETCH(c);
	if (ONIGENC_IS_CODE_DIGIT(enc, c)) {     /* (n) */
	  PUNFETCH;
	  r = fetch_name((OnigCodePoint )'(', &p, end, &name_end, env, &num, 1);
	  if (r < 0) return r;
#if 0
	  /* Relative number is not currently supported. (same as Perl) */
	  if (num < 0) {
	    num = BACKREF_REL_TO_ABS(num, env);
	    if (num <= 0)
	      return ONIGERR_INVALID_BACKREF;
	  }
#endif
	  if (IS_SYNTAX_BV(env->syntax, ONIG_SYN_STRICT_CHECK_BACKREF)) {
	    if (num > env->num_mem ||
		IS_NULL(SCANENV_MEM_NODES(env)[num]))
	    return ONIGERR_INVALID_BACKREF;
	  }
	}
#ifdef USE_NAMED_GROUP
	else if (c == '<' || c == '\'') {    /* (<name>), ('name') */
	  int nums;
	  int *backs;

	  name = p;
	  r = fetch_name((OnigCodePoint )c, &p, end, &name_end, env, &num, 0);
	  if (r < 0) return r;
	  PFETCH(c);
	  if (c != ')') return ONIGERR_UNDEFINED_GROUP_OPTION;

	  nums = onig_name_to_group_numbers(env->reg, name, name_end, &backs);
	  if (nums <= 0) {
	    onig_scan_env_set_error_string(env,
		     ONIGERR_UNDEFINED_NAME_REFERENCE, name, name_end);
	    return ONIGERR_UNDEFINED_NAME_REFERENCE;
	  }
	  if (IS_SYNTAX_BV(env->syntax, ONIG_SYN_STRICT_CHECK_BACKREF)) {
	    int i;
	    for (i = 0; i < nums; i++) {
	      if (backs[i] > env->num_mem ||
		  IS_NULL(SCANENV_MEM_NODES(env)[backs[i]]))
	      return ONIGERR_INVALID_BACKREF;
	    }
	  }
	  num = backs[0];       /* XXX: use left most named group as Perl */
	}
#endif
	else
	  return ONIGERR_INVALID_CONDITION_PATTERN;
	*np = node_new_enclose(ENCLOSE_CONDITION);
	CHECK_NULL_RETURN_MEMERR(*np);
	NENCLOSE(*np)->regnum = num;
	if (IS_NOT_NULL(name)) NENCLOSE(*np)->state |= NST_NAME_REF;
      }
      else
	return ONIGERR_UNDEFINED_GROUP_OPTION;
      break;

#if 0
    case '|':   /* branch reset: (?|...) */
      if (IS_SYNTAX_OP2(env->syntax, ONIG_SYN_OP2_QMARK_VBAR_BRANCH_RESET)) {
	/* TODO */
      }
      else
	return ONIGERR_UNDEFINED_GROUP_OPTION;
      break;
#endif

    case '^':   /* loads default options */
      if (IS_SYNTAX_OP2(env->syntax, ONIG_SYN_OP2_OPTION_PERL)) {
	/* d-imsx */
	ONOFF(option, ONIG_OPTION_ASCII_RANGE, 1);
	ONOFF(option, ONIG_OPTION_IGNORECASE, 1);
	ONOFF(option, ONIG_OPTION_SINGLELINE, 0);
	ONOFF(option, ONIG_OPTION_MULTILINE,  1);
	ONOFF(option, ONIG_OPTION_EXTEND, 1);
	PFETCH(c);
      }
#if 0
      else if (IS_SYNTAX_OP2(env->syntax, ONIG_SYN_OP2_OPTION_RUBY)) {
	/* d-imx */
	ONOFF(option, ONIG_OPTION_ASCII_RANGE, 0);
	ONOFF(option, ONIG_OPTION_POSIX_BRACKET_ALL_RANGE, 0);
	ONOFF(option, ONIG_OPTION_WORD_BOUND_ALL_RANGE, 0);
	ONOFF(option, ONIG_OPTION_IGNORECASE, 1);
	ONOFF(option, ONIG_OPTION_MULTILINE,  1);
	ONOFF(option, ONIG_OPTION_EXTEND, 1);
	PFETCH(c);
      }
#endif
      else {
	return ONIGERR_UNDEFINED_GROUP_OPTION;
      }
      /* fall through */
#ifdef USE_POSIXLINE_OPTION
    case 'p':
#endif
    case '-': case 'i': case 'm': case 's': case 'x':
    case 'a': case 'd': case 'l': case 'u':
      {
	int neg = 0;

	while (1) {
	  switch (c) {
	  case ':':
	  case ')':
	  break;

	  case '-':  neg = 1; break;
	  case 'x':  ONOFF(option, ONIG_OPTION_EXTEND,     neg); break;
	  case 'i':  ONOFF(option, ONIG_OPTION_IGNORECASE, neg); break;
	  case 's':
	    if (IS_SYNTAX_OP2(env->syntax, ONIG_SYN_OP2_OPTION_PERL)) {
	      ONOFF(option, ONIG_OPTION_MULTILINE,  neg);
	    }
	    else
	      return ONIGERR_UNDEFINED_GROUP_OPTION;
	    break;

	  case 'm':
	    if (IS_SYNTAX_OP2(env->syntax, ONIG_SYN_OP2_OPTION_PERL)) {
	      ONOFF(option, ONIG_OPTION_SINGLELINE, (neg == 0 ? 1 : 0));
	    }
	    else if (IS_SYNTAX_OP2(env->syntax, ONIG_SYN_OP2_OPTION_RUBY)) {
	      ONOFF(option, ONIG_OPTION_MULTILINE,  neg);
	    }
	    else
	      return ONIGERR_UNDEFINED_GROUP_OPTION;
	    break;
#ifdef USE_POSIXLINE_OPTION
	  case 'p':
	    ONOFF(option, ONIG_OPTION_MULTILINE|ONIG_OPTION_SINGLELINE, neg);
	    break;
#endif

	  case 'a':     /* limits \d, \s, \w and POSIX brackets to ASCII range */
	    if ((IS_SYNTAX_OP2(env->syntax, ONIG_SYN_OP2_OPTION_PERL) ||
		 IS_SYNTAX_OP2(env->syntax, ONIG_SYN_OP2_OPTION_RUBY)) &&
		(neg == 0)) {
	      ONOFF(option, ONIG_OPTION_ASCII_RANGE, 0);
	      ONOFF(option, ONIG_OPTION_POSIX_BRACKET_ALL_RANGE, 1);
	      ONOFF(option, ONIG_OPTION_WORD_BOUND_ALL_RANGE, 1);
	    }
	    else
	      return ONIGERR_UNDEFINED_GROUP_OPTION;
	    break;

	  case 'u':
	    if ((IS_SYNTAX_OP2(env->syntax, ONIG_SYN_OP2_OPTION_PERL) ||
		 IS_SYNTAX_OP2(env->syntax, ONIG_SYN_OP2_OPTION_RUBY)) &&
		(neg == 0)) {
	      ONOFF(option, ONIG_OPTION_ASCII_RANGE, 1);
	      ONOFF(option, ONIG_OPTION_POSIX_BRACKET_ALL_RANGE, 1);
	      ONOFF(option, ONIG_OPTION_WORD_BOUND_ALL_RANGE, 1);
	    }
	    else
	      return ONIGERR_UNDEFINED_GROUP_OPTION;
	    break;

	  case 'd':
	    if (IS_SYNTAX_OP2(env->syntax, ONIG_SYN_OP2_OPTION_PERL) &&
		(neg == 0)) {
	      ONOFF(option, ONIG_OPTION_ASCII_RANGE, 1);
	    }
	    else if (IS_SYNTAX_OP2(env->syntax, ONIG_SYN_OP2_OPTION_RUBY) &&
		(neg == 0)) {
	      ONOFF(option, ONIG_OPTION_ASCII_RANGE, 0);
	      ONOFF(option, ONIG_OPTION_POSIX_BRACKET_ALL_RANGE, 0);
	      ONOFF(option, ONIG_OPTION_WORD_BOUND_ALL_RANGE, 0);
	    }
	    else
	      return ONIGERR_UNDEFINED_GROUP_OPTION;
	    break;

	  case 'l':
	    if (IS_SYNTAX_OP2(env->syntax, ONIG_SYN_OP2_OPTION_PERL) && (neg == 0)) {
	      ONOFF(option, ONIG_OPTION_ASCII_RANGE, 1);
	    }
	    else
	      return ONIGERR_UNDEFINED_GROUP_OPTION;
	    break;

	  default:
	    return ONIGERR_UNDEFINED_GROUP_OPTION;
	  }

	  if (c == ')') {
	    *np = node_new_option(option);
	    CHECK_NULL_RETURN_MEMERR(*np);
	    *src = p;
	    return 2; /* option only */
	  }
	  else if (c == ':') {
	    OnigOptionType prev = env->option;

	    env->option     = option;
	    r = fetch_token(tok, &p, end, env);
	    if (r < 0) return r;
	    r = parse_subexp(&target, tok, term, &p, end, env);
	    env->option = prev;
	    if (r < 0) return r;
	    *np = node_new_option(option);
	    CHECK_NULL_RETURN_MEMERR(*np);
	    NENCLOSE(*np)->target = target;
	    *src = p;
	    return 0;
	  }

	  if (PEND) return ONIGERR_END_PATTERN_IN_GROUP;
	  PFETCH(c);
	}
      }
      break;

    default:
      return ONIGERR_UNDEFINED_GROUP_OPTION;
    }
  }
  else {
    if (ONIG_IS_OPTION_ON(env->option, ONIG_OPTION_DONT_CAPTURE_GROUP))
      goto group;

    *np = node_new_enclose_memory(env->option, 0);
    CHECK_NULL_RETURN_MEMERR(*np);
    num = scan_env_add_mem_entry(env);
    if (num < 0) return num;
    NENCLOSE(*np)->regnum = num;
  }

  CHECK_NULL_RETURN_MEMERR(*np);
  r = fetch_token(tok, &p, end, env);
  if (r < 0) return r;
  r = parse_subexp(&target, tok, term, &p, end, env);
  if (r < 0) {
    onig_node_free(target);
    return r;
  }

  if (NTYPE(*np) == NT_ANCHOR)
    NANCHOR(*np)->target = target;
  else {
    NENCLOSE(*np)->target = target;
    if (NENCLOSE(*np)->type == ENCLOSE_MEMORY) {
      /* Don't move this to previous of parse_subexp() */
      r = scan_env_set_mem_node(env, NENCLOSE(*np)->regnum, *np);
      if (r != 0) return r;
    }
    else if (NENCLOSE(*np)->type == ENCLOSE_CONDITION) {
      if (NTYPE(target) != NT_ALT) {
	/* convert (?(cond)yes) to (?(cond)yes|empty) */
	work1 = node_new_empty();
	if (IS_NULL(work1)) goto err;
	work2 = onig_node_new_alt(work1, NULL_NODE);
	if (IS_NULL(work2)) goto err;
	work1 = onig_node_new_alt(target, work2);
	if (IS_NULL(work1)) goto err;
	NENCLOSE(*np)->target = work1;
      }
    }
  }

  *src = p;
  return 0;

 err:
  onig_node_free(work1);
  onig_node_free(work2);
  onig_node_free(*np);
  *np = NULL;
  return ONIGERR_MEMORY;
}