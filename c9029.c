REGEXP *
Perl_re_op_compile(pTHX_ SV ** const patternp, int pat_count,
		    OP *expr, const regexp_engine* eng, REGEXP *old_re,
		     bool *is_bare_re, const U32 orig_rx_flags, const U32 pm_flags)
{
    dVAR;
    REGEXP *Rx;         /* Capital 'R' means points to a REGEXP */
    STRLEN plen;
    char *exp;
    regnode *scan;
    I32 flags;
    SSize_t minlen = 0;
    U32 rx_flags;
    SV *pat;
    SV** new_patternp = patternp;

    /* these are all flags - maybe they should be turned
     * into a single int with different bit masks */
    I32 sawlookahead = 0;
    I32 sawplus = 0;
    I32 sawopen = 0;
    I32 sawminmod = 0;

    regex_charset initial_charset = get_regex_charset(orig_rx_flags);
    bool recompile = 0;
    bool runtime_code = 0;
    scan_data_t data;
    RExC_state_t RExC_state;
    RExC_state_t * const pRExC_state = &RExC_state;
#ifdef TRIE_STUDY_OPT
    int restudied = 0;
    RExC_state_t copyRExC_state;
#endif
    GET_RE_DEBUG_FLAGS_DECL;

    PERL_ARGS_ASSERT_RE_OP_COMPILE;

    DEBUG_r(if (!PL_colorset) reginitcolors());

    /* Initialize these here instead of as-needed, as is quick and avoids
     * having to test them each time otherwise */
    if (! PL_InBitmap) {
#ifdef DEBUGGING
        char * dump_len_string;
#endif

        /* This is calculated here, because the Perl program that generates the
         * static global ones doesn't currently have access to
         * NUM_ANYOF_CODE_POINTS */
	PL_InBitmap = _new_invlist(2);
	PL_InBitmap = _add_range_to_invlist(PL_InBitmap, 0,
                                                    NUM_ANYOF_CODE_POINTS - 1);
#ifdef DEBUGGING
        dump_len_string = PerlEnv_getenv("PERL_DUMP_RE_MAX_LEN");
        if (   ! dump_len_string
            || ! grok_atoUV(dump_len_string, (UV *)&PL_dump_re_max_len, NULL))
        {
            PL_dump_re_max_len = 60;    /* A reasonable default */
        }
#endif
    }

    pRExC_state->warn_text = NULL;
    pRExC_state->unlexed_names = NULL;
    pRExC_state->code_blocks = NULL;

    if (is_bare_re)
	*is_bare_re = FALSE;

    if (expr && (expr->op_type == OP_LIST ||
		(expr->op_type == OP_NULL && expr->op_targ == OP_LIST))) {
	/* allocate code_blocks if needed */
	OP *o;
	int ncode = 0;

	for (o = cLISTOPx(expr)->op_first; o; o = OpSIBLING(o))
	    if (o->op_type == OP_NULL && (o->op_flags & OPf_SPECIAL))
		ncode++; /* count of DO blocks */

	if (ncode)
            pRExC_state->code_blocks = S_alloc_code_blocks(aTHX_ ncode);
    }

    if (!pat_count) {
        /* compile-time pattern with just OP_CONSTs and DO blocks */

        int n;
        OP *o;

        /* find how many CONSTs there are */
        assert(expr);
        n = 0;
        if (expr->op_type == OP_CONST)
            n = 1;
        else
            for (o = cLISTOPx(expr)->op_first; o; o = OpSIBLING(o)) {
                if (o->op_type == OP_CONST)
                    n++;
            }

        /* fake up an SV array */

        assert(!new_patternp);
        Newx(new_patternp, n, SV*);
        SAVEFREEPV(new_patternp);
        pat_count = n;

        n = 0;
        if (expr->op_type == OP_CONST)
            new_patternp[n] = cSVOPx_sv(expr);
        else
            for (o = cLISTOPx(expr)->op_first; o; o = OpSIBLING(o)) {
                if (o->op_type == OP_CONST)
                    new_patternp[n++] = cSVOPo_sv;
            }

    }

    DEBUG_PARSE_r(Perl_re_printf( aTHX_
        "Assembling pattern from %d elements%s\n", pat_count,
            orig_rx_flags & RXf_SPLIT ? " for split" : ""));

    /* set expr to the first arg op */

    if (pRExC_state->code_blocks && pRExC_state->code_blocks->count
         && expr->op_type != OP_CONST)
    {
            expr = cLISTOPx(expr)->op_first;
            assert(   expr->op_type == OP_PUSHMARK
                   || (expr->op_type == OP_NULL && expr->op_targ == OP_PUSHMARK)
                   || expr->op_type == OP_PADRANGE);
            expr = OpSIBLING(expr);
    }

    pat = S_concat_pat(aTHX_ pRExC_state, NULL, new_patternp, pat_count,
                        expr, &recompile, NULL);

    /* handle bare (possibly after overloading) regex: foo =~ $re */
    {
        SV *re = pat;
        if (SvROK(re))
            re = SvRV(re);
        if (SvTYPE(re) == SVt_REGEXP) {
            if (is_bare_re)
                *is_bare_re = TRUE;
            SvREFCNT_inc(re);
            DEBUG_PARSE_r(Perl_re_printf( aTHX_
                "Precompiled pattern%s\n",
                    orig_rx_flags & RXf_SPLIT ? " for split" : ""));

            return (REGEXP*)re;
        }
    }

    exp = SvPV_nomg(pat, plen);

    if (!eng->op_comp) {
	if ((SvUTF8(pat) && IN_BYTES)
		|| SvGMAGICAL(pat) || SvAMAGIC(pat))
	{
	    /* make a temporary copy; either to convert to bytes,
	     * or to avoid repeating get-magic / overloaded stringify */
	    pat = newSVpvn_flags(exp, plen, SVs_TEMP |
					(IN_BYTES ? 0 : SvUTF8(pat)));
	}
	return CALLREGCOMP_ENG(eng, pat, orig_rx_flags);
    }

    /* ignore the utf8ness if the pattern is 0 length */
    RExC_utf8 = RExC_orig_utf8 = (plen == 0 || IN_BYTES) ? 0 : SvUTF8(pat);
    RExC_uni_semantics = 0;
    RExC_contains_locale = 0;
    RExC_strict = cBOOL(pm_flags & RXf_PMf_STRICT);
    RExC_in_script_run = 0;
    RExC_study_started = 0;
    pRExC_state->runtime_code_qr = NULL;
    RExC_frame_head= NULL;
    RExC_frame_last= NULL;
    RExC_frame_count= 0;
    RExC_latest_warn_offset = 0;
    RExC_use_BRANCHJ = 0;
    RExC_total_parens = 0;
    RExC_open_parens = NULL;
    RExC_close_parens = NULL;
    RExC_paren_names = NULL;
    RExC_size = 0;
    RExC_seen_d_op = FALSE;
#ifdef DEBUGGING
    RExC_paren_name_list = NULL;
#endif

    DEBUG_r({
        RExC_mysv1= sv_newmortal();
        RExC_mysv2= sv_newmortal();
    });

    DEBUG_COMPILE_r({
            SV *dsv= sv_newmortal();
            RE_PV_QUOTED_DECL(s, RExC_utf8, dsv, exp, plen, PL_dump_re_max_len);
            Perl_re_printf( aTHX_  "%sCompiling REx%s %s\n",
                          PL_colors[4], PL_colors[5], s);
        });

    /* we jump here if we have to recompile, e.g., from upgrading the pattern
     * to utf8 */

    if ((pm_flags & PMf_USE_RE_EVAL)
		/* this second condition covers the non-regex literal case,
		 * i.e.  $foo =~ '(?{})'. */
		|| (IN_PERL_COMPILETIME && (PL_hints & HINT_RE_EVAL))
    )
	runtime_code = S_has_runtime_code(aTHX_ pRExC_state, exp, plen);

  redo_parse:
    /* return old regex if pattern hasn't changed */
    /* XXX: note in the below we have to check the flags as well as the
     * pattern.
     *
     * Things get a touch tricky as we have to compare the utf8 flag
     * independently from the compile flags.  */

    if (   old_re
        && !recompile
        && !!RX_UTF8(old_re) == !!RExC_utf8
        && ( RX_COMPFLAGS(old_re) == ( orig_rx_flags & RXf_PMf_FLAGCOPYMASK ) )
	&& RX_PRECOMP(old_re)
	&& RX_PRELEN(old_re) == plen
        && memEQ(RX_PRECOMP(old_re), exp, plen)
	&& !runtime_code /* with runtime code, always recompile */ )
    {
        return old_re;
    }

    /* Allocate the pattern's SV */
    RExC_rx_sv = Rx = (REGEXP*) newSV_type(SVt_REGEXP);
    RExC_rx = ReANY(Rx);
    if ( RExC_rx == NULL )
        FAIL("Regexp out of space");

    rx_flags = orig_rx_flags;

    if (   (UTF || RExC_uni_semantics)
        && initial_charset == REGEX_DEPENDS_CHARSET)
    {

	/* Set to use unicode semantics if the pattern is in utf8 and has the
	 * 'depends' charset specified, as it means unicode when utf8  */
	set_regex_charset(&rx_flags, REGEX_UNICODE_CHARSET);
        RExC_uni_semantics = 1;
    }

    RExC_pm_flags = pm_flags;

    if (runtime_code) {
        assert(TAINTING_get || !TAINT_get);
	if (TAINT_get)
	    Perl_croak(aTHX_ "Eval-group in insecure regular expression");

	if (!S_compile_runtime_code(aTHX_ pRExC_state, exp, plen)) {
	    /* whoops, we have a non-utf8 pattern, whilst run-time code
	     * got compiled as utf8. Try again with a utf8 pattern */
            S_pat_upgrade_to_utf8(aTHX_ pRExC_state, &exp, &plen,
                pRExC_state->code_blocks ? pRExC_state->code_blocks->count : 0);
            goto redo_parse;
	}
    }
    assert(!pRExC_state->runtime_code_qr);

    RExC_sawback = 0;

    RExC_seen = 0;
    RExC_maxlen = 0;
    RExC_in_lookbehind = 0;
    RExC_seen_zerolen = *exp == '^' ? -1 : 0;
#ifdef EBCDIC
    RExC_recode_x_to_native = 0;
#endif
    RExC_in_multi_char_class = 0;

    RExC_start = RExC_copy_start_in_constructed = RExC_copy_start_in_input = RExC_precomp = exp;
    RExC_precomp_end = RExC_end = exp + plen;
    RExC_nestroot = 0;
    RExC_whilem_seen = 0;
    RExC_end_op = NULL;
    RExC_recurse = NULL;
    RExC_study_chunk_recursed = NULL;
    RExC_study_chunk_recursed_bytes= 0;
    RExC_recurse_count = 0;
    pRExC_state->code_index = 0;

    /* Initialize the string in the compiled pattern.  This is so that there is
     * something to output if necessary */
    set_regex_pv(pRExC_state, Rx);

    DEBUG_PARSE_r({
        Perl_re_printf( aTHX_
            "Starting parse and generation\n");
        RExC_lastnum=0;
        RExC_lastparse=NULL;
    });

    /* Allocate space and zero-initialize. Note, the two step process
       of zeroing when in debug mode, thus anything assigned has to
       happen after that */
    if (!  RExC_size) {

        /* On the first pass of the parse, we guess how big this will be.  Then
         * we grow in one operation to that amount and then give it back.  As
         * we go along, we re-allocate what we need.
         *
         * XXX Currently the guess is essentially that the pattern will be an
         * EXACT node with one byte input, one byte output.  This is crude, and
         * better heuristics are welcome.
         *
         * On any subsequent passes, we guess what we actually computed in the
         * latest earlier pass.  Such a pass probably didn't complete so is
         * missing stuff.  We could improve those guesses by knowing where the
         * parse stopped, and use the length so far plus apply the above
         * assumption to what's left. */
        RExC_size = STR_SZ(RExC_end - RExC_start);
    }

    Newxc(RExC_rxi, sizeof(regexp_internal) + RExC_size, char, regexp_internal);
    if ( RExC_rxi == NULL )
        FAIL("Regexp out of space");

    Zero(RExC_rxi, sizeof(regexp_internal) + RExC_size, char);
    RXi_SET( RExC_rx, RExC_rxi );

    /* We start from 0 (over from 0 in the case this is a reparse.  The first
     * node parsed will give back any excess memory we have allocated so far).
     * */
    RExC_size = 0;

    /* non-zero initialization begins here */
    RExC_rx->engine= eng;
    RExC_rx->extflags = rx_flags;
    RXp_COMPFLAGS(RExC_rx) = orig_rx_flags & RXf_PMf_FLAGCOPYMASK;

    if (pm_flags & PMf_IS_QR) {
	RExC_rxi->code_blocks = pRExC_state->code_blocks;
        if (RExC_rxi->code_blocks) {
            RExC_rxi->code_blocks->refcnt++;
        }
    }

    RExC_rx->intflags = 0;

    RExC_flags = rx_flags;	/* don't let top level (?i) bleed */
    RExC_parse = exp;

    /* This NUL is guaranteed because the pattern comes from an SV*, and the sv
     * code makes sure the final byte is an uncounted NUL.  But should this
     * ever not be the case, lots of things could read beyond the end of the
     * buffer: loops like
     *      while(isFOO(*RExC_parse)) RExC_parse++;
     *      strchr(RExC_parse, "foo");
     * etc.  So it is worth noting. */
    assert(*RExC_end == '\0');

    RExC_naughty = 0;
    RExC_npar = 1;
    RExC_parens_buf_size = 0;
    RExC_emit_start = RExC_rxi->program;
    pRExC_state->code_index = 0;

    *((char*) RExC_emit_start) = (char) REG_MAGIC;
    RExC_emit = 1;

    /* Do the parse */
    if (reg(pRExC_state, 0, &flags, 1)) {

        /* Success!, But we may need to redo the parse knowing how many parens
         * there actually are */
        if (IN_PARENS_PASS) {
            flags |= RESTART_PARSE;
        }

        /* We have that number in RExC_npar */
        RExC_total_parens = RExC_npar;
    }
    else if (! MUST_RESTART(flags)) {
	ReREFCNT_dec(Rx);
        Perl_croak(aTHX_ "panic: reg returned failure to re_op_compile, flags=%#" UVxf, (UV) flags);
    }

    /* Here, we either have success, or we have to redo the parse for some reason */
    if (MUST_RESTART(flags)) {

        /* It's possible to write a regexp in ascii that represents Unicode
        codepoints outside of the byte range, such as via \x{100}. If we
        detect such a sequence we have to convert the entire pattern to utf8
        and then recompile, as our sizing calculation will have been based
        on 1 byte == 1 character, but we will need to use utf8 to encode
        at least some part of the pattern, and therefore must convert the whole
        thing.
        -- dmq */
        if (flags & NEED_UTF8) {

            /* We have stored the offset of the final warning output so far.
             * That must be adjusted.  Any variant characters between the start
             * of the pattern and this warning count for 2 bytes in the final,
             * so just add them again */
            if (UNLIKELY(RExC_latest_warn_offset > 0)) {
                RExC_latest_warn_offset +=
                            variant_under_utf8_count((U8 *) exp, (U8 *) exp
                                                + RExC_latest_warn_offset);
            }
            S_pat_upgrade_to_utf8(aTHX_ pRExC_state, &exp, &plen,
            pRExC_state->code_blocks ? pRExC_state->code_blocks->count : 0);
            DEBUG_PARSE_r(Perl_re_printf( aTHX_ "Need to redo parse after upgrade\n"));
        }
        else {
            DEBUG_PARSE_r(Perl_re_printf( aTHX_ "Need to redo parse\n"));
        }

        if (ALL_PARENS_COUNTED) {
            /* Make enough room for all the known parens, and zero it */
            Renew(RExC_open_parens, RExC_total_parens, regnode_offset);
            Zero(RExC_open_parens, RExC_total_parens, regnode_offset);
            RExC_open_parens[0] = 1;    /* +1 for REG_MAGIC */

            Renew(RExC_close_parens, RExC_total_parens, regnode_offset);
            Zero(RExC_close_parens, RExC_total_parens, regnode_offset);
        }
        else { /* Parse did not complete.  Reinitialize the parentheses
                  structures */
            RExC_total_parens = 0;
            if (RExC_open_parens) {
                Safefree(RExC_open_parens);
                RExC_open_parens = NULL;
            }
            if (RExC_close_parens) {
                Safefree(RExC_close_parens);
                RExC_close_parens = NULL;
            }
        }

        /* Clean up what we did in this parse */
        SvREFCNT_dec_NN(RExC_rx_sv);

        goto redo_parse;
    }

    /* Here, we have successfully parsed and generated the pattern's program
     * for the regex engine.  We are ready to finish things up and look for
     * optimizations. */

    /* Update the string to compile, with correct modifiers, etc */
    set_regex_pv(pRExC_state, Rx);

    RExC_rx->nparens = RExC_total_parens - 1;

    /* Uses the upper 4 bits of the FLAGS field, so keep within that size */
    if (RExC_whilem_seen > 15)
        RExC_whilem_seen = 15;

    DEBUG_PARSE_r({
        Perl_re_printf( aTHX_
            "Required size %" IVdf " nodes\n", (IV)RExC_size);
        RExC_lastnum=0;
        RExC_lastparse=NULL;
    });

#ifdef RE_TRACK_PATTERN_OFFSETS
    DEBUG_OFFSETS_r(Perl_re_printf( aTHX_
                          "%s %" UVuf " bytes for offset annotations.\n",
                          RExC_offsets ? "Got" : "Couldn't get",
                          (UV)((RExC_offsets[0] * 2 + 1))));
    DEBUG_OFFSETS_r(if (RExC_offsets) {
        const STRLEN len = RExC_offsets[0];
        STRLEN i;
        GET_RE_DEBUG_FLAGS_DECL;
        Perl_re_printf( aTHX_
                      "Offsets: [%" UVuf "]\n\t", (UV)RExC_offsets[0]);
        for (i = 1; i <= len; i++) {
            if (RExC_offsets[i*2-1] || RExC_offsets[i*2])
                Perl_re_printf( aTHX_  "%" UVuf ":%" UVuf "[%" UVuf "] ",
                (UV)i, (UV)RExC_offsets[i*2-1], (UV)RExC_offsets[i*2]);
        }
        Perl_re_printf( aTHX_  "\n");
    });

#else
    SetProgLen(RExC_rxi,RExC_size);
#endif

    DEBUG_OPTIMISE_r(
        Perl_re_printf( aTHX_  "Starting post parse optimization\n");
    );

    /* XXXX To minimize changes to RE engine we always allocate
       3-units-long substrs field. */
    Newx(RExC_rx->substrs, 1, struct reg_substr_data);
    if (RExC_recurse_count) {
        Newx(RExC_recurse, RExC_recurse_count, regnode *);
        SAVEFREEPV(RExC_recurse);
    }

    if (RExC_seen & REG_RECURSE_SEEN) {
        /* Note, RExC_total_parens is 1 + the number of parens in a pattern.
         * So its 1 if there are no parens. */
        RExC_study_chunk_recursed_bytes= (RExC_total_parens >> 3) +
                                         ((RExC_total_parens & 0x07) != 0);
        Newx(RExC_study_chunk_recursed,
             RExC_study_chunk_recursed_bytes * RExC_total_parens, U8);
        SAVEFREEPV(RExC_study_chunk_recursed);
    }

  reStudy:
    RExC_rx->minlen = minlen = sawlookahead = sawplus = sawopen = sawminmod = 0;
    DEBUG_r(
        RExC_study_chunk_recursed_count= 0;
    );
    Zero(RExC_rx->substrs, 1, struct reg_substr_data);
    if (RExC_study_chunk_recursed) {
        Zero(RExC_study_chunk_recursed,
             RExC_study_chunk_recursed_bytes * RExC_total_parens, U8);
    }


#ifdef TRIE_STUDY_OPT
    if (!restudied) {
        StructCopy(&zero_scan_data, &data, scan_data_t);
        copyRExC_state = RExC_state;
    } else {
        U32 seen=RExC_seen;
        DEBUG_OPTIMISE_r(Perl_re_printf( aTHX_ "Restudying\n"));

        RExC_state = copyRExC_state;
        if (seen & REG_TOP_LEVEL_BRANCHES_SEEN)
            RExC_seen |= REG_TOP_LEVEL_BRANCHES_SEEN;
        else
            RExC_seen &= ~REG_TOP_LEVEL_BRANCHES_SEEN;
	StructCopy(&zero_scan_data, &data, scan_data_t);
    }
#else
    StructCopy(&zero_scan_data, &data, scan_data_t);
#endif

    /* Dig out information for optimizations. */
    RExC_rx->extflags = RExC_flags; /* was pm_op */
    /*dmq: removed as part of de-PMOP: pm->op_pmflags = RExC_flags; */

    if (UTF)
	SvUTF8_on(Rx);	/* Unicode in it? */
    RExC_rxi->regstclass = NULL;
    if (RExC_naughty >= TOO_NAUGHTY)	/* Probably an expensive pattern. */
	RExC_rx->intflags |= PREGf_NAUGHTY;
    scan = RExC_rxi->program + 1;		/* First BRANCH. */

    /* testing for BRANCH here tells us whether there is "must appear"
       data in the pattern. If there is then we can use it for optimisations */
    if (!(RExC_seen & REG_TOP_LEVEL_BRANCHES_SEEN)) { /*  Only one top-level choice.
                                                  */
	SSize_t fake;
	STRLEN longest_length[2];
	regnode_ssc ch_class; /* pointed to by data */
	int stclass_flag;
	SSize_t last_close = 0; /* pointed to by data */
        regnode *first= scan;
        regnode *first_next= regnext(first);
        int i;

	/*
	 * Skip introductions and multiplicators >= 1
	 * so that we can extract the 'meat' of the pattern that must
	 * match in the large if() sequence following.
	 * NOTE that EXACT is NOT covered here, as it is normally
	 * picked up by the optimiser separately.
	 *
	 * This is unfortunate as the optimiser isnt handling lookahead
	 * properly currently.
	 *
	 */
	while ((OP(first) == OPEN && (sawopen = 1)) ||
	       /* An OR of *one* alternative - should not happen now. */
	    (OP(first) == BRANCH && OP(first_next) != BRANCH) ||
	    /* for now we can't handle lookbehind IFMATCH*/
	    (OP(first) == IFMATCH && !first->flags && (sawlookahead = 1)) ||
	    (OP(first) == PLUS) ||
	    (OP(first) == MINMOD) ||
	       /* An {n,m} with n>0 */
	    (PL_regkind[OP(first)] == CURLY && ARG1(first) > 0) ||
	    (OP(first) == NOTHING && PL_regkind[OP(first_next)] != END ))
	{
		/*
		 * the only op that could be a regnode is PLUS, all the rest
		 * will be regnode_1 or regnode_2.
		 *
                 * (yves doesn't think this is true)
		 */
		if (OP(first) == PLUS)
		    sawplus = 1;
                else {
                    if (OP(first) == MINMOD)
                        sawminmod = 1;
		    first += regarglen[OP(first)];
                }
		first = NEXTOPER(first);
		first_next= regnext(first);
	}

	/* Starting-point info. */
      again:
        DEBUG_PEEP("first:", first, 0, 0);
        /* Ignore EXACT as we deal with it later. */
	if (PL_regkind[OP(first)] == EXACT) {
	    if (   OP(first) == EXACT
                || OP(first) == EXACT_ONLY8
                || OP(first) == EXACTL)
            {
		NOOP;	/* Empty, get anchored substr later. */
            }
	    else
		RExC_rxi->regstclass = first;
	}
#ifdef TRIE_STCLASS
	else if (PL_regkind[OP(first)] == TRIE &&
	        ((reg_trie_data *)RExC_rxi->data->data[ ARG(first) ])->minlen>0)
	{
            /* this can happen only on restudy */
            RExC_rxi->regstclass = construct_ahocorasick_from_trie(pRExC_state, (regnode *)first, 0);
	}
#endif
	else if (REGNODE_SIMPLE(OP(first)))
	    RExC_rxi->regstclass = first;
	else if (PL_regkind[OP(first)] == BOUND ||
		 PL_regkind[OP(first)] == NBOUND)
	    RExC_rxi->regstclass = first;
	else if (PL_regkind[OP(first)] == BOL) {
            RExC_rx->intflags |= (OP(first) == MBOL
                           ? PREGf_ANCH_MBOL
                           : PREGf_ANCH_SBOL);
	    first = NEXTOPER(first);
	    goto again;
	}
	else if (OP(first) == GPOS) {
            RExC_rx->intflags |= PREGf_ANCH_GPOS;
	    first = NEXTOPER(first);
	    goto again;
	}
	else if ((!sawopen || !RExC_sawback) &&
            !sawlookahead &&
	    (OP(first) == STAR &&
	    PL_regkind[OP(NEXTOPER(first))] == REG_ANY) &&
            !(RExC_rx->intflags & PREGf_ANCH) && !pRExC_state->code_blocks)
	{
	    /* turn .* into ^.* with an implied $*=1 */
	    const int type =
		(OP(NEXTOPER(first)) == REG_ANY)
                    ? PREGf_ANCH_MBOL
                    : PREGf_ANCH_SBOL;
            RExC_rx->intflags |= (type | PREGf_IMPLICIT);
	    first = NEXTOPER(first);
	    goto again;
	}
        if (sawplus && !sawminmod && !sawlookahead
            && (!sawopen || !RExC_sawback)
	    && !pRExC_state->code_blocks) /* May examine pos and $& */
	    /* x+ must match at the 1st pos of run of x's */
	    RExC_rx->intflags |= PREGf_SKIP;

	/* Scan is after the zeroth branch, first is atomic matcher. */
#ifdef TRIE_STUDY_OPT
	DEBUG_PARSE_r(
	    if (!restudied)
                Perl_re_printf( aTHX_  "first at %" IVdf "\n",
			      (IV)(first - scan + 1))
        );
#else
	DEBUG_PARSE_r(
            Perl_re_printf( aTHX_  "first at %" IVdf "\n",
	        (IV)(first - scan + 1))
        );
#endif


	/*
	* If there's something expensive in the r.e., find the
	* longest literal string that must appear and make it the
	* regmust.  Resolve ties in favor of later strings, since
	* the regstart check works with the beginning of the r.e.
	* and avoiding duplication strengthens checking.  Not a
	* strong reason, but sufficient in the absence of others.
	* [Now we resolve ties in favor of the earlier string if
	* it happens that c_offset_min has been invalidated, since the
	* earlier string may buy us something the later one won't.]
	*/

	data.substrs[0].str = newSVpvs("");
	data.substrs[1].str = newSVpvs("");
	data.last_found = newSVpvs("");
	data.cur_is_floating = 0; /* initially any found substring is fixed */
	ENTER_with_name("study_chunk");
	SAVEFREESV(data.substrs[0].str);
	SAVEFREESV(data.substrs[1].str);
	SAVEFREESV(data.last_found);
	first = scan;
	if (!RExC_rxi->regstclass) {
	    ssc_init(pRExC_state, &ch_class);
	    data.start_class = &ch_class;
	    stclass_flag = SCF_DO_STCLASS_AND;
	} else				/* XXXX Check for BOUND? */
	    stclass_flag = 0;
	data.last_closep = &last_close;

        DEBUG_RExC_seen();
        /*
         * MAIN ENTRY FOR study_chunk() FOR m/PATTERN/
         * (NO top level branches)
         */
	minlen = study_chunk(pRExC_state, &first, &minlen, &fake,
                             scan + RExC_size, /* Up to end */
            &data, -1, 0, NULL,
            SCF_DO_SUBSTR | SCF_WHILEM_VISITED_POS | stclass_flag
                          | (restudied ? SCF_TRIE_DOING_RESTUDY : 0),
            0);


        CHECK_RESTUDY_GOTO_butfirst(LEAVE_with_name("study_chunk"));


	if ( RExC_total_parens == 1 && !data.cur_is_floating
	     && data.last_start_min == 0 && data.last_end > 0
	     && !RExC_seen_zerolen
             && !(RExC_seen & REG_VERBARG_SEEN)
             && !(RExC_seen & REG_GPOS_SEEN)
        ){
	    RExC_rx->extflags |= RXf_CHECK_ALL;
        }
	scan_commit(pRExC_state, &data,&minlen, 0);


        /* XXX this is done in reverse order because that's the way the
         * code was before it was parameterised. Don't know whether it
         * actually needs doing in reverse order. DAPM */
        for (i = 1; i >= 0; i--) {
            longest_length[i] = CHR_SVLEN(data.substrs[i].str);

            if (   !(   i
                     && SvCUR(data.substrs[0].str)  /* ok to leave SvCUR */
                     &&    data.substrs[0].min_offset
                        == data.substrs[1].min_offset
                     &&    SvCUR(data.substrs[0].str)
                        == SvCUR(data.substrs[1].str)
                    )
                && S_setup_longest (aTHX_ pRExC_state,
                                        &(RExC_rx->substrs->data[i]),
                                        &(data.substrs[i]),
                                        longest_length[i]))
            {
                RExC_rx->substrs->data[i].min_offset =
                        data.substrs[i].min_offset - data.substrs[i].lookbehind;

                RExC_rx->substrs->data[i].max_offset = data.substrs[i].max_offset;
                /* Don't offset infinity */
                if (data.substrs[i].max_offset < SSize_t_MAX)
                    RExC_rx->substrs->data[i].max_offset -= data.substrs[i].lookbehind;
                SvREFCNT_inc_simple_void_NN(data.substrs[i].str);
            }
            else {
                RExC_rx->substrs->data[i].substr      = NULL;
                RExC_rx->substrs->data[i].utf8_substr = NULL;
                longest_length[i] = 0;
            }
        }

	LEAVE_with_name("study_chunk");

	if (RExC_rxi->regstclass
	    && (OP(RExC_rxi->regstclass) == REG_ANY || OP(RExC_rxi->regstclass) == SANY))
	    RExC_rxi->regstclass = NULL;

	if ((!(RExC_rx->substrs->data[0].substr || RExC_rx->substrs->data[0].utf8_substr)
              || RExC_rx->substrs->data[0].min_offset)
	    && stclass_flag
            && ! (ANYOF_FLAGS(data.start_class) & SSC_MATCHES_EMPTY_STRING)
	    && is_ssc_worth_it(pRExC_state, data.start_class))
	{
	    const U32 n = add_data(pRExC_state, STR_WITH_LEN("f"));

            ssc_finalize(pRExC_state, data.start_class);

	    Newx(RExC_rxi->data->data[n], 1, regnode_ssc);
	    StructCopy(data.start_class,
		       (regnode_ssc*)RExC_rxi->data->data[n],
		       regnode_ssc);
	    RExC_rxi->regstclass = (regnode*)RExC_rxi->data->data[n];
	    RExC_rx->intflags &= ~PREGf_SKIP;	/* Used in find_byclass(). */
	    DEBUG_COMPILE_r({ SV *sv = sv_newmortal();
                      regprop(RExC_rx, sv, (regnode*)data.start_class, NULL, pRExC_state);
                      Perl_re_printf( aTHX_
				    "synthetic stclass \"%s\".\n",
				    SvPVX_const(sv));});
            data.start_class = NULL;
	}

        /* A temporary algorithm prefers floated substr to fixed one of
         * same length to dig more info. */
	i = (longest_length[0] <= longest_length[1]);
        RExC_rx->substrs->check_ix = i;
        RExC_rx->check_end_shift  = RExC_rx->substrs->data[i].end_shift;
        RExC_rx->check_substr     = RExC_rx->substrs->data[i].substr;
        RExC_rx->check_utf8       = RExC_rx->substrs->data[i].utf8_substr;
        RExC_rx->check_offset_min = RExC_rx->substrs->data[i].min_offset;
        RExC_rx->check_offset_max = RExC_rx->substrs->data[i].max_offset;
        if (!i && (RExC_rx->intflags & (PREGf_ANCH_SBOL|PREGf_ANCH_GPOS)))
            RExC_rx->intflags |= PREGf_NOSCAN;

	if ((RExC_rx->check_substr || RExC_rx->check_utf8) ) {
	    RExC_rx->extflags |= RXf_USE_INTUIT;
	    if (SvTAIL(RExC_rx->check_substr ? RExC_rx->check_substr : RExC_rx->check_utf8))
		RExC_rx->extflags |= RXf_INTUIT_TAIL;
	}

	/* XXX Unneeded? dmq (shouldn't as this is handled elsewhere)
	if ( (STRLEN)minlen < longest_length[1] )
            minlen= longest_length[1];
        if ( (STRLEN)minlen < longest_length[0] )
            minlen= longest_length[0];
        */
    }
    else {
	/* Several toplevels. Best we can is to set minlen. */
	SSize_t fake;
	regnode_ssc ch_class;
	SSize_t last_close = 0;

        DEBUG_PARSE_r(Perl_re_printf( aTHX_  "\nMulti Top Level\n"));

	scan = RExC_rxi->program + 1;
	ssc_init(pRExC_state, &ch_class);
	data.start_class = &ch_class;
	data.last_closep = &last_close;

        DEBUG_RExC_seen();
        /*
         * MAIN ENTRY FOR study_chunk() FOR m/P1|P2|.../
         * (patterns WITH top level branches)
         */
	minlen = study_chunk(pRExC_state,
            &scan, &minlen, &fake, scan + RExC_size, &data, -1, 0, NULL,
            SCF_DO_STCLASS_AND|SCF_WHILEM_VISITED_POS|(restudied
                                                      ? SCF_TRIE_DOING_RESTUDY
                                                      : 0),
            0);

        CHECK_RESTUDY_GOTO_butfirst(NOOP);

	RExC_rx->check_substr = NULL;
        RExC_rx->check_utf8 = NULL;
        RExC_rx->substrs->data[0].substr      = NULL;
        RExC_rx->substrs->data[0].utf8_substr = NULL;
        RExC_rx->substrs->data[1].substr      = NULL;
        RExC_rx->substrs->data[1].utf8_substr = NULL;

        if (! (ANYOF_FLAGS(data.start_class) & SSC_MATCHES_EMPTY_STRING)
	    && is_ssc_worth_it(pRExC_state, data.start_class))
        {
	    const U32 n = add_data(pRExC_state, STR_WITH_LEN("f"));

            ssc_finalize(pRExC_state, data.start_class);

	    Newx(RExC_rxi->data->data[n], 1, regnode_ssc);
	    StructCopy(data.start_class,
		       (regnode_ssc*)RExC_rxi->data->data[n],
		       regnode_ssc);
	    RExC_rxi->regstclass = (regnode*)RExC_rxi->data->data[n];
	    RExC_rx->intflags &= ~PREGf_SKIP;	/* Used in find_byclass(). */
	    DEBUG_COMPILE_r({ SV* sv = sv_newmortal();
                      regprop(RExC_rx, sv, (regnode*)data.start_class, NULL, pRExC_state);
                      Perl_re_printf( aTHX_
				    "synthetic stclass \"%s\".\n",
				    SvPVX_const(sv));});
            data.start_class = NULL;
	}
    }

    if (RExC_seen & REG_UNBOUNDED_QUANTIFIER_SEEN) {
        RExC_rx->extflags |= RXf_UNBOUNDED_QUANTIFIER_SEEN;
        RExC_rx->maxlen = REG_INFTY;
    }
    else {
        RExC_rx->maxlen = RExC_maxlen;
    }

    /* Guard against an embedded (?=) or (?<=) with a longer minlen than
       the "real" pattern. */
    DEBUG_OPTIMISE_r({
        Perl_re_printf( aTHX_ "minlen: %" IVdf " RExC_rx->minlen:%" IVdf " maxlen:%" IVdf "\n",
                      (IV)minlen, (IV)RExC_rx->minlen, (IV)RExC_maxlen);
    });
    RExC_rx->minlenret = minlen;
    if (RExC_rx->minlen < minlen)
        RExC_rx->minlen = minlen;

    if (RExC_seen & REG_RECURSE_SEEN ) {
        RExC_rx->intflags |= PREGf_RECURSE_SEEN;
        Newx(RExC_rx->recurse_locinput, RExC_rx->nparens + 1, char *);
    }
    if (RExC_seen & REG_GPOS_SEEN)
        RExC_rx->intflags |= PREGf_GPOS_SEEN;
    if (RExC_seen & REG_LOOKBEHIND_SEEN)
        RExC_rx->extflags |= RXf_NO_INPLACE_SUBST; /* inplace might break the
                                                lookbehind */
    if (pRExC_state->code_blocks)
	RExC_rx->extflags |= RXf_EVAL_SEEN;
    if (RExC_seen & REG_VERBARG_SEEN)
    {
	RExC_rx->intflags |= PREGf_VERBARG_SEEN;
        RExC_rx->extflags |= RXf_NO_INPLACE_SUBST; /* don't understand this! Yves */
    }
    if (RExC_seen & REG_CUTGROUP_SEEN)
	RExC_rx->intflags |= PREGf_CUTGROUP_SEEN;
    if (pm_flags & PMf_USE_RE_EVAL)
	RExC_rx->intflags |= PREGf_USE_RE_EVAL;
    if (RExC_paren_names)
        RXp_PAREN_NAMES(RExC_rx) = MUTABLE_HV(SvREFCNT_inc(RExC_paren_names));
    else
        RXp_PAREN_NAMES(RExC_rx) = NULL;

    /* If we have seen an anchor in our pattern then we set the extflag RXf_IS_ANCHORED
     * so it can be used in pp.c */
    if (RExC_rx->intflags & PREGf_ANCH)
        RExC_rx->extflags |= RXf_IS_ANCHORED;


    {
        /* this is used to identify "special" patterns that might result
         * in Perl NOT calling the regex engine and instead doing the match "itself",
         * particularly special cases in split//. By having the regex compiler
         * do this pattern matching at a regop level (instead of by inspecting the pattern)
         * we avoid weird issues with equivalent patterns resulting in different behavior,
         * AND we allow non Perl engines to get the same optimizations by the setting the
         * flags appropriately - Yves */
        regnode *first = RExC_rxi->program + 1;
        U8 fop = OP(first);
        regnode *next = regnext(first);
        U8 nop = OP(next);

        if (PL_regkind[fop] == NOTHING && nop == END)
            RExC_rx->extflags |= RXf_NULL;
        else if ((fop == MBOL || (fop == SBOL && !first->flags)) && nop == END)
            /* when fop is SBOL first->flags will be true only when it was
             * produced by parsing /\A/, and not when parsing /^/. This is
             * very important for the split code as there we want to
             * treat /^/ as /^/m, but we do not want to treat /\A/ as /^/m.
             * See rt #122761 for more details. -- Yves */
            RExC_rx->extflags |= RXf_START_ONLY;
        else if (fop == PLUS
                 && PL_regkind[nop] == POSIXD && FLAGS(next) == _CC_SPACE
                 && nop == END)
            RExC_rx->extflags |= RXf_WHITE;
        else if ( RExC_rx->extflags & RXf_SPLIT
                  && (fop == EXACT || fop == EXACT_ONLY8 || fop == EXACTL)
                  && STR_LEN(first) == 1
                  && *(STRING(first)) == ' '
                  && nop == END )
            RExC_rx->extflags |= (RXf_SKIPWHITE|RXf_WHITE);

    }

    if (RExC_contains_locale) {
        RXp_EXTFLAGS(RExC_rx) |= RXf_TAINTED;
    }

#ifdef DEBUGGING
    if (RExC_paren_names) {
        RExC_rxi->name_list_idx = add_data( pRExC_state, STR_WITH_LEN("a"));
        RExC_rxi->data->data[RExC_rxi->name_list_idx]
                                   = (void*)SvREFCNT_inc(RExC_paren_name_list);
    } else
#endif
    RExC_rxi->name_list_idx = 0;

    while ( RExC_recurse_count > 0 ) {
        const regnode *scan = RExC_recurse[ --RExC_recurse_count ];
        /*
         * This data structure is set up in study_chunk() and is used
         * to calculate the distance between a GOSUB regopcode and
         * the OPEN/CURLYM (CURLYM's are special and can act like OPEN's)
         * it refers to.
         *
         * If for some reason someone writes code that optimises
         * away a GOSUB opcode then the assert should be changed to
         * an if(scan) to guard the ARG2L_SET() - Yves
         *
         */
        assert(scan && OP(scan) == GOSUB);
        ARG2L_SET( scan, RExC_open_parens[ARG(scan)] - REGNODE_OFFSET(scan));
    }

    Newxz(RExC_rx->offs, RExC_total_parens, regexp_paren_pair);
    /* assume we don't need to swap parens around before we match */
    DEBUG_TEST_r({
        Perl_re_printf( aTHX_ "study_chunk_recursed_count: %lu\n",
            (unsigned long)RExC_study_chunk_recursed_count);
    });
    DEBUG_DUMP_r({
        DEBUG_RExC_seen();
        Perl_re_printf( aTHX_ "Final program:\n");
        regdump(RExC_rx);
    });

    if (RExC_open_parens) {
        Safefree(RExC_open_parens);
        RExC_open_parens = NULL;
    }
    if (RExC_close_parens) {
        Safefree(RExC_close_parens);
        RExC_close_parens = NULL;
    }

#ifdef USE_ITHREADS
    /* under ithreads the ?pat? PMf_USED flag on the pmop is simulated
     * by setting the regexp SV to readonly-only instead. If the
     * pattern's been recompiled, the USEDness should remain. */
    if (old_re && SvREADONLY(old_re))
        SvREADONLY_on(Rx);
#endif
    return Rx;