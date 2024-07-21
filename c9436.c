static int do_directive(Token *tline, Token **output)
{
    enum preproc_token op;
    int j;
    bool err;
    enum nolist_flags nolist;
    bool casesense;
    int k, m;
    int offset;
    const char *p;
    char *q, *qbuf;
    const char *found_path;
    const char *mname;
    struct ppscan pps;
    Include *inc;
    Context *ctx;
    Cond *cond;
    MMacro *mmac, **mmhead;
    Token *t = NULL, *tt, *macro_start, *last, *origline;
    Line *l;
    struct tokenval tokval;
    expr *evalresult;
    int64_t count;
    size_t len;
    errflags severity;
    const char *dname;          /* Name of directive, for messages */

    *output = NULL;             /* No output generated */
    origline = tline;

    if (tok_is(tline, '#')) {
        /* cpp-style line directive */
        if (!tok_white(tline->next))
            return NO_DIRECTIVE_FOUND;
        dname = tok_text(tline);
        goto pp_line;
    }

    tline = skip_white(tline);
    if (!tline || !tok_type(tline, TOK_PREPROC_ID))
	return NO_DIRECTIVE_FOUND;

    dname = tok_text(tline);
    if (dname[1] == '%')
	return NO_DIRECTIVE_FOUND;

    op = pp_token_hash(dname);

    casesense = true;
    if (PP_HAS_CASE(op) & PP_INSENSITIVE(op)) {
        casesense = false;
        op--;
    }

    /*
     * %line directives are always processed immediately and
     * unconditionally, as they are intended to reflect position
     * in externally preprocessed sources.
     */
    if (op == PP_LINE) {
    pp_line:
        /*
         * Syntax is `%line nnn[+mmm] [filename]'
         */
        if (pp_noline || istk->mstk.mstk)
            goto done;

        tline = tline->next;
        tline = skip_white(tline);
        if (!tok_type(tline, TOK_NUMBER)) {
            nasm_nonfatal("`%s' expects line number", dname);
            goto done;
        }
        k = readnum(tok_text(tline), &err);
        m = 1;
        tline = tline->next;
        if (tok_is(tline, '+') || tok_is(tline, '-')) {
            bool minus = tok_is(tline, '-');
            tline = tline->next;
            if (!tok_type(tline, TOK_NUMBER)) {
                nasm_nonfatal("`%s' expects line increment", dname);
                goto done;
            }
            m = readnum(tok_text(tline), &err);
            if (minus)
                m = -m;
            tline = tline->next;
        }
        tline = skip_white(tline);
        if (tline) {
            if (tline->type == TOK_STRING) {
                if (dname[0] == '#') {
                    /* cpp version: treat double quotes like NASM backquotes */
                    char *txt = tok_text_buf(tline);
                    if (txt[0] == '"') {
                        txt[0] = '`';
                        txt[tline->len - 1] = '`';
                    }
                }
                src_set_fname(unquote_token(tline));
                /*
                 * Anything after the string is ignored by design (for cpp
                 * compatibility and future extensions.)
                 */
            } else {
                char *fname = detoken(tline, false);
                src_set_fname(fname);
                nasm_free(fname);
            }
        }
        src_set_linnum(k);

        istk->where = src_where();
        istk->lineinc = m;
        goto done;
    }

    /*
     * If we're in a non-emitting branch of a condition construct,
     * or walking to the end of an already terminated %rep block,
     * we should ignore all directives except for condition
     * directives.
     */
    if (((istk->conds && !emitting(istk->conds->state)) ||
         (istk->mstk.mstk && !istk->mstk.mstk->in_progress)) &&
        !is_condition(op)) {
        return NO_DIRECTIVE_FOUND;
    }

    /*
     * If we're defining a macro or reading a %rep block, we should
     * ignore all directives except for %macro/%imacro (which nest),
     * %endm/%endmacro, %line and (only if we're in a %rep block) %endrep.
     * If we're in a %rep block, another %rep nests, so should be let through.
     */
    if (defining && op != PP_MACRO && op != PP_RMACRO &&
        op != PP_ENDMACRO && op != PP_ENDM &&
        (defining->name || (op != PP_ENDREP && op != PP_REP))) {
        return NO_DIRECTIVE_FOUND;
    }

    if (defining) {
        if (op == PP_MACRO || op == PP_RMACRO) {
            nested_mac_count++;
            return NO_DIRECTIVE_FOUND;
        } else if (nested_mac_count > 0) {
            if (op == PP_ENDMACRO) {
                nested_mac_count--;
                return NO_DIRECTIVE_FOUND;
            }
        }
        if (!defining->name) {
            if (op == PP_REP) {
                nested_rep_count++;
                return NO_DIRECTIVE_FOUND;
            } else if (nested_rep_count > 0) {
                if (op == PP_ENDREP) {
                    nested_rep_count--;
                    return NO_DIRECTIVE_FOUND;
                }
            }
        }
    }

    switch (op) {
    default:
        nasm_nonfatal("unknown preprocessor directive `%s'", dname);
        return NO_DIRECTIVE_FOUND;      /* didn't get it */

    case PP_PRAGMA:
        /*
         * %pragma namespace options...
         *
         * The namespace "preproc" is reserved for the preprocessor;
         * all other namespaces generate a [pragma] assembly directive.
         *
         * Invalid %pragmas are ignored and may have different
         * meaning in future versions of NASM.
         */
        t = tline;
        tline = tline->next;
        t->next = NULL;
        tline = zap_white(expand_smacro(tline));
        if (tok_type(tline, TOK_ID)) {
            if (!nasm_stricmp(tok_text(tline), "preproc")) {
                /* Preprocessor pragma */
                do_pragma_preproc(tline);
                free_tlist(tline);
            } else {
                /* Build the assembler directive */

                /* Append bracket to the end of the output */
                for (t = tline; t->next; t = t->next)
                    ;
                t->next = make_tok_char(NULL, ']');

                /* Prepend "[pragma " */
                t = new_White(tline);
                t = new_Token(t, TOK_ID, "pragma", 6);
                t = make_tok_char(t, '[');
                tline = t;
                *output = tline;
            }
        }
        break;

    case PP_STACKSIZE:
        /* Directive to tell NASM what the default stack size is. The
         * default is for a 16-bit stack, and this can be overriden with
         * %stacksize large.
         */
        tline = skip_white(tline->next);
        if (!tline || tline->type != TOK_ID) {
            nasm_nonfatal("`%s' missing size parameter", dname);
        }
        if (nasm_stricmp(tok_text(tline), "flat") == 0) {
            /* All subsequent ARG directives are for a 32-bit stack */
            StackSize = 4;
            StackPointer = "ebp";
            ArgOffset = 8;
            LocalOffset = 0;
        } else if (nasm_stricmp(tok_text(tline), "flat64") == 0) {
            /* All subsequent ARG directives are for a 64-bit stack */
            StackSize = 8;
            StackPointer = "rbp";
            ArgOffset = 16;
            LocalOffset = 0;
        } else if (nasm_stricmp(tok_text(tline), "large") == 0) {
            /* All subsequent ARG directives are for a 16-bit stack,
             * far function call.
             */
            StackSize = 2;
            StackPointer = "bp";
            ArgOffset = 4;
            LocalOffset = 0;
        } else if (nasm_stricmp(tok_text(tline), "small") == 0) {
            /* All subsequent ARG directives are for a 16-bit stack,
             * far function call. We don't support near functions.
             */
            StackSize = 2;
            StackPointer = "bp";
            ArgOffset = 6;
            LocalOffset = 0;
        } else {
            nasm_nonfatal("`%s' invalid size type", dname);
        }
        break;

    case PP_ARG:
        /* TASM like ARG directive to define arguments to functions, in
         * the following form:
         *
         *      ARG arg1:WORD, arg2:DWORD, arg4:QWORD
         */
        offset = ArgOffset;
        do {
            const char *arg;
            char directive[256];
            int size = StackSize;

            /* Find the argument name */
            tline = skip_white(tline->next);
            if (!tline || tline->type != TOK_ID) {
                nasm_nonfatal("`%s' missing argument parameter", dname);
                goto done;
            }
            arg = tok_text(tline);

            /* Find the argument size type */
            tline = tline->next;
            if (!tok_is(tline, ':')) {
                nasm_nonfatal("syntax error processing `%s' directive", dname);
                goto done;
            }
            tline = tline->next;
            if (!tok_type(tline, TOK_ID)) {
                nasm_nonfatal("`%s' missing size type parameter", dname);
                goto done;
            }

            /* Allow macro expansion of type parameter */
            tt = tokenize(tok_text(tline));
            tt = expand_smacro(tt);
            size = parse_size(tok_text(tt));
            if (!size) {
                nasm_nonfatal("invalid size type for `%s' missing directive", dname);
                free_tlist(tt);
                goto done;
            }
            free_tlist(tt);

            /* Round up to even stack slots */
            size = ALIGN(size, StackSize);

            /* Now define the macro for the argument */
            snprintf(directive, sizeof(directive), "%%define %s (%s+%d)",
                     arg, StackPointer, offset);
            do_directive(tokenize(directive), output);
            offset += size;

            /* Move to the next argument in the list */
            tline = skip_white(tline->next);
        } while (tok_is(tline, ','));
        ArgOffset = offset;
        break;

    case PP_LOCAL:
        /* TASM like LOCAL directive to define local variables for a
         * function, in the following form:
         *
         *      LOCAL local1:WORD, local2:DWORD, local4:QWORD = LocalSize
         *
         * The '= LocalSize' at the end is ignored by NASM, but is
         * required by TASM to define the local parameter size (and used
         * by the TASM macro package).
         */
        offset = LocalOffset;
        do {
            const char *local;
	    char directive[256];
            int size = StackSize;

            /* Find the argument name */
            tline = skip_white(tline->next);
            if (!tline || tline->type != TOK_ID) {
                nasm_nonfatal("`%s' missing argument parameter", dname);
                goto done;
            }
            local = tok_text(tline);

            /* Find the argument size type */
            tline = tline->next;
            if (!tok_is(tline, ':')) {
                nasm_nonfatal("syntax error processing `%s' directive", dname);
                goto done;
            }
            tline = tline->next;
            if (!tok_type(tline, TOK_ID)) {
                nasm_nonfatal("`%s' missing size type parameter", dname);
                goto done;
            }

            /* Allow macro expansion of type parameter */
            tt = tokenize(tok_text(tline));
            tt = expand_smacro(tt);
            size = parse_size(tok_text(tt));
            if (!size) {
                nasm_nonfatal("invalid size type for `%s' missing directive", dname);
                free_tlist(tt);
                goto done;
            }
            free_tlist(tt);

            /* Round up to even stack slots */
            size = ALIGN(size, StackSize);

            offset += size;     /* Negative offset, increment before */

            /* Now define the macro for the argument */
            snprintf(directive, sizeof(directive), "%%define %s (%s-%d)",
                     local, StackPointer, offset);
            do_directive(tokenize(directive), output);

            /* Now define the assign to setup the enter_c macro correctly */
            snprintf(directive, sizeof(directive),
                     "%%assign %%$localsize %%$localsize+%d", size);
            do_directive(tokenize(directive), output);

            /* Move to the next argument in the list */
            tline = skip_white(tline->next);
        } while (tok_is(tline, ','));
        LocalOffset = offset;
        break;

    case PP_CLEAR:
    {
        bool context = false;

        t = tline->next = expand_smacro(tline->next);
        t = skip_white(t);
        if (!t) {
            /* Emulate legacy behavior */
            do_clear(CLEAR_DEFINE|CLEAR_MMACRO, false);
        } else {
            while ((t = skip_white(t)) && t->type == TOK_ID) {
                const char *txt = tok_text(t);
                if (!nasm_stricmp(txt, "all")) {
                    do_clear(CLEAR_ALL, context);
                } else if (!nasm_stricmp(txt, "define") ||
                           !nasm_stricmp(txt, "def") ||
                           !nasm_stricmp(txt, "smacro")) {
                    do_clear(CLEAR_DEFINE, context);
                } else if (!nasm_stricmp(txt, "defalias") ||
                           !nasm_stricmp(txt, "alias") ||
                           !nasm_stricmp(txt, "salias")) {
                    do_clear(CLEAR_DEFALIAS, context);
                } else if (!nasm_stricmp(txt, "alldef") ||
                           !nasm_stricmp(txt, "alldefine")) {
                    do_clear(CLEAR_ALLDEFINE, context);
                } else if (!nasm_stricmp(txt, "macro") ||
                           !nasm_stricmp(txt, "mmacro")) {
                    do_clear(CLEAR_MMACRO, context);
                } else if (!nasm_stricmp(txt, "context") ||
                           !nasm_stricmp(txt, "ctx")) {
                    context = true;
                } else if (!nasm_stricmp(txt, "global")) {
                    context = false;
                } else if (!nasm_stricmp(txt, "nothing") ||
                         !nasm_stricmp(txt, "none") ||
                         !nasm_stricmp(txt, "ignore") ||
                         !nasm_stricmp(txt, "-") ||
                         !nasm_stricmp(txt, "--")) {
                    /* Do nothing */
                } else {
                    nasm_nonfatal("invalid option to %s: %s", dname, txt);
                    t = NULL;
                }
            }
        }

        t = skip_white(t);
        if (t)
            nasm_warn(WARN_OTHER, "trailing garbage after `%s' ignored", dname);
        break;
    }

    case PP_DEPEND:
        t = tline->next = expand_smacro(tline->next);
        t = skip_white(t);
        if (!t || (t->type != TOK_STRING &&
                   t->type != TOK_INTERNAL_STRING)) {
            nasm_nonfatal("`%s' expects a file name", dname);
            goto done;
        }
        if (t->next)
            nasm_warn(WARN_OTHER, "trailing garbage after `%s' ignored", dname);

        strlist_add(deplist, unquote_token_cstr(t));
        goto done;

    case PP_INCLUDE:
        t = tline->next = expand_smacro(tline->next);
        t = skip_white(t);

        if (!t || (t->type != TOK_STRING &&
                   t->type != TOK_INTERNAL_STRING)) {
            nasm_nonfatal("`%s' expects a file name", dname);
            goto done;
        }
        if (t->next)
            nasm_warn(WARN_OTHER, "trailing garbage after `%s' ignored", dname);
        p = unquote_token_cstr(t);
        nasm_new(inc);
        inc->next = istk;
        found_path = NULL;
        inc->fp = inc_fopen(p, deplist, &found_path,
                            (pp_mode == PP_DEPS)
                            ? INC_OPTIONAL : INC_NEEDED, NF_TEXT);
        if (!inc->fp) {
            /* -MG given but file not found */
            nasm_free(inc);
        } else {
            inc->where = src_where();
            inc->lineinc = 1;
            inc->nolist = istk->nolist;
            inc->noline = istk->noline;
            if (!inc->noline)
                src_set(0, found_path ? found_path : p);
            istk = inc;
            lfmt->uplevel(LIST_INCLUDE, 0);
        }
        break;

    case PP_USE:
    {
        const struct use_package *pkg;
        const char *name;

        pkg = get_use_pkg(tline->next, dname, &name);
        if (!name)
            goto done;
        if (!pkg) {
            nasm_nonfatal("unknown `%s' package: `%s'", dname, name);
        } else if (!use_loaded[pkg->index]) {
            /*
             * Not already included, go ahead and include it.
             * Treat it as an include file for the purpose of
             * producing a listing.
             */
            use_loaded[pkg->index] = true;
            stdmacpos = pkg->macros;
            nasm_new(inc);
            inc->next = istk;
            inc->nolist = istk->nolist + !list_option('b');
            inc->noline = istk->noline;
            if (!inc->noline)
                src_set(0, NULL);
            istk = inc;
            lfmt->uplevel(LIST_INCLUDE, 0);
        }
        break;
    }
    case PP_PUSH:
    case PP_REPL:
    case PP_POP:
        tline = tline->next;
        tline = skip_white(tline);
        tline = expand_id(tline);
        if (tline) {
            if (!tok_type(tline, TOK_ID)) {
                nasm_nonfatal("`%s' expects a context identifier", dname);
                goto done;
            }
            if (tline->next)
                nasm_warn(WARN_OTHER, "trailing garbage after `%s' ignored",
                           dname);
            p = tok_text(tline);
        } else {
            p = NULL; /* Anonymous */
        }

        if (op == PP_PUSH) {
            nasm_new(ctx);
            ctx->depth = cstk ? cstk->depth + 1 : 1;
            ctx->next = cstk;
            ctx->name = p ? nasm_strdup(p) : NULL;
            ctx->number = unique++;
            cstk = ctx;
        } else {
            /* %pop or %repl */
            if (!cstk) {
                nasm_nonfatal("`%s': context stack is empty", dname);
            } else if (op == PP_POP) {
                if (p && (!cstk->name || nasm_stricmp(p, cstk->name)))
                    nasm_nonfatal("`%s' in wrong context: %s, "
                               "expected %s",
                               dname, cstk->name ? cstk->name : "anonymous", p);
                else
                    ctx_pop();
            } else {
                /* op == PP_REPL */
                nasm_free((char *)cstk->name);
                cstk->name = p ? nasm_strdup(p) : NULL;
                p = NULL;
            }
        }
        break;
    case PP_FATAL:
        severity = ERR_FATAL;
        goto issue_error;
    case PP_ERROR:
        severity = ERR_NONFATAL|ERR_PASS2;
        goto issue_error;
    case PP_WARNING:
        /*!
         *!user [on] %warning directives
         *!  controls output of \c{%warning} directives (see \k{pperror}).
         */
        severity = ERR_WARNING|WARN_USER|ERR_PASS2;
        goto issue_error;

issue_error:
    {
        /* Only error out if this is the final pass */
        tline->next = expand_smacro(tline->next);
        tline = tline->next;
        tline = skip_white(tline);
        t = tline ? tline->next : NULL;
        t = skip_white(t);
        if (tok_type(tline, TOK_STRING) && !t) {
            /* The line contains only a quoted string */
            p = unquote_token(tline); /* Ignore NUL character truncation */
            nasm_error(severity, "%s",  p);
        } else {
            /* Not a quoted string, or more than a quoted string */
            q = detoken(tline, false);
            nasm_error(severity, "%s",  q);
            nasm_free(q);
        }
        break;
    }

    CASE_PP_IF:
        if (istk->conds && !emitting(istk->conds->state))
            j = COND_NEVER;
        else {
            j = if_condition(tline->next, op);
            tline->next = NULL; /* it got freed */
        }
        cond = nasm_malloc(sizeof(Cond));
        cond->next = istk->conds;
        cond->state = j;
        istk->conds = cond;
        if(istk->mstk.mstk)
            istk->mstk.mstk->condcnt++;
        break;

    CASE_PP_ELIF:
        if (!istk->conds)
            nasm_fatal("`%s': no matching `%%if'", dname);
        switch(istk->conds->state) {
        case COND_IF_TRUE:
            istk->conds->state = COND_DONE;
            break;

        case COND_DONE:
        case COND_NEVER:
            break;

        case COND_ELSE_TRUE:
        case COND_ELSE_FALSE:
            nasm_warn(WARN_OTHER|ERR_PP_PRECOND,
                       "`%%elif' after `%%else' ignored");
            istk->conds->state = COND_NEVER;
            break;

        case COND_IF_FALSE:
            /*
             * IMPORTANT: In the case of %if, we will already have
             * called expand_mmac_params(); however, if we're
             * processing an %elif we must have been in a
             * non-emitting mode, which would have inhibited
             * the normal invocation of expand_mmac_params().
             * Therefore, we have to do it explicitly here.
             */
            j = if_condition(expand_mmac_params(tline->next), op);
            tline->next = NULL; /* it got freed */
            istk->conds->state = j;
            break;
        }
        break;

    case PP_ELSE:
        if (tline->next)
            nasm_warn(WARN_OTHER|ERR_PP_PRECOND,
                       "trailing garbage after `%%else' ignored");
        if (!istk->conds)
	    nasm_fatal("`%%else: no matching `%%if'");
        switch(istk->conds->state) {
        case COND_IF_TRUE:
        case COND_DONE:
            istk->conds->state = COND_ELSE_FALSE;
            break;

        case COND_NEVER:
            break;

        case COND_IF_FALSE:
            istk->conds->state = COND_ELSE_TRUE;
            break;

        case COND_ELSE_TRUE:
        case COND_ELSE_FALSE:
            nasm_warn(WARN_OTHER|ERR_PP_PRECOND,
                       "`%%else' after `%%else' ignored.");
            istk->conds->state = COND_NEVER;
            break;
        }
        break;

    case PP_ENDIF:
        if (tline->next)
            nasm_warn(WARN_OTHER|ERR_PP_PRECOND,
                       "trailing garbage after `%%endif' ignored");
        if (!istk->conds)
            nasm_fatal("`%%endif': no matching `%%if'");
        cond = istk->conds;
        istk->conds = cond->next;
        nasm_free(cond);
        if(istk->mstk.mstk)
            istk->mstk.mstk->condcnt--;
        break;

    case PP_RMACRO:
    case PP_MACRO:
    {
        MMacro *def;

        nasm_assert(!defining);
        nasm_new(def);
        def->casesense = casesense;
        /*
         * dstk.mstk points to the previous definition bracket,
         * whereas dstk.mmac points to the topmost mmacro, which
         * in this case is the one we are just starting to create.
         */
        def->dstk.mstk = defining;
        def->dstk.mmac = def;
        if (op == PP_RMACRO)
            def->max_depth = nasm_limit[LIMIT_MACRO_LEVELS];
        if (!parse_mmacro_spec(tline, def, dname)) {
            nasm_free(def);
            goto done;
        }

        defining = def;
        defining->where = istk->where;

        mmac = (MMacro *) hash_findix(&mmacros, defining->name);
        while (mmac) {
            if (!strcmp(mmac->name, defining->name) &&
                (mmac->nparam_min <= defining->nparam_max
                 || defining->plus)
                && (defining->nparam_min <= mmac->nparam_max
                    || mmac->plus)) {
                nasm_warn(WARN_OTHER, "redefining multi-line macro `%s'",
                           defining->name);
                break;
            }
            mmac = mmac->next;
        }
        break;
    }

    case PP_ENDM:
    case PP_ENDMACRO:
        if (!(defining && defining->name)) {
            nasm_nonfatal("`%s': not defining a macro", tok_text(tline));
            goto done;
        }
        mmhead = (MMacro **) hash_findi_add(&mmacros, defining->name);
        defining->next = *mmhead;
        *mmhead = defining;
        defining = NULL;
        break;

    case PP_EXITMACRO:
        /*
         * We must search along istk->expansion until we hit a
         * macro-end marker for a macro with a name. Then we
         * bypass all lines between exitmacro and endmacro.
         */
        list_for_each(l, istk->expansion)
            if (l->finishes && l->finishes->name)
                break;

        if (l) {
            /*
             * Remove all conditional entries relative to this
             * macro invocation. (safe to do in this context)
             */
            for ( ; l->finishes->condcnt > 0; l->finishes->condcnt --) {
                cond = istk->conds;
                istk->conds = cond->next;
                nasm_free(cond);
            }
            istk->expansion = l;
        } else {
            nasm_nonfatal("`%%exitmacro' not within `%%macro' block");
        }
        break;

    case PP_UNIMACRO:
        casesense = false;
        /* fall through */
    case PP_UNMACRO:
    {
        MMacro **mmac_p;
        MMacro spec;

        nasm_zero(spec);
        spec.casesense = casesense;
        if (!parse_mmacro_spec(tline, &spec, dname)) {
            goto done;
        }
        mmac_p = (MMacro **) hash_findi(&mmacros, spec.name, NULL);
        while (mmac_p && *mmac_p) {
            mmac = *mmac_p;
            if (mmac->casesense == spec.casesense &&
                !mstrcmp(mmac->name, spec.name, spec.casesense) &&
                mmac->nparam_min == spec.nparam_min &&
                mmac->nparam_max == spec.nparam_max &&
                mmac->plus == spec.plus) {
                *mmac_p = mmac->next;
                free_mmacro(mmac);
            } else {
                mmac_p = &mmac->next;
            }
        }
        free_tlist(spec.dlist);
        break;
    }

    case PP_ROTATE:
        while (tok_white(tline->next))
            tline = tline->next;
        if (!tline->next) {
            free_tlist(origline);
            nasm_nonfatal("`%%rotate' missing rotate count");
            return DIRECTIVE_FOUND;
        }
        t = expand_smacro(tline->next);
        tline->next = NULL;
        pps.tptr = tline = t;
	pps.ntokens = -1;
        tokval.t_type = TOKEN_INVALID;
        evalresult =
            evaluate(ppscan, &pps, &tokval, NULL, true, NULL);
        free_tlist(tline);
        if (!evalresult)
            return DIRECTIVE_FOUND;
        if (tokval.t_type)
            nasm_warn(WARN_OTHER, "trailing garbage after expression ignored");
        if (!is_simple(evalresult)) {
            nasm_nonfatal("non-constant value given to `%%rotate'");
            return DIRECTIVE_FOUND;
        }
        mmac = istk->mstk.mmac;
        if (!mmac) {
            nasm_nonfatal("`%%rotate' invoked outside a macro call");
        } else if (mmac->nparam == 0) {
            nasm_nonfatal("`%%rotate' invoked within macro without parameters");
        } else {
            int rotate = mmac->rotate + reloc_value(evalresult);

            rotate %= (int)mmac->nparam;
            if (rotate < 0)
                rotate += mmac->nparam;

            mmac->rotate = rotate;
        }
        break;

    case PP_REP:
    {
        MMacro *tmp_defining;

        nolist = 0;
        tline = skip_white(tline->next);
        if (tok_type(tline, TOK_ID) && tline->len == 7 &&
	    !nasm_memicmp(tline->text.a, ".nolist", 7)) {
            if (!list_option('f'))
                nolist |= NL_LIST; /* ... but update line numbers */
            tline = skip_white(tline->next);
        }

        if (tline) {
            pps.tptr = expand_smacro(tline);
	    pps.ntokens = -1;
            tokval.t_type = TOKEN_INVALID;
            /* XXX: really critical?! */
            evalresult =
                evaluate(ppscan, &pps, &tokval, NULL, true, NULL);
            if (!evalresult)
                goto done;
            if (tokval.t_type)
                nasm_warn(WARN_OTHER, "trailing garbage after expression ignored");
            if (!is_simple(evalresult)) {
                nasm_nonfatal("non-constant value given to `%%rep'");
                goto done;
            }
            count = reloc_value(evalresult);
            if (count > nasm_limit[LIMIT_REP]) {
                nasm_nonfatal("`%%rep' count %"PRId64" exceeds limit (currently %"PRId64")",
                              count, nasm_limit[LIMIT_REP]);
                count = 0;
            } else if (count < 0) {
                /*!
                 *!negative-rep [on] regative %rep count
                 *!  warns about negative counts given to the \c{%rep}
                 *!  preprocessor directive.
                 */
                nasm_warn(ERR_PASS2|WARN_NEGATIVE_REP,
                           "negative `%%rep' count: %"PRId64, count);
                count = 0;
            } else {
                count++;
            }
        } else {
            nasm_nonfatal("`%%rep' expects a repeat count");
            count = 0;
        }
        tmp_defining = defining;
        nasm_new(defining);
        defining->nolist = nolist;
        defining->in_progress = count;
        defining->mstk = istk->mstk;
        defining->dstk.mstk = tmp_defining;
        defining->dstk.mmac = tmp_defining ? tmp_defining->dstk.mmac : NULL;
        defining->where = istk->where;
        break;
    }

    case PP_ENDREP:
        if (!defining || defining->name) {
            nasm_nonfatal("`%%endrep': no matching `%%rep'");
            goto done;
        }

        /*
         * Now we have a "macro" defined - although it has no name
         * and we won't be entering it in the hash tables - we must
         * push a macro-end marker for it on to istk->expansion.
         * After that, it will take care of propagating itself (a
         * macro-end marker line for a macro which is really a %rep
         * block will cause the macro to be re-expanded, complete
         * with another macro-end marker to ensure the process
         * continues) until the whole expansion is forcibly removed
         * from istk->expansion by a %exitrep.
         */
        nasm_new(l);
        l->next = istk->expansion;
        l->finishes = defining;
        l->first = NULL;
        l->where = src_where();
        istk->expansion = l;

        istk->mstk.mstk = defining;

        /* A loop does not change istk->noline */
        istk->nolist += !!(defining->nolist & NL_LIST);
        if (!istk->nolist)
            lfmt->uplevel(LIST_MACRO, 0);

        defining = defining->dstk.mstk;
        break;

    case PP_EXITREP:
        /*
         * We must search along istk->expansion until we hit a
         * macro-end marker for a macro with no name. Then we set
         * its `in_progress' flag to 0.
         */
        list_for_each(l, istk->expansion)
            if (l->finishes && !l->finishes->name)
                break;

        if (l)
            l->finishes->in_progress = 0;
        else
            nasm_nonfatal("`%%exitrep' not within `%%rep' block");
        break;

    case PP_DEFINE:
    case PP_XDEFINE:
    case PP_DEFALIAS:
    {
        SMacro tmpl;
        Token **lastp;
        int nparam;

        if (!(mname = get_id(&tline, dname)))
            goto done;

        nasm_zero(tmpl);
        lastp = &tline->next;
        nparam = parse_smacro_template(&lastp, &tmpl);
        tline = *lastp;
        *lastp = NULL;

        if (unlikely(op == PP_DEFALIAS)) {
            macro_start = tline;
            if (!is_macro_id(macro_start)) {
                nasm_nonfatal("`%s' expects a macro identifier to alias",
                              dname);
                goto done;
            }
            tt = macro_start->next;
            macro_start->next = NULL;
            tline = tline->next;
            tline = skip_white(tline);
            if (tline && tline->type) {
                nasm_warn(WARN_OTHER,
                          "trailing garbage after aliasing identifier ignored");
            }
            free_tlist(tt);
            tmpl.alias = true;
        } else {
            if (op == PP_XDEFINE) {
                /* Protect macro parameter tokens */
                if (nparam)
                    mark_smac_params(tline, &tmpl, TOK_XDEF_PARAM);
                tline = expand_smacro(tline);
            }
            /* NB: Does this still make sense? */
            macro_start = reverse_tokens(tline);
        }

        /*
         * Good. We now have a macro name, a parameter count, and a
         * token list (in reverse order) for an expansion. We ought
         * to be OK just to create an SMacro, store it, and let
         * free_tlist have the rest of the line (which we have
         * carefully re-terminated after chopping off the expansion
         * from the end).
         */
        define_smacro(mname, casesense, macro_start, &tmpl);
        break;
    }

    case PP_UNDEF:
    case PP_UNDEFALIAS:
        if (!(mname = get_id(&tline, dname)))
            goto done;
        if (tline->next)
            nasm_warn(WARN_OTHER, "trailing garbage after macro name ignored");

        undef_smacro(mname, op == PP_UNDEFALIAS);
        break;

    case PP_DEFSTR:
        if (!(mname = get_id(&tline, dname)))
            goto done;

        last = tline;
        tline = expand_smacro(tline->next);
        last->next = NULL;

        tline = zap_white(tline);
        q = detoken(tline, false);
        macro_start = make_tok_qstr(NULL, q);
        nasm_free(q);

        /*
         * We now have a macro name, an implicit parameter count of
         * zero, and a string token to use as an expansion. Create
         * and store an SMacro.
         */
        define_smacro(mname, casesense, macro_start, NULL);
        break;

    case PP_DEFTOK:
        if (!(mname = get_id(&tline, dname)))
            goto done;

        last = tline;
        tline = expand_smacro(tline->next);
        last->next = NULL;

        t = skip_white(tline);
        /* t should now point to the string */
        if (!tok_type(t, TOK_STRING)) {
            nasm_nonfatal("`%s' requires string as second parameter", dname);
            free_tlist(tline);
            goto done;
        }

        /*
         * Convert the string to a token stream.  Note that smacros
         * are stored with the token stream reversed, so we have to
         * reverse the output of tokenize().
         */
        macro_start = reverse_tokens(tokenize(unquote_token_cstr(t)));

        /*
         * We now have a macro name, an implicit parameter count of
         * zero, and a numeric token to use as an expansion. Create
         * and store an SMacro.
         */
        define_smacro(mname, casesense, macro_start, NULL);
        free_tlist(tline);
        break;

    case PP_PATHSEARCH:
    {
        const char *found_path;

        if (!(mname = get_id(&tline, dname)))
            goto done;

        last = tline;
        tline = expand_smacro(tline->next);
        last->next = NULL;

        t = skip_white(tline);
        if (!t || (t->type != TOK_STRING &&
                   t->type != TOK_INTERNAL_STRING)) {
            nasm_nonfatal("`%s' expects a file name", dname);
            free_tlist(tline);
            goto done;
        }
        if (t->next)
            nasm_warn(WARN_OTHER, "trailing garbage after `%s' ignored", dname);

	p = unquote_token_cstr(t);

        inc_fopen(p, NULL, &found_path, INC_PROBE, NF_BINARY);
        if (!found_path)
            found_path = p;
	macro_start = make_tok_qstr(NULL, found_path);

        /*
         * We now have a macro name, an implicit parameter count of
         * zero, and a string token to use as an expansion. Create
         * and store an SMacro.
         */
        define_smacro(mname, casesense, macro_start, NULL);
        free_tlist(tline);
        break;
    }

    case PP_STRLEN:
        if (!(mname = get_id(&tline, dname)))
            goto done;

        last = tline;
        tline = expand_smacro(tline->next);
        last->next = NULL;

        t = skip_white(tline);
        /* t should now point to the string */
        if (!tok_type(t, TOK_STRING)) {
            nasm_nonfatal("`%s' requires string as second parameter", dname);
            free_tlist(tline);
            free_tlist(origline);
            return DIRECTIVE_FOUND;
        }

	unquote_token(t);
        macro_start = make_tok_num(NULL, t->len);

        /*
         * We now have a macro name, an implicit parameter count of
         * zero, and a numeric token to use as an expansion. Create
         * and store an SMacro.
         */
        define_smacro(mname, casesense, macro_start, NULL);
        free_tlist(tline);
        free_tlist(origline);
        return DIRECTIVE_FOUND;

    case PP_STRCAT:
        if (!(mname = get_id(&tline, dname)))
            goto done;

        last = tline;
        tline = expand_smacro(tline->next);
        last->next = NULL;

        len = 0;
        list_for_each(t, tline) {
            switch (t->type) {
            case TOK_WHITESPACE:
                break;
            case TOK_STRING:
		unquote_token(t);
                len += t->len;
                break;
            case TOK_OTHER:
                if (tok_is(t, ',')) /* permit comma separators */
                    break;
                /* else fall through */
            default:
                nasm_nonfatal("non-string passed to `%s': %s", dname,
			      tok_text(t));
                free_tlist(tline);
                goto done;
            }
        }

        q = qbuf = nasm_malloc(len+1);
        list_for_each(t, tline) {
            if (t->type == TOK_INTERNAL_STRING)
                q = mempcpy(q, tok_text(t), t->len);
        }
        *q = '\0';

        /*
         * We now have a macro name, an implicit parameter count of
         * zero, and a numeric token to use as an expansion. Create
         * and store an SMacro.
         */
        macro_start = make_tok_qstr_len(NULL, qbuf, len);
        nasm_free(qbuf);
        define_smacro(mname, casesense, macro_start, NULL);
        free_tlist(tline);
        break;

    case PP_SUBSTR:
    {
        int64_t start, count;
	const char *txt;
        size_t len;

        if (!(mname = get_id(&tline, dname)))
            goto done;

        last = tline;
        tline = expand_smacro(tline->next);
        last->next = NULL;

        if (tline) /* skip expanded id */
            t = tline->next;

        t = skip_white(t);

        /* t should now point to the string */
        if (!tok_type(t, TOK_STRING)) {
            nasm_nonfatal("`%s' requires string as second parameter", dname);
            free_tlist(tline);
            goto done;
        }

        pps.tptr = t->next;
	pps.ntokens = -1;
        tokval.t_type = TOKEN_INVALID;
        evalresult = evaluate(ppscan, &pps, &tokval, NULL, true, NULL);
        if (!evalresult) {
            free_tlist(tline);
            goto done;
        } else if (!is_simple(evalresult)) {
            nasm_nonfatal("non-constant value given to `%s'", dname);
            free_tlist(tline);
            goto done;
        }
        start = evalresult->value - 1;

        pps.tptr = skip_white(pps.tptr);
        if (!pps.tptr) {
            count = 1;  /* Backwards compatibility: one character */
        } else {
            tokval.t_type = TOKEN_INVALID;
            evalresult = evaluate(ppscan, &pps, &tokval, NULL, true, NULL);
            if (!evalresult) {
                free_tlist(tline);
                goto done;
            } else if (!is_simple(evalresult)) {
                nasm_nonfatal("non-constant value given to `%s'", dname);
                free_tlist(tline);
                goto done;
            }
            count = evalresult->value;
        }

	unquote_token(t);
        len = t->len;

        /* make start and count being in range */
        if (start < 0)
            start = 0;
        if (count < 0)
            count = len + count + 1 - start;
        if (start + count > (int64_t)len)
            count = len - start;
        if (!len || count < 0 || start >=(int64_t)len)
            start = -1, count = 0; /* empty string */

	txt = (start < 0) ? "" : tok_text(t) + start;
	len = count;
        macro_start = make_tok_qstr_len(NULL, txt, len);

        /*
         * We now have a macro name, an implicit parameter count of
         * zero, and a numeric token to use as an expansion. Create
         * and store an SMacro.
         */
        define_smacro(mname, casesense, macro_start, NULL);
        free_tlist(tline);
        break;
    }

    case PP_ASSIGN:
        if (!(mname = get_id(&tline, dname)))
            goto done;

        last = tline;
        tline = expand_smacro(tline->next);
        last->next = NULL;

        pps.tptr = tline;
	pps.ntokens = -1;
        tokval.t_type = TOKEN_INVALID;
        evalresult = evaluate(ppscan, &pps, &tokval, NULL, true, NULL);
        free_tlist(tline);
        if (!evalresult)
            goto done;

        if (tokval.t_type)
            nasm_warn(WARN_OTHER, "trailing garbage after expression ignored");

        if (!is_simple(evalresult)) {
            nasm_nonfatal("non-constant value given to `%s'", dname);
            free_tlist(origline);
            return DIRECTIVE_FOUND;
	}

	macro_start = make_tok_num(NULL, reloc_value(evalresult));

        /*
         * We now have a macro name, an implicit parameter count of
         * zero, and a numeric token to use as an expansion. Create
         * and store an SMacro.
         */
        define_smacro(mname, casesense, macro_start, NULL);
        break;

    case PP_ALIASES:
        tline = tline->next;
        tline = expand_smacro(tline);
        ppopt.noaliases = !pp_get_boolean_option(tline, !ppopt.noaliases);
        break;

    case PP_LINE:
        nasm_panic("`%s' directive not preprocessed early", dname);
        break;

    case PP_NULL:
        /* Goes nowhere, does nothing... */
        break;

    }

done:
    free_tlist(origline);
    return DIRECTIVE_FOUND;
}