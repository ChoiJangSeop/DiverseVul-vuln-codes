swproc(gs_main_instance * minst, const char *arg, arg_list * pal)
{
    char sw = arg[1];
    ref vtrue;
    int code = 0;
#undef initial_enter_name
#define initial_enter_name(nstr, pvalue)\
  i_initial_enter_name(minst->i_ctx_p, nstr, pvalue)

    make_true(&vtrue);
    arg += 2;                   /* skip - and letter */
    switch (sw) {
        default:
            return 1;
        case 0:         /* read stdin as a file char-by-char */
            /* This is a ******HACK****** for Ghostview. */
            minst->heap->gs_lib_ctx->stdin_is_interactive = true;
            goto run_stdin;
        case '_':       /* read stdin with normal buffering */
            minst->heap->gs_lib_ctx->stdin_is_interactive = false;
run_stdin:
            minst->run_start = false;   /* don't run 'start' */
            /* Set NOPAUSE so showpage won't try to read from stdin. */
            code = swproc(minst, "-dNOPAUSE", pal);
            if (code)
                return code;
            code = gs_main_init2(minst);        /* Finish initialization */
            if (code < 0)
                return code;

            code = run_string(minst, ".runstdin", runFlush);
            if (code < 0)
                return code;
            if (minst->saved_pages_test_mode) {
                gx_device *pdev;

                /* get the current device */
                pdev = gs_currentdevice(minst->i_ctx_p->pgs);
                if ((code = gx_saved_pages_param_process((gx_device_printer *)pdev,
                           (byte *)"print normal flush", 18)) < 0)
                    return code;
                if (code > 0)
                    if ((code = gs_erasepage(minst->i_ctx_p->pgs)) < 0)
                        return code;
            }
            break;
        case '-':               /* run with command line args */
            if (strncmp(arg, "debug=", 6) == 0) {
                code = gs_debug_flags_parse(minst->heap, arg+6);
                if (code < 0)
                    return code;
                break;
            } else if (strncmp(arg, "saved-pages=", 12) == 0) {
                gx_device *pdev;
                gx_device_printer *ppdev;

                /* If init2 not yet done, just save the argument for processing then */
                if (minst->init_done < 2) {
                    if (minst->saved_pages_initial_arg == NULL) {
                        /* Tuck the parameters away for later when init2 is done */
                        minst->saved_pages_initial_arg = (char *)arg+12;
                    } else {
                        outprintf(minst->heap,
                                  "   Only one --saved-pages=... command allowed before processing input\n");
                        arg_finit(pal);
                        return e_Fatal;
                    }
                } else {
                    /* get the current device */
                    pdev = gs_currentdevice(minst->i_ctx_p->pgs);
                    if (dev_proc(pdev, dev_spec_op)(pdev, gxdso_supports_saved_pages, NULL, 0) == 0) {
                        outprintf(minst->heap,
                                  "   --saved-pages not supported by the '%s' device.\n",
                                  pdev->dname);
                        arg_finit(pal);
                        return e_Fatal;
                    }
                    ppdev = (gx_device_printer *)pdev;
                    code = gx_saved_pages_param_process(ppdev, (byte *)arg+12, strlen(arg+12));
                    if (code > 0)
                        if ((code = gs_erasepage(minst->i_ctx_p->pgs)) < 0)
                            return code;
                }
                break;
            /* The following code is only to allow regression testing of saved-pages */
            } else if (strncmp(arg, "saved-pages-test", 16) == 0) {
                minst->saved_pages_test_mode = true;
                break;
            }
            /* FALLTHROUGH */
        case '+':
            pal->expand_ats = false;
            /* FALLTHROUGH */
        case '@':               /* ditto with @-expansion */
            {
                const char *psarg = arg_next(pal, &code, minst->heap);

                if (code < 0)
                    return e_Fatal;
                if (psarg == 0) {
                    outprintf(minst->heap, "Usage: gs ... -%c file.ps arg1 ... argn\n", sw);
                    arg_finit(pal);
                    return e_Fatal;
                }
                psarg = arg_copy(psarg, minst->heap);
                if (psarg == NULL)
                    code = e_Fatal;
                else
                    code = gs_main_init2(minst);
                if (code >= 0)
                    code = run_string(minst, "userdict/ARGUMENTS[", 0);
                if (code >= 0)
                    while ((arg = arg_next(pal, &code, minst->heap)) != 0) {
                        code = runarg(minst, "", arg, "", runInit);
                        if (code < 0)
                            break;
                    }
                if (code >= 0)
                    code = runarg(minst, "]put", psarg, ".runfile", runInit | runFlush);
                arg_free((char *)psarg, minst->heap);
                if (code >= 0)
                    code = e_Quit;
                
                return code;
            }
        case 'A':               /* trace allocator */
            switch (*arg) {
                case 0:
                    gs_alloc_debug = 1;
                    break;
                case '-':
                    gs_alloc_debug = 0;
                    break;
                default:
                    puts(minst->heap, "-A may only be followed by -");
                    return e_Fatal;
            }
            break;
        case 'B':               /* set run_string buffer size */
            if (*arg == '-')
                minst->run_buffer_size = 0;
            else {
                uint bsize;

                if (sscanf((const char *)arg, "%u", &bsize) != 1 ||
                    bsize <= 0 || bsize > MAX_BUFFERED_SIZE
                    ) {
                    outprintf(minst->heap,
                              "-B must be followed by - or size between 1 and %u\n",
                              MAX_BUFFERED_SIZE);
                    return e_Fatal;
                }
                minst->run_buffer_size = bsize;
            }
            break;
        case 'c':               /* code follows */
            {
                bool ats = pal->expand_ats;

                code = gs_main_init2(minst);
                if (code < 0)
                    return code;
                pal->expand_ats = false;
                while ((arg = arg_next(pal, &code, minst->heap)) != 0) {
                    if (arg[0] == '@' ||
                        (arg[0] == '-' && !isdigit((unsigned char)arg[1]))
                        )
                        break;
                    code = runarg(minst, "", arg, ".runstring", runBuffer);
                    if (code < 0)
                        return code;
                }
                if (code < 0)
                    return e_Fatal;
                if (arg != 0) {
                    char *p = arg_copy(arg, minst->heap);
                    if (p == NULL)
                        return e_Fatal;
                    arg_push_string(pal, p, true);
                }
                pal->expand_ats = ats;
                break;
            }
        case 'E':               /* log errors */
            switch (*arg) {
                case 0:
                    gs_log_errors = 1;
                    break;
                case '-':
                    gs_log_errors = 0;
                    break;
                default:
                    puts(minst->heap, "-E may only be followed by -");
                    return e_Fatal;
            }
            break;
        case 'f':               /* run file of arbitrary name */
            if (*arg != 0) {
                code = argproc(minst, arg);
                if (code < 0)
                    return code;
                if (minst->saved_pages_test_mode) {
                    gx_device *pdev;

                    /* get the current device */
                    pdev = gs_currentdevice(minst->i_ctx_p->pgs);
                        return code;
                    if ((code = gx_saved_pages_param_process((gx_device_printer *)pdev,
                               (byte *)"print normal flush", 18)) < 0)
                        return code;
                    if (code > 0)
                        if ((code = gs_erasepage(minst->i_ctx_p->pgs)) < 0)
                            return code;
                }
            }
            break;
        case 'F':               /* run file with buffer_size = 1 */
            if (!*arg) {
                puts(minst->heap, "-F requires a file name");
                return e_Fatal;
            } {
                uint bsize = minst->run_buffer_size;

                minst->run_buffer_size = 1;
                code = argproc(minst, arg);
                minst->run_buffer_size = bsize;
                if (code < 0)
                    return code;
                if (minst->saved_pages_test_mode) {
                    gx_device *pdev;

                    /* get the current device */
                    pdev = gs_currentdevice(minst->i_ctx_p->pgs);
                    if ((code = gx_saved_pages_param_process((gx_device_printer *)pdev,
                               (byte *)"print normal flush", 18)) < 0)
                        return code;
                    if (code > 0)
                        if ((code = gs_erasepage(minst->i_ctx_p->pgs)) < 0)
                            return code;
                }
            }
            break;
        case 'g':               /* define device geometry */
            {
                long width, height;
                ref value;

                if ((code = gs_main_init1(minst)) < 0)
                    return code;
                if (sscanf((const char *)arg, "%ldx%ld", &width, &height) != 2) {
                    puts(minst->heap, "-g must be followed by <width>x<height>");
                    return e_Fatal;
                }
                make_int(&value, width);
                initial_enter_name("DEVICEWIDTH", &value);
                make_int(&value, height);
                initial_enter_name("DEVICEHEIGHT", &value);
                initial_enter_name("FIXEDMEDIA", &vtrue);
                break;
            }
        case 'h':               /* print help */
        case '?':               /* ditto */
            print_help(minst);
            return e_Info;      /* show usage info on exit */
        case 'I':               /* specify search path */
            {
                const char *path;

                if (arg[0] == 0) {
                    path = arg_next(pal, &code, minst->heap);
                    if (code < 0)
                        return code;
                } else
                    path = arg;
                if (path == NULL)
                    return e_Fatal;
                path = arg_copy(path, minst->heap);
                if (path == NULL)
                    return e_Fatal;
                gs_main_add_lib_path(minst, path);
            }
            break;
        case 'K':               /* set malloc limit */
            {
                long msize = 0;
                gs_malloc_memory_t *rawheap = gs_malloc_wrapped_contents(minst->heap);

                sscanf((const char *)arg, "%ld", &msize);
                if (msize <= 0 || msize > max_long >> 10) {
                    outprintf(minst->heap, "-K<numK> must have 1 <= numK <= %ld\n",
                              max_long >> 10);
                    return e_Fatal;
                }
                rawheap->limit = msize << 10;
            }
            break;
        case 'M':               /* set memory allocation increment */
            {
                unsigned msize = 0;

                sscanf((const char *)arg, "%u", &msize);
#if ARCH_INTS_ARE_SHORT
                if (msize <= 0 || msize >= 64) {
                    puts(minst->heap, "-M must be between 1 and 63");
                    return e_Fatal;
                }
#endif
                minst->memory_chunk_size = msize << 10;
            }
            break;
        case 'N':               /* set size of name table */
            {
                unsigned nsize = 0;

                sscanf((const char *)arg, "%d", &nsize);
#if ARCH_INTS_ARE_SHORT
                if (nsize < 2 || nsize > 64) {
                    puts(minst->heap, "-N must be between 2 and 64");
                    return e_Fatal;
                }
#endif
                minst->name_table_size = (ulong) nsize << 10;
            }
            break;
        case 'o':               /* set output file name and batch mode */
            {
                const char *adef;
                char *str;
                ref value;
                int len;

                if (arg[0] == 0) {
                    adef = arg_next(pal, &code, minst->heap);
                    if (code < 0)
                        return code;
                } else
                    adef = arg;
                if ((code = gs_main_init1(minst)) < 0)
                    return code;
                len = strlen(adef);
                str = (char *)gs_alloc_bytes(minst->heap, (uint)len, "-o");
                if (str == NULL)
                    return e_VMerror;
                memcpy(str, adef, len);
                make_const_string(&value, a_readonly | avm_foreign,
                                  len, (const byte *)str);
                initial_enter_name("OutputFile", &value);
                initial_enter_name("NOPAUSE", &vtrue);
                initial_enter_name("BATCH", &vtrue);
            }
            break;
        case 'P':               /* choose whether search '.' first */
            if (!strcmp(arg, ""))
                minst->search_here_first = true;
            else if (!strcmp(arg, "-"))
                minst->search_here_first = false;
            else {
                puts(minst->heap, "Only -P or -P- is allowed.");
                return e_Fatal;
            }
            break;
        case 'q':               /* quiet startup */
            if ((code = gs_main_init1(minst)) < 0)
                return code;
            initial_enter_name("QUIET", &vtrue);
            break;
        case 'r':               /* define device resolution */
            {
                float xres, yres;
                ref value;

                if ((code = gs_main_init1(minst)) < 0)
                    return code;
                switch (sscanf((const char *)arg, "%fx%f", &xres, &yres)) {
                    default:
                        puts(minst->heap, "-r must be followed by <res> or <xres>x<yres>");
                        return e_Fatal;
                    case 1:     /* -r<res> */
                        yres = xres;
                    case 2:     /* -r<xres>x<yres> */
                        make_real(&value, xres);
                        initial_enter_name("DEVICEXRESOLUTION", &value);
                        make_real(&value, yres);
                        initial_enter_name("DEVICEYRESOLUTION", &value);
                        initial_enter_name("FIXEDRESOLUTION", &vtrue);
                }
                break;
            }
        case 'D':               /* define name */
        case 'd':
        case 'S':               /* define name as string */
        case 's':
            {
                char *adef = arg_copy(arg, minst->heap);
                char *eqp;
                bool isd = (sw == 'D' || sw == 'd');
                ref value;

                if (adef == NULL)
                    return e_Fatal;
                eqp = strchr(adef, '=');

                if (eqp == NULL)
                    eqp = strchr(adef, '#');
                /* Initialize the object memory, scanner, and */
                /* name table now if needed. */
                if ((code = gs_main_init1(minst)) < 0)
                    return code;
                if (eqp == adef) {
                    puts(minst->heap, "Usage: -dNAME, -dNAME=TOKEN, -sNAME=STRING");
                    return e_Fatal;
                }
                if (eqp == NULL) {
                    if (isd)
                        make_true(&value);
                    else
                        make_empty_string(&value, a_readonly);
                } else {
                    int code;
                    i_ctx_t *i_ctx_p = minst->i_ctx_p;
                    uint space = icurrent_space;

                    *eqp++ = 0;
                    ialloc_set_space(idmemory, avm_system);
                    if (isd) {
                        int num, i;

                        /* Check for numbers so we can provide for suffix scalers */
                        /* Note the check for '#' is for PS "radix" numbers such as 16#ff */
                        /* and check for '.' and 'e' or 'E' which are 'real' numbers */
                        if ((strchr(eqp, '#') == NULL) && (strchr(eqp, '.') == NULL) &&
                            (strchr(eqp, 'e') == NULL) && (strchr(eqp, 'E') == NULL) &&
                            ((i = sscanf((const char *)eqp, "%d", &num)) == 1)) {
                            char suffix = eqp[strlen(eqp) - 1];

                            switch (suffix) {
                                case 'k':
                                case 'K':
                                    num *= 1024;
                                    break;
                                case 'm':
                                case 'M':
                                    num *= 1024 * 1024;
                                    break;
                                case 'g':
                                case 'G':
                                    /* caveat emptor: more than 2g will overflow */
                                    /* and really should produce a 'real', so don't do this */
                                    num *= 1024 * 1024 * 1024;
                                    break;
                                default:
                                    break;   /* not a valid suffix or last char was digit */
                            }
                            make_int(&value, num);
                        } else {
                            /* use the PS scanner to capture other valid token types */
                            stream astream;
                            scanner_state state;

                            s_init(&astream, NULL);
                            sread_string(&astream,
                                         (const byte *)eqp, strlen(eqp));
                            gs_scanner_init_stream(&state, &astream);
                            code = gs_scan_token(minst->i_ctx_p, &value, &state);
                            if (code) {
                                outprintf(minst->heap, "Invalid value for option -d%s, -dNAME= must be followed by a valid token\n", arg);
                                return e_Fatal;
                            }
                            if (r_has_type_attrs(&value, t_name,
                                                 a_executable)) {
                                ref nsref;

                                name_string_ref(minst->heap, &value, &nsref);
#undef string_is
#define string_is(nsref, str, len)\
       (r_size(&(nsref)) == (len) &&\
       !strncmp((const char *)(nsref).value.const_bytes, str, (len)))
                                if (string_is(nsref, "null", 4))
                                    make_null(&value);
                                else if (string_is(nsref, "true", 4))
                                    make_true(&value);
                                else if (string_is(nsref, "false", 5))
                                    make_false(&value);
                                else {
                                    outprintf(minst->heap, "Invalid value for option -d%s, use -sNAME= to define string constants\n", arg);
                                    return e_Fatal;
                                }
                            }
                        }
                    } else {
                        int len = strlen(eqp);
                        char *str =
                        (char *)gs_alloc_bytes(minst->heap,
                                               (uint) len, "-s");

                        if (str == 0) {
                            lprintf("Out of memory!\n");
                            return e_Fatal;
                        }
                        memcpy(str, eqp, len);
                        make_const_string(&value,
                                          a_readonly | avm_foreign,
                                          len, (const byte *)str);
                        if ((code = try_stdout_redirect(minst, adef, eqp)) < 0)
                            return code;
                    }
                    ialloc_set_space(idmemory, space);
                }
                /* Enter the name in systemdict. */
                initial_enter_name(adef, &value);
                break;
            }
        case 'T':
            set_debug_flags(arg, vd_flags);
            break;
        case 'u':               /* undefine name */
            if (!*arg) {
                puts(minst->heap, "-u requires a name to undefine.");
                return e_Fatal;
            }
            if ((code = gs_main_init1(minst)) < 0)
                return code;
            i_initial_remove_name(minst->i_ctx_p, arg);
            break;
        case 'v':               /* print revision */
            print_revision(minst);
            return e_Info;
/*#ifdef DEBUG */
            /*
             * Here we provide a place for inserting debugging code that can be
             * run in place of the normal interpreter code.
             */
        case 'X':
            code = gs_main_init2(minst);
            if (code < 0)
                return code;
            {
                int xec;        /* exit_code */
                ref xeo;        /* error_object */

#define start_x()\
  gs_main_run_string_begin(minst, 1, &xec, &xeo)
#define run_x(str)\
  gs_main_run_string_continue(minst, str, strlen(str), 1, &xec, &xeo)
#define stop_x()\
  gs_main_run_string_end(minst, 1, &xec, &xeo)
                start_x();
                run_x("\216\003abc");
                run_x("== flush\n");
                stop_x();
            }
            return e_Quit;
/*#endif */
        case 'Z':
            set_debug_flags(arg, gs_debug);
            break;
    }
    return 0;
}