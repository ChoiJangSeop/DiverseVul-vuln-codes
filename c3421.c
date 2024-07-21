yyparse (void)
{
    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       'yyss': related to states.
       'yyvs': related to semantic values.

       Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yyssp = yyss = yyssa;
  yyvsp = yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */
  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        YYSTYPE *yyvs1 = yyvs;
        yytype_int16 *yyss1 = yyss;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * sizeof (*yyssp),
                    &yyvs1, yysize * sizeof (*yyvsp),
                    &yystacksize);

        yyss = yyss1;
        yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yytype_int16 *yyss1 = yyss;
        union yyalloc *yyptr =
          (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
        if (! yyptr)
          goto yyexhaustedlab;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
                  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 5:
#line 373 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			/* I will need to incorporate much more fine grained
			 * error messages. The following should suffice for
			 * the time being.
			 */
			struct FILE_INFO * ip_ctx = lex_current();
			msyslog(LOG_ERR,
				"syntax error in %s line %d, column %d",
				ip_ctx->fname,
				ip_ctx->errpos.nline,
				ip_ctx->errpos.ncol);
		}
#line 2106 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 20:
#line 409 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			peer_node *my_node;

			my_node = create_peer_node((yyvsp[-2].Integer), (yyvsp[-1].Address_node), (yyvsp[0].Attr_val_fifo));
			APPEND_G_FIFO(cfgt.peers, my_node);
		}
#line 2117 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 27:
#line 428 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    { (yyval.Address_node) = create_address_node((yyvsp[0].String), (yyvsp[-1].Integer)); }
#line 2123 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 28:
#line 433 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    { (yyval.Address_node) = create_address_node((yyvsp[0].String), AF_UNSPEC); }
#line 2129 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 29:
#line 438 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    { (yyval.Integer) = AF_INET; }
#line 2135 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 30:
#line 440 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    { (yyval.Integer) = AF_INET6; }
#line 2141 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 31:
#line 445 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    { (yyval.Attr_val_fifo) = NULL; }
#line 2147 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 32:
#line 447 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			(yyval.Attr_val_fifo) = (yyvsp[-1].Attr_val_fifo);
			APPEND_G_FIFO((yyval.Attr_val_fifo), (yyvsp[0].Attr_val));
		}
#line 2156 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 36:
#line 461 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    { (yyval.Attr_val) = create_attr_ival(T_Flag, (yyvsp[0].Integer)); }
#line 2162 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 45:
#line 477 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    { (yyval.Attr_val) = create_attr_ival((yyvsp[-1].Integer), (yyvsp[0].Integer)); }
#line 2168 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 46:
#line 479 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    { (yyval.Attr_val) = create_attr_uval((yyvsp[-1].Integer), (yyvsp[0].Integer)); }
#line 2174 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 53:
#line 493 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    { (yyval.Attr_val) = create_attr_sval((yyvsp[-1].Integer), (yyvsp[0].String)); }
#line 2180 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 55:
#line 507 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			unpeer_node *my_node;

			my_node = create_unpeer_node((yyvsp[0].Address_node));
			if (my_node)
				APPEND_G_FIFO(cfgt.unpeers, my_node);
		}
#line 2192 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 58:
#line 528 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    { cfgt.broadcastclient = 1; }
#line 2198 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 59:
#line 530 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    { CONCAT_G_FIFOS(cfgt.manycastserver, (yyvsp[0].Address_fifo)); }
#line 2204 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 60:
#line 532 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    { CONCAT_G_FIFOS(cfgt.multicastclient, (yyvsp[0].Address_fifo)); }
#line 2210 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 61:
#line 534 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    { cfgt.mdnstries = (yyvsp[0].Integer); }
#line 2216 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 62:
#line 545 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			attr_val *atrv;

			atrv = create_attr_ival((yyvsp[-1].Integer), (yyvsp[0].Integer));
			APPEND_G_FIFO(cfgt.vars, atrv);
		}
#line 2227 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 63:
#line 552 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    { cfgt.auth.control_key = (yyvsp[0].Integer); }
#line 2233 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 64:
#line 554 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			cfgt.auth.cryptosw++;
			CONCAT_G_FIFOS(cfgt.auth.crypto_cmd_list, (yyvsp[0].Attr_val_fifo));
		}
#line 2242 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 65:
#line 559 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    { cfgt.auth.keys = (yyvsp[0].String); }
#line 2248 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 66:
#line 561 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    { cfgt.auth.keysdir = (yyvsp[0].String); }
#line 2254 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 67:
#line 563 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    { cfgt.auth.request_key = (yyvsp[0].Integer); }
#line 2260 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 68:
#line 565 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    { cfgt.auth.revoke = (yyvsp[0].Integer); }
#line 2266 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 69:
#line 567 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			cfgt.auth.trusted_key_list = (yyvsp[0].Attr_val_fifo);

			// if (!cfgt.auth.trusted_key_list)
			// 	cfgt.auth.trusted_key_list = $2;
			// else
			// 	LINK_SLIST(cfgt.auth.trusted_key_list, $2, link);
		}
#line 2279 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 70:
#line 576 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    { cfgt.auth.ntp_signd_socket = (yyvsp[0].String); }
#line 2285 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 71:
#line 581 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    { (yyval.Attr_val_fifo) = NULL; }
#line 2291 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 72:
#line 583 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			(yyval.Attr_val_fifo) = (yyvsp[-1].Attr_val_fifo);
			APPEND_G_FIFO((yyval.Attr_val_fifo), (yyvsp[0].Attr_val));
		}
#line 2300 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 73:
#line 591 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    { (yyval.Attr_val) = create_attr_sval((yyvsp[-1].Integer), (yyvsp[0].String)); }
#line 2306 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 74:
#line 593 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			(yyval.Attr_val) = NULL;
			cfgt.auth.revoke = (yyvsp[0].Integer);
			msyslog(LOG_WARNING,
				"'crypto revoke %d' is deprecated, "
				"please use 'revoke %d' instead.",
				cfgt.auth.revoke, cfgt.auth.revoke);
		}
#line 2319 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 80:
#line 618 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    { CONCAT_G_FIFOS(cfgt.orphan_cmds, (yyvsp[0].Attr_val_fifo)); }
#line 2325 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 81:
#line 623 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			(yyval.Attr_val_fifo) = (yyvsp[-1].Attr_val_fifo);
			APPEND_G_FIFO((yyval.Attr_val_fifo), (yyvsp[0].Attr_val));
		}
#line 2334 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 82:
#line 628 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			(yyval.Attr_val_fifo) = NULL;
			APPEND_G_FIFO((yyval.Attr_val_fifo), (yyvsp[0].Attr_val));
		}
#line 2343 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 83:
#line 636 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    { (yyval.Attr_val) = create_attr_dval((yyvsp[-1].Integer), (double)(yyvsp[0].Integer)); }
#line 2349 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 84:
#line 638 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    { (yyval.Attr_val) = create_attr_dval((yyvsp[-1].Integer), (yyvsp[0].Double)); }
#line 2355 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 85:
#line 640 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    { (yyval.Attr_val) = create_attr_dval((yyvsp[-1].Integer), (double)(yyvsp[0].Integer)); }
#line 2361 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 96:
#line 666 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    { CONCAT_G_FIFOS(cfgt.stats_list, (yyvsp[0].Int_fifo)); }
#line 2367 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 97:
#line 668 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			if (lex_from_file()) {
				cfgt.stats_dir = (yyvsp[0].String);
			} else {
				YYFREE((yyvsp[0].String));
				yyerror("statsdir remote configuration ignored");
			}
		}
#line 2380 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 98:
#line 677 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			filegen_node *fgn;

			fgn = create_filegen_node((yyvsp[-1].Integer), (yyvsp[0].Attr_val_fifo));
			APPEND_G_FIFO(cfgt.filegen_opts, fgn);
		}
#line 2391 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 99:
#line 687 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			(yyval.Int_fifo) = (yyvsp[-1].Int_fifo);
			APPEND_G_FIFO((yyval.Int_fifo), create_int_node((yyvsp[0].Integer)));
		}
#line 2400 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 100:
#line 692 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			(yyval.Int_fifo) = NULL;
			APPEND_G_FIFO((yyval.Int_fifo), create_int_node((yyvsp[0].Integer)));
		}
#line 2409 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 109:
#line 711 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    { (yyval.Attr_val_fifo) = NULL; }
#line 2415 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 110:
#line 713 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			(yyval.Attr_val_fifo) = (yyvsp[-1].Attr_val_fifo);
			APPEND_G_FIFO((yyval.Attr_val_fifo), (yyvsp[0].Attr_val));
		}
#line 2424 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 111:
#line 721 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			if (lex_from_file()) {
				(yyval.Attr_val) = create_attr_sval((yyvsp[-1].Integer), (yyvsp[0].String));
			} else {
				(yyval.Attr_val) = NULL;
				YYFREE((yyvsp[0].String));
				yyerror("filegen file remote config ignored");
			}
		}
#line 2438 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 112:
#line 731 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			if (lex_from_file()) {
				(yyval.Attr_val) = create_attr_ival((yyvsp[-1].Integer), (yyvsp[0].Integer));
			} else {
				(yyval.Attr_val) = NULL;
				yyerror("filegen type remote config ignored");
			}
		}
#line 2451 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 113:
#line 740 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			const char *err;

			if (lex_from_file()) {
				(yyval.Attr_val) = create_attr_ival(T_Flag, (yyvsp[0].Integer));
			} else {
				(yyval.Attr_val) = NULL;
				if (T_Link == (yyvsp[0].Integer))
					err = "filegen link remote config ignored";
				else
					err = "filegen nolink remote config ignored";
				yyerror(err);
			}
		}
#line 2470 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 114:
#line 755 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    { (yyval.Attr_val) = create_attr_ival(T_Flag, (yyvsp[0].Integer)); }
#line 2476 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 126:
#line 785 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			CONCAT_G_FIFOS(cfgt.discard_opts, (yyvsp[0].Attr_val_fifo));
		}
#line 2484 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 127:
#line 789 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			CONCAT_G_FIFOS(cfgt.mru_opts, (yyvsp[0].Attr_val_fifo));
		}
#line 2492 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 128:
#line 793 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			restrict_node *rn;

			rn = create_restrict_node((yyvsp[-1].Address_node), NULL, (yyvsp[0].Int_fifo),
						  lex_current()->curpos.nline);
			APPEND_G_FIFO(cfgt.restrict_opts, rn);
		}
#line 2504 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 129:
#line 801 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			restrict_node *rn;

			rn = create_restrict_node((yyvsp[-3].Address_node), (yyvsp[-1].Address_node), (yyvsp[0].Int_fifo),
						  lex_current()->curpos.nline);
			APPEND_G_FIFO(cfgt.restrict_opts, rn);
		}
#line 2516 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 130:
#line 809 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			restrict_node *rn;

			rn = create_restrict_node(NULL, NULL, (yyvsp[0].Int_fifo),
						  lex_current()->curpos.nline);
			APPEND_G_FIFO(cfgt.restrict_opts, rn);
		}
#line 2528 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 131:
#line 817 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			restrict_node *rn;

			rn = create_restrict_node(
				create_address_node(
					estrdup("0.0.0.0"),
					AF_INET),
				create_address_node(
					estrdup("0.0.0.0"),
					AF_INET),
				(yyvsp[0].Int_fifo),
				lex_current()->curpos.nline);
			APPEND_G_FIFO(cfgt.restrict_opts, rn);
		}
#line 2547 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 132:
#line 832 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			restrict_node *rn;

			rn = create_restrict_node(
				create_address_node(
					estrdup("::"),
					AF_INET6),
				create_address_node(
					estrdup("::"),
					AF_INET6),
				(yyvsp[0].Int_fifo),
				lex_current()->curpos.nline);
			APPEND_G_FIFO(cfgt.restrict_opts, rn);
		}
#line 2566 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 133:
#line 847 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			restrict_node *	rn;

			APPEND_G_FIFO((yyvsp[0].Int_fifo), create_int_node((yyvsp[-1].Integer)));
			rn = create_restrict_node(
				NULL, NULL, (yyvsp[0].Int_fifo), lex_current()->curpos.nline);
			APPEND_G_FIFO(cfgt.restrict_opts, rn);
		}
#line 2579 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 134:
#line 859 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    { (yyval.Int_fifo) = NULL; }
#line 2585 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 135:
#line 861 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			(yyval.Int_fifo) = (yyvsp[-1].Int_fifo);
			APPEND_G_FIFO((yyval.Int_fifo), create_int_node((yyvsp[0].Integer)));
		}
#line 2594 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 151:
#line 887 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			(yyval.Attr_val_fifo) = (yyvsp[-1].Attr_val_fifo);
			APPEND_G_FIFO((yyval.Attr_val_fifo), (yyvsp[0].Attr_val));
		}
#line 2603 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 152:
#line 892 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			(yyval.Attr_val_fifo) = NULL;
			APPEND_G_FIFO((yyval.Attr_val_fifo), (yyvsp[0].Attr_val));
		}
#line 2612 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 153:
#line 900 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    { (yyval.Attr_val) = create_attr_ival((yyvsp[-1].Integer), (yyvsp[0].Integer)); }
#line 2618 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 157:
#line 911 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			(yyval.Attr_val_fifo) = (yyvsp[-1].Attr_val_fifo);
			APPEND_G_FIFO((yyval.Attr_val_fifo), (yyvsp[0].Attr_val));
		}
#line 2627 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 158:
#line 916 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			(yyval.Attr_val_fifo) = NULL;
			APPEND_G_FIFO((yyval.Attr_val_fifo), (yyvsp[0].Attr_val));
		}
#line 2636 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 159:
#line 924 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    { (yyval.Attr_val) = create_attr_ival((yyvsp[-1].Integer), (yyvsp[0].Integer)); }
#line 2642 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 168:
#line 944 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			addr_opts_node *aon;

			aon = create_addr_opts_node((yyvsp[-1].Address_node), (yyvsp[0].Attr_val_fifo));
			APPEND_G_FIFO(cfgt.fudge, aon);
		}
#line 2653 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 169:
#line 954 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			(yyval.Attr_val_fifo) = (yyvsp[-1].Attr_val_fifo);
			APPEND_G_FIFO((yyval.Attr_val_fifo), (yyvsp[0].Attr_val));
		}
#line 2662 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 170:
#line 959 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			(yyval.Attr_val_fifo) = NULL;
			APPEND_G_FIFO((yyval.Attr_val_fifo), (yyvsp[0].Attr_val));
		}
#line 2671 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 171:
#line 967 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    { (yyval.Attr_val) = create_attr_dval((yyvsp[-1].Integer), (yyvsp[0].Double)); }
#line 2677 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 172:
#line 969 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    { (yyval.Attr_val) = create_attr_ival((yyvsp[-1].Integer), (yyvsp[0].Integer)); }
#line 2683 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 173:
#line 971 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			if ((yyvsp[0].Integer) >= 0 && (yyvsp[0].Integer) <= 16) {
				(yyval.Attr_val) = create_attr_ival((yyvsp[-1].Integer), (yyvsp[0].Integer));
			} else {
				(yyval.Attr_val) = NULL;
				yyerror("fudge factor: stratum value not in [0..16], ignored");
			}
		}
#line 2696 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 174:
#line 980 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    { (yyval.Attr_val) = create_attr_sval((yyvsp[-1].Integer), (yyvsp[0].String)); }
#line 2702 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 175:
#line 982 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    { (yyval.Attr_val) = create_attr_sval((yyvsp[-1].Integer), (yyvsp[0].String)); }
#line 2708 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 182:
#line 1003 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    { CONCAT_G_FIFOS(cfgt.rlimit, (yyvsp[0].Attr_val_fifo)); }
#line 2714 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 183:
#line 1008 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			(yyval.Attr_val_fifo) = (yyvsp[-1].Attr_val_fifo);
			APPEND_G_FIFO((yyval.Attr_val_fifo), (yyvsp[0].Attr_val));
		}
#line 2723 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 184:
#line 1013 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			(yyval.Attr_val_fifo) = NULL;
			APPEND_G_FIFO((yyval.Attr_val_fifo), (yyvsp[0].Attr_val));
		}
#line 2732 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 185:
#line 1021 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    { (yyval.Attr_val) = create_attr_ival((yyvsp[-1].Integer), (yyvsp[0].Integer)); }
#line 2738 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 189:
#line 1037 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    { CONCAT_G_FIFOS(cfgt.enable_opts, (yyvsp[0].Attr_val_fifo)); }
#line 2744 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 190:
#line 1039 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    { CONCAT_G_FIFOS(cfgt.disable_opts, (yyvsp[0].Attr_val_fifo)); }
#line 2750 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 191:
#line 1044 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			(yyval.Attr_val_fifo) = (yyvsp[-1].Attr_val_fifo);
			APPEND_G_FIFO((yyval.Attr_val_fifo), (yyvsp[0].Attr_val));
		}
#line 2759 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 192:
#line 1049 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			(yyval.Attr_val_fifo) = NULL;
			APPEND_G_FIFO((yyval.Attr_val_fifo), (yyvsp[0].Attr_val));
		}
#line 2768 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 193:
#line 1057 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    { (yyval.Attr_val) = create_attr_ival(T_Flag, (yyvsp[0].Integer)); }
#line 2774 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 194:
#line 1059 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			if (lex_from_file()) {
				(yyval.Attr_val) = create_attr_ival(T_Flag, (yyvsp[0].Integer));
			} else {
				char err_str[128];

				(yyval.Attr_val) = NULL;
				snprintf(err_str, sizeof(err_str),
					 "enable/disable %s remote configuration ignored",
					 keyword((yyvsp[0].Integer)));
				yyerror(err_str);
			}
		}
#line 2792 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 203:
#line 1094 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    { CONCAT_G_FIFOS(cfgt.tinker, (yyvsp[0].Attr_val_fifo)); }
#line 2798 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 204:
#line 1099 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			(yyval.Attr_val_fifo) = (yyvsp[-1].Attr_val_fifo);
			APPEND_G_FIFO((yyval.Attr_val_fifo), (yyvsp[0].Attr_val));
		}
#line 2807 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 205:
#line 1104 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			(yyval.Attr_val_fifo) = NULL;
			APPEND_G_FIFO((yyval.Attr_val_fifo), (yyvsp[0].Attr_val));
		}
#line 2816 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 206:
#line 1112 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    { (yyval.Attr_val) = create_attr_dval((yyvsp[-1].Integer), (yyvsp[0].Double)); }
#line 2822 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 219:
#line 1137 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			attr_val *av;

			av = create_attr_dval((yyvsp[-1].Integer), (yyvsp[0].Double));
			APPEND_G_FIFO(cfgt.vars, av);
		}
#line 2833 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 220:
#line 1144 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			attr_val *av;

			av = create_attr_ival((yyvsp[-1].Integer), (yyvsp[0].Integer));
			APPEND_G_FIFO(cfgt.vars, av);
		}
#line 2844 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 221:
#line 1151 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			attr_val *av;

			av = create_attr_sval((yyvsp[-1].Integer), (yyvsp[0].String));
			APPEND_G_FIFO(cfgt.vars, av);
		}
#line 2855 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 222:
#line 1158 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			char error_text[64];
			attr_val *av;

			if (lex_from_file()) {
				av = create_attr_sval((yyvsp[-1].Integer), (yyvsp[0].String));
				APPEND_G_FIFO(cfgt.vars, av);
			} else {
				YYFREE((yyvsp[0].String));
				snprintf(error_text, sizeof(error_text),
					 "%s remote config ignored",
					 keyword((yyvsp[-1].Integer)));
				yyerror(error_text);
			}
		}
#line 2875 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 223:
#line 1174 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			if (!lex_from_file()) {
				YYFREE((yyvsp[-1].String)); /* avoid leak */
				yyerror("remote includefile ignored");
				break;
			}
			if (lex_level() > MAXINCLUDELEVEL) {
				fprintf(stderr, "getconfig: Maximum include file level exceeded.\n");
				msyslog(LOG_ERR, "getconfig: Maximum include file level exceeded.");
			} else {
				const char * path = FindConfig((yyvsp[-1].String)); /* might return $2! */
				if (!lex_push_file(path, "r")) {
					fprintf(stderr, "getconfig: Couldn't open <%s>\n", path);
					msyslog(LOG_ERR, "getconfig: Couldn't open <%s>", path);
				}
			}
			YYFREE((yyvsp[-1].String)); /* avoid leak */
		}
#line 2898 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 224:
#line 1193 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    { lex_flush_stack(); }
#line 2904 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 225:
#line 1195 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    { /* see drift_parm below for actions */ }
#line 2910 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 226:
#line 1197 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    { CONCAT_G_FIFOS(cfgt.logconfig, (yyvsp[0].Attr_val_fifo)); }
#line 2916 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 227:
#line 1199 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    { CONCAT_G_FIFOS(cfgt.phone, (yyvsp[0].String_fifo)); }
#line 2922 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 228:
#line 1201 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    { APPEND_G_FIFO(cfgt.setvar, (yyvsp[0].Set_var)); }
#line 2928 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 229:
#line 1203 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			addr_opts_node *aon;

			aon = create_addr_opts_node((yyvsp[-1].Address_node), (yyvsp[0].Attr_val_fifo));
			APPEND_G_FIFO(cfgt.trap, aon);
		}
#line 2939 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 230:
#line 1210 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    { CONCAT_G_FIFOS(cfgt.ttl, (yyvsp[0].Attr_val_fifo)); }
#line 2945 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 235:
#line 1225 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
#ifndef LEAP_SMEAR
			yyerror("Built without LEAP_SMEAR support.");
#endif
		}
#line 2955 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 241:
#line 1245 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			if (lex_from_file()) {
				attr_val *av;
				av = create_attr_sval(T_Driftfile, (yyvsp[0].String));
				APPEND_G_FIFO(cfgt.vars, av);
			} else {
				YYFREE((yyvsp[0].String));
				yyerror("driftfile remote configuration ignored");
			}
		}
#line 2970 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 242:
#line 1256 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			if (lex_from_file()) {
				attr_val *av;
				av = create_attr_sval(T_Driftfile, (yyvsp[-1].String));
				APPEND_G_FIFO(cfgt.vars, av);
				av = create_attr_dval(T_WanderThreshold, (yyvsp[0].Double));
				APPEND_G_FIFO(cfgt.vars, av);
			} else {
				YYFREE((yyvsp[-1].String));
				yyerror("driftfile remote configuration ignored");
			}
		}
#line 2987 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 243:
#line 1269 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			if (lex_from_file()) {
				attr_val *av;
				av = create_attr_sval(T_Driftfile, estrdup(""));
				APPEND_G_FIFO(cfgt.vars, av);
			} else {
				yyerror("driftfile remote configuration ignored");
			}
		}
#line 3001 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 244:
#line 1282 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    { (yyval.Set_var) = create_setvar_node((yyvsp[-3].String), (yyvsp[-1].String), (yyvsp[0].Integer)); }
#line 3007 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 246:
#line 1288 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    { (yyval.Integer) = 0; }
#line 3013 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 247:
#line 1293 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    { (yyval.Attr_val_fifo) = NULL; }
#line 3019 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 248:
#line 1295 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			(yyval.Attr_val_fifo) = (yyvsp[-1].Attr_val_fifo);
			APPEND_G_FIFO((yyval.Attr_val_fifo), (yyvsp[0].Attr_val));
		}
#line 3028 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 249:
#line 1303 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    { (yyval.Attr_val) = create_attr_ival((yyvsp[-1].Integer), (yyvsp[0].Integer)); }
#line 3034 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 250:
#line 1305 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			(yyval.Attr_val) = create_attr_sval((yyvsp[-1].Integer), estrdup((yyvsp[0].Address_node)->address));
			destroy_address_node((yyvsp[0].Address_node));
		}
#line 3043 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 251:
#line 1313 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			(yyval.Attr_val_fifo) = (yyvsp[-1].Attr_val_fifo);
			APPEND_G_FIFO((yyval.Attr_val_fifo), (yyvsp[0].Attr_val));
		}
#line 3052 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 252:
#line 1318 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			(yyval.Attr_val_fifo) = NULL;
			APPEND_G_FIFO((yyval.Attr_val_fifo), (yyvsp[0].Attr_val));
		}
#line 3061 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 253:
#line 1326 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			char	prefix;
			char *	type;

			switch ((yyvsp[0].String)[0]) {

			case '+':
			case '-':
			case '=':
				prefix = (yyvsp[0].String)[0];
				type = (yyvsp[0].String) + 1;
				break;

			default:
				prefix = '=';
				type = (yyvsp[0].String);
			}

			(yyval.Attr_val) = create_attr_sval(prefix, estrdup(type));
			YYFREE((yyvsp[0].String));
		}
#line 3087 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 254:
#line 1351 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			nic_rule_node *nrn;

			nrn = create_nic_rule_node((yyvsp[0].Integer), NULL, (yyvsp[-1].Integer));
			APPEND_G_FIFO(cfgt.nic_rules, nrn);
		}
#line 3098 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 255:
#line 1358 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			nic_rule_node *nrn;

			nrn = create_nic_rule_node(0, (yyvsp[0].String), (yyvsp[-1].Integer));
			APPEND_G_FIFO(cfgt.nic_rules, nrn);
		}
#line 3109 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 265:
#line 1386 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    { CONCAT_G_FIFOS(cfgt.reset_counters, (yyvsp[0].Int_fifo)); }
#line 3115 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 266:
#line 1391 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			(yyval.Int_fifo) = (yyvsp[-1].Int_fifo);
			APPEND_G_FIFO((yyval.Int_fifo), create_int_node((yyvsp[0].Integer)));
		}
#line 3124 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 267:
#line 1396 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			(yyval.Int_fifo) = NULL;
			APPEND_G_FIFO((yyval.Int_fifo), create_int_node((yyvsp[0].Integer)));
		}
#line 3133 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 275:
#line 1420 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			(yyval.Attr_val_fifo) = (yyvsp[-1].Attr_val_fifo);
			APPEND_G_FIFO((yyval.Attr_val_fifo), create_int_node((yyvsp[0].Integer)));
		}
#line 3142 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 276:
#line 1425 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			(yyval.Attr_val_fifo) = NULL;
			APPEND_G_FIFO((yyval.Attr_val_fifo), create_int_node((yyvsp[0].Integer)));
		}
#line 3151 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 277:
#line 1433 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			(yyval.Attr_val_fifo) = (yyvsp[-1].Attr_val_fifo);
			APPEND_G_FIFO((yyval.Attr_val_fifo), (yyvsp[0].Attr_val));
		}
#line 3160 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 278:
#line 1438 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			(yyval.Attr_val_fifo) = NULL;
			APPEND_G_FIFO((yyval.Attr_val_fifo), (yyvsp[0].Attr_val));
		}
#line 3169 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 279:
#line 1446 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    { (yyval.Attr_val) = create_attr_ival('i', (yyvsp[0].Integer)); }
#line 3175 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 281:
#line 1452 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    { (yyval.Attr_val) = create_attr_rangeval('-', (yyvsp[-3].Integer), (yyvsp[-1].Integer)); }
#line 3181 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 282:
#line 1457 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			(yyval.String_fifo) = (yyvsp[-1].String_fifo);
			APPEND_G_FIFO((yyval.String_fifo), create_string_node((yyvsp[0].String)));
		}
#line 3190 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 283:
#line 1462 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			(yyval.String_fifo) = NULL;
			APPEND_G_FIFO((yyval.String_fifo), create_string_node((yyvsp[0].String)));
		}
#line 3199 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 284:
#line 1470 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			(yyval.Address_fifo) = (yyvsp[-1].Address_fifo);
			APPEND_G_FIFO((yyval.Address_fifo), (yyvsp[0].Address_node));
		}
#line 3208 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 285:
#line 1475 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			(yyval.Address_fifo) = NULL;
			APPEND_G_FIFO((yyval.Address_fifo), (yyvsp[0].Address_node));
		}
#line 3217 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 286:
#line 1483 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			if ((yyvsp[0].Integer) != 0 && (yyvsp[0].Integer) != 1) {
				yyerror("Integer value is not boolean (0 or 1). Assuming 1");
				(yyval.Integer) = 1;
			} else {
				(yyval.Integer) = (yyvsp[0].Integer);
			}
		}
#line 3230 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 287:
#line 1491 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    { (yyval.Integer) = 1; }
#line 3236 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 288:
#line 1492 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    { (yyval.Integer) = 0; }
#line 3242 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 289:
#line 1496 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    { (yyval.Double) = (double)(yyvsp[0].Integer); }
#line 3248 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 291:
#line 1507 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			sim_node *sn;

			sn =  create_sim_node((yyvsp[-2].Attr_val_fifo), (yyvsp[-1].Sim_server_fifo));
			APPEND_G_FIFO(cfgt.sim_details, sn);

			/* Revert from ; to \n for end-of-command */
			old_config_style = 1;
		}
#line 3262 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 292:
#line 1524 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    { old_config_style = 0; }
#line 3268 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 293:
#line 1529 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			(yyval.Attr_val_fifo) = (yyvsp[-2].Attr_val_fifo);
			APPEND_G_FIFO((yyval.Attr_val_fifo), (yyvsp[-1].Attr_val));
		}
#line 3277 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 294:
#line 1534 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			(yyval.Attr_val_fifo) = NULL;
			APPEND_G_FIFO((yyval.Attr_val_fifo), (yyvsp[-1].Attr_val));
		}
#line 3286 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 295:
#line 1542 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    { (yyval.Attr_val) = create_attr_dval((yyvsp[-2].Integer), (yyvsp[0].Double)); }
#line 3292 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 298:
#line 1552 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			(yyval.Sim_server_fifo) = (yyvsp[-1].Sim_server_fifo);
			APPEND_G_FIFO((yyval.Sim_server_fifo), (yyvsp[0].Sim_server));
		}
#line 3301 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 299:
#line 1557 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			(yyval.Sim_server_fifo) = NULL;
			APPEND_G_FIFO((yyval.Sim_server_fifo), (yyvsp[0].Sim_server));
		}
#line 3310 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 300:
#line 1565 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    { (yyval.Sim_server) = ONLY_SIM(create_sim_server((yyvsp[-4].Address_node), (yyvsp[-2].Double), (yyvsp[-1].Sim_script_fifo))); }
#line 3316 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 301:
#line 1570 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    { (yyval.Double) = (yyvsp[-1].Double); }
#line 3322 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 302:
#line 1575 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    { (yyval.Address_node) = (yyvsp[0].Address_node); }
#line 3328 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 303:
#line 1580 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			(yyval.Sim_script_fifo) = (yyvsp[-1].Sim_script_fifo);
			APPEND_G_FIFO((yyval.Sim_script_fifo), (yyvsp[0].Sim_script));
		}
#line 3337 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 304:
#line 1585 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			(yyval.Sim_script_fifo) = NULL;
			APPEND_G_FIFO((yyval.Sim_script_fifo), (yyvsp[0].Sim_script));
		}
#line 3346 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 305:
#line 1593 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    { (yyval.Sim_script) = ONLY_SIM(create_sim_script_info((yyvsp[-3].Double), (yyvsp[-1].Attr_val_fifo))); }
#line 3352 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 306:
#line 1598 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			(yyval.Attr_val_fifo) = (yyvsp[-2].Attr_val_fifo);
			APPEND_G_FIFO((yyval.Attr_val_fifo), (yyvsp[-1].Attr_val));
		}
#line 3361 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 307:
#line 1603 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    {
			(yyval.Attr_val_fifo) = NULL;
			APPEND_G_FIFO((yyval.Attr_val_fifo), (yyvsp[-1].Attr_val));
		}
#line 3370 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;

  case 308:
#line 1611 "../../ntpd/ntp_parser.y" /* yacc.c:1646  */
    { (yyval.Attr_val) = create_attr_dval((yyvsp[-2].Integer), (yyvsp[0].Double)); }
#line 3376 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
    break;


#line 3380 "../../ntpd/ntp_parser.c" /* yacc.c:1646  */
      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = (char *) YYSTACK_ALLOC (yymsg_alloc);
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYTERROR;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;


      yydestruct ("Error: popping",
                  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined yyoverflow || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  return yyresult;
}