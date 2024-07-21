static SplineFont *SFD_GetFont( FILE *sfd,SplineFont *cidmaster,char *tok,
				int fromdir, char *dirname, float sfdversion )
{
    SplineFont *sf;
    int realcnt, i, eof, mappos=-1, ch;
    struct table_ordering *lastord = NULL;
    struct axismap *lastaxismap = NULL;
    struct named_instance *lastnamedinstance = NULL;
    int pushedbacktok = false;
    Encoding *enc = &custom;
    struct remap *remap = NULL;
    int haddupenc;
    int old_style_order2 = false;
    int had_layer_cnt=false;

    orig_pos = 0;		/* Only used for compatibility with extremely old sfd files */

    sf = SplineFontEmpty();
    if ( sfdversion>0 && sfdversion<2 ) {
	/* If it's an old style sfd file with old style features we need some */
	/*  extra data space to do the conversion from old to new */
	sf = realloc(sf,sizeof(SplineFont1));
	memset(((uint8 *) sf) + sizeof(SplineFont),0,sizeof(SplineFont1)-sizeof(SplineFont));
    }
    sf->sfd_version = sfdversion;
    sf->cidmaster = cidmaster;
    sf->uni_interp = ui_unset;
	SFD_GetFontMetaDataData d;
	SFD_GetFontMetaDataData_Init( &d );
    while ( 1 ) {
	if ( pushedbacktok )
	    pushedbacktok = false;
	else if ( (eof = getname(sfd,tok))!=1 ) {
	    if ( eof==-1 )
    break;
	    geteol(sfd,tok);
    continue;
	}


	bool wasMetadata = SFD_GetFontMetaData( sfd, tok, sf, &d );
	had_layer_cnt = d.had_layer_cnt;
        if( wasMetadata )
        {
            // we have handled the token entirely
            // inside SFD_GetFontMetaData() move to next token.
            continue;
        }
        
        
	if ( strmatch(tok,"DisplaySize:")==0 )
	{
	    getint(sfd,&sf->display_size);
	}
	else if ( strmatch(tok,"DisplayLayer:")==0 )
	{
	    getint(sfd,&sf->display_layer);
	}
	else if ( strmatch(tok,"ExtremaBound:")==0 )
	{
	    getint(sfd,&sf->extrema_bound);
	}
	else if ( strmatch(tok,"WidthSeparation:")==0 )
	{
	    getint(sfd,&sf->width_separation);
	}
	else if ( strmatch(tok,"WinInfo:")==0 )
	{
	    int temp1, temp2;
	    getint(sfd,&sf->top_enc);
	    getint(sfd,&temp1);
	    getint(sfd,&temp2);
	    if ( sf->top_enc<=0 ) sf->top_enc=-1;
	    if ( temp1<=0 ) temp1 = 16;
	    if ( temp2<=0 ) temp2 = 4;
	    sf->desired_col_cnt = temp1;
	    sf->desired_row_cnt = temp2;
	}
	else if ( strmatch(tok,"AntiAlias:")==0 )
	{
	    int temp;
	    getint(sfd,&temp);
	    sf->display_antialias = temp;
	}
	else if ( strmatch(tok,"FitToEm:")==0 )
	{
	    int temp;
	    getint(sfd,&temp);
	    sf->display_bbsized = temp;
	}
	else if ( strmatch(tok,"OnlyBitmaps:")==0 )
	{
	    int temp;
	    getint(sfd,&temp);
	    sf->onlybitmaps = temp;
	}
	else if ( strmatch(tok,"Order2:")==0 )
	{
	    getint(sfd,&old_style_order2);
	    sf->grid.order2 = old_style_order2;
	    sf->layers[ly_back].order2 = old_style_order2;
	    sf->layers[ly_fore].order2 = old_style_order2;
	}
	else if ( strmatch(tok,"GridOrder2:")==0 )
	{
	    int o2;
	    getint(sfd,&o2);
	    sf->grid.order2 = o2;
	}
	else if ( strmatch(tok,"Encoding:")==0 )
	{
	    enc = SFDGetEncoding(sfd,tok);
	    if ( sf->map!=NULL ) sf->map->enc = enc;
	}
	else if ( strmatch(tok,"OldEncoding:")==0 )
	{
	    /* old_encname =*/ (void) SFDGetEncoding(sfd,tok);
	}
	else if ( strmatch(tok,"UnicodeInterp:")==0 )
	{
	    sf->uni_interp = SFDGetUniInterp(sfd,tok,sf);
	}
	else if ( strmatch(tok,"NameList:")==0 )
	{
	    SFDGetNameList(sfd,tok,sf);
	}
	else if ( strmatch(tok,"Compacted:")==0 )
	{
	    int temp;
	    getint(sfd,&temp);
	    sf->compacted = temp;
	}
	else if ( strmatch(tok,"Registry:")==0 )
	{
	    geteol(sfd,tok);
	    sf->cidregistry = copy(tok);
	}


	//////////


	else if ( strmatch(tok,"Ordering:")==0 ) {
	    geteol(sfd,tok);
	    sf->ordering = copy(tok);
	} else if ( strmatch(tok,"Supplement:")==0 ) {
	    getint(sfd,&sf->supplement);
	} else if ( strmatch(tok,"RemapN:")==0 ) {
	    int n;
	    getint(sfd,&n);
	    remap = calloc(n+1,sizeof(struct remap));
	    remap[n].infont = -1;
	    mappos = 0;
	    if ( sf->map!=NULL ) sf->map->remap = remap;
	} else if ( strmatch(tok,"Remap:")==0 ) {
	    uint32 f, l; int p;
	    gethex(sfd,&f);
	    gethex(sfd,&l);
	    getint(sfd,&p);
	    if ( remap!=NULL && remap[mappos].infont!=-1 ) {
		remap[mappos].firstenc = f;
		remap[mappos].lastenc = l;
		remap[mappos].infont = p;
		mappos++;
	    }
	} else if ( strmatch(tok,"CIDVersion:")==0 ) {
	    real temp;
	    getreal(sfd,&temp);
	    sf->cidversion = temp;
	} else if ( strmatch(tok,"Grid")==0 ) {
	    sf->grid.splines = SFDGetSplineSet(sfd,sf->grid.order2);
	} else if ( strmatch(tok,"ScriptLang:")==0 ) {
	    int i,j,k;
	    int imax, jmax, kmax;
	    if ( sf->sfd_version==0 || sf->sfd_version>=2 ) {
		IError( "Script lang lists should not happen in version 2 sfd files." );
                SplineFontFree(sf);
                return NULL;
	    }
	    getint(sfd,&imax);
	    ((SplineFont1 *) sf)->sli_cnt = imax;
	    ((SplineFont1 *) sf)->script_lang = malloc((imax+1)*sizeof(struct script_record *));
	    ((SplineFont1 *) sf)->script_lang[imax] = NULL;
	    for ( i=0; i<imax; ++i ) {
		getint(sfd,&jmax);
		((SplineFont1 *) sf)->script_lang[i] = malloc((jmax+1)*sizeof(struct script_record));
		((SplineFont1 *) sf)->script_lang[i][jmax].script = 0;
		for ( j=0; j<jmax; ++j ) {
		    ((SplineFont1 *) sf)->script_lang[i][j].script = gettag(sfd);
		    getint(sfd,&kmax);
		    ((SplineFont1 *) sf)->script_lang[i][j].langs = malloc((kmax+1)*sizeof(uint32));
		    ((SplineFont1 *) sf)->script_lang[i][j].langs[kmax] = 0;
		    for ( k=0; k<kmax; ++k ) {
			((SplineFont1 *) sf)->script_lang[i][j].langs[k] = gettag(sfd);
		    }
		}
	    }
	} else if ( strmatch(tok,"TeXData:")==0 ) {
	    int temp;
	    getint(sfd,&temp);
	    sf->texdata.type = temp;
	    getint(sfd, &temp);
	    if ( sf->design_size==0 ) {
	    	sf->design_size = (5*temp+(1<<18))>>19;
	    }
	    for ( i=0; i<22; ++i ) {
		int foo;
		getint(sfd,&foo);
		sf->texdata.params[i]=foo;
	    }
	} else if ( strnmatch(tok,"AnchorClass",11)==0 ) {
	    char *name;
	    AnchorClass *lastan = NULL, *an;
	    int old = strchr(tok,'2')==NULL;
	    while ( (name=SFDReadUTF7Str(sfd))!=NULL ) {
		an = chunkalloc(old ? sizeof(AnchorClass1) : sizeof(AnchorClass));
		an->name = name;
		if ( old ) {
		    getname(sfd,tok);
		    if ( tok[0]=='0' && tok[1]=='\0' )
			((AnchorClass1 *) an)->feature_tag = 0;
		    else {
			if ( tok[1]=='\0' ) { tok[1]=' '; tok[2] = 0; }
			if ( tok[2]=='\0' ) { tok[2]=' '; tok[3] = 0; }
			if ( tok[3]=='\0' ) { tok[3]=' '; tok[4] = 0; }
			((AnchorClass1 *) an)->feature_tag = (tok[0]<<24) | (tok[1]<<16) | (tok[2]<<8) | tok[3];
		    }
		    while ( (ch=nlgetc(sfd))==' ' || ch=='\t' );
		    ungetc(ch,sfd);
		    if ( isdigit(ch)) {
			int temp;
			getint(sfd,&temp);
			((AnchorClass1 *) an)->flags = temp;
		    }
		    while ( (ch=nlgetc(sfd))==' ' || ch=='\t' );
		    ungetc(ch,sfd);
		    if ( isdigit(ch)) {
			int temp;
			getint(sfd,&temp);
			((AnchorClass1 *) an)->script_lang_index = temp;
		    } else
			((AnchorClass1 *) an)->script_lang_index = 0xffff;		/* Will be fixed up later */
		    while ( (ch=nlgetc(sfd))==' ' || ch=='\t' );
		    ungetc(ch,sfd);
		    if ( isdigit(ch)) {
			int temp;
			getint(sfd,&temp);
			((AnchorClass1 *) an)->merge_with = temp;
		    } else
			((AnchorClass1 *) an)->merge_with = 0xffff;			/* Will be fixed up later */
		} else {
                    char *subtable_name = SFDReadUTF7Str(sfd);
                    if ( subtable_name!=NULL)                                           /* subtable is optional */
		        an->subtable = SFFindLookupSubtableAndFreeName(sf,subtable_name);
                }
		while ( (ch=nlgetc(sfd))==' ' || ch=='\t' );
		ungetc(ch,sfd);
		if ( isdigit(ch) ) {
		    /* Early versions of SfdFormat 2 had a number here */
		    int temp;
		    getint(sfd,&temp);
		    an->type = temp;
		} else if ( old ) {
		    if ( ((AnchorClass1 *) an)->feature_tag==CHR('c','u','r','s'))
			an->type = act_curs;
		    else if ( ((AnchorClass1 *) an)->feature_tag==CHR('m','k','m','k'))
			an->type = act_mkmk;
		    else
			an->type = act_mark;
		} else {
		    an->type = act_mark;
		    if( an->subtable && an->subtable->lookup )
		    {
			switch ( an->subtable->lookup->lookup_type )
			{
			case gpos_cursive:
			    an->type = act_curs;
			    break;
			case gpos_mark2base:
			    an->type = act_mark;
			    break;
			case gpos_mark2ligature:
			    an->type = act_mklg;
			    break;
			case gpos_mark2mark:
			    an->type = act_mkmk;
			    break;
			default:
			    an->type = act_mark;
			    break;
			}
		    }
		}
		if ( lastan==NULL )
		    sf->anchor = an;
		else
		    lastan->next = an;
		lastan = an;
	    }
	} else if ( strncmp(tok,"MATH:",5)==0 ) {
	    SFDParseMathItem(sfd,sf,tok);
	} else if ( strmatch(tok,"TableOrder:")==0 ) {
	    int temp;
	    struct table_ordering *ord;
	    if ( sfdversion==0 || sfdversion>=2 ) {
		IError("Table ordering specified in version 2 sfd file.\n" );
                SplineFontFree(sf);
                return NULL;
	    }
	    ord = chunkalloc(sizeof(struct table_ordering));
	    ord->table_tag = gettag(sfd);
	    getint(sfd,&temp);
	    ord->ordered_features = malloc((temp+1)*sizeof(uint32));
	    ord->ordered_features[temp] = 0;
	    for ( i=0; i<temp; ++i ) {
		while ( isspace((ch=nlgetc(sfd))) );
		if ( ch=='\'' ) {
		    ungetc(ch,sfd);
		    ord->ordered_features[i] = gettag(sfd);
		} else if ( ch=='<' ) {
		    int f,s;
		    fscanf(sfd,"%d,%d>", &f, &s );
		    ord->ordered_features[i] = (f<<16)|s;
		}
	    }
	    if ( lastord==NULL )
		((SplineFont1 *) sf)->orders = ord;
	    else
		lastord->next = ord;
	    lastord = ord;
	} else if ( strmatch(tok,"BeginPrivate:")==0 ) {
	    SFDGetPrivate(sfd,sf);
	} else if ( strmatch(tok,"BeginSubrs:")==0 ) {	/* leave in so we don't croak on old sfd files */
	    SFDGetSubrs(sfd);
	} else if ( strmatch(tok,"PickledData:")==0 ) {
	    if (sf->python_persistent != NULL) {
#if defined(_NO_PYTHON)
	      free( sf->python_persistent );	/* It's a string of pickled data which we leave as a string */
#else
	      PyFF_FreePythonPersistent(sf->python_persistent);
#endif
	      sf->python_persistent = NULL;
	    }
	    sf->python_persistent = SFDUnPickle(sfd, 0);
	    sf->python_persistent_has_lists = 0;
	} else if ( strmatch(tok,"PickledDataWithLists:")==0 ) {
	    if (sf->python_persistent != NULL) {
#if defined(_NO_PYTHON)
	      free( sf->python_persistent );	/* It's a string of pickled data which we leave as a string */
#else
	      PyFF_FreePythonPersistent(sf->python_persistent);
#endif
	      sf->python_persistent = NULL;
	    }
	    sf->python_persistent = SFDUnPickle(sfd, 1);
	    sf->python_persistent_has_lists = 1;
	} else if ( strmatch(tok,"MMCounts:")==0 ) {
	    MMSet *mm = sf->mm = chunkalloc(sizeof(MMSet));
	    getint(sfd,&mm->instance_count);
	    getint(sfd,&mm->axis_count);
	    ch = nlgetc(sfd);
	    if ( ch!=' ' )
		ungetc(ch,sfd);
	    else { int temp;
		getint(sfd,&temp);
		mm->apple = temp;
		getint(sfd,&mm->named_instance_count);
	    }
	    mm->instances = calloc(mm->instance_count,sizeof(SplineFont *));
	    mm->positions = malloc(mm->instance_count*mm->axis_count*sizeof(real));
	    mm->defweights = malloc(mm->instance_count*sizeof(real));
	    mm->axismaps = calloc(mm->axis_count,sizeof(struct axismap));
	    if ( mm->named_instance_count!=0 )
		mm->named_instances = calloc(mm->named_instance_count,sizeof(struct named_instance));
	} else if ( strmatch(tok,"MMAxis:")==0 ) {
	    MMSet *mm = sf->mm;
	    if ( mm!=NULL ) {
		for ( i=0; i<mm->axis_count; ++i ) {
		    getname(sfd,tok);
		    mm->axes[i] = copy(tok);
		}
	    }
	} else if ( strmatch(tok,"MMPositions:")==0 ) {
	    MMSet *mm = sf->mm;
	    if ( mm!=NULL ) {
		for ( i=0; i<mm->axis_count*mm->instance_count; ++i )
		    getreal(sfd,&mm->positions[i]);
	    }
	} else if ( strmatch(tok,"MMWeights:")==0 ) {
	    MMSet *mm = sf->mm;
	    if ( mm!=NULL ) {
		for ( i=0; i<mm->instance_count; ++i )
		    getreal(sfd,&mm->defweights[i]);
	    }
	} else if ( strmatch(tok,"MMAxisMap:")==0 ) {
	    MMSet *mm = sf->mm;
	    if ( mm!=NULL ) {
		int index, points;
		getint(sfd,&index); getint(sfd,&points);
		mm->axismaps[index].points = points;
		mm->axismaps[index].blends = malloc(points*sizeof(real));
		mm->axismaps[index].designs = malloc(points*sizeof(real));
		for ( i=0; i<points; ++i ) {
		    getreal(sfd,&mm->axismaps[index].blends[i]);
		    while ( (ch=nlgetc(sfd))!=EOF && isspace(ch));
		    ungetc(ch,sfd);
		    if ( (ch=nlgetc(sfd))!='=' )
			ungetc(ch,sfd);
		    else if ( (ch=nlgetc(sfd))!='>' )
			ungetc(ch,sfd);
		    getreal(sfd,&mm->axismaps[index].designs[i]);
		}
		lastaxismap = &mm->axismaps[index];
		lastnamedinstance = NULL;
	    }
	} else if ( strmatch(tok,"MMNamedInstance:")==0 ) {
	    MMSet *mm = sf->mm;
	    if ( mm!=NULL ) {
		int index;
		getint(sfd,&index);
		mm->named_instances[index].coords = malloc(mm->axis_count*sizeof(real));
		for ( i=0; i<mm->axis_count; ++i )
		    getreal(sfd,&mm->named_instances[index].coords[i]);
		lastnamedinstance = &mm->named_instances[index];
		lastaxismap = NULL;
	    }
	} else if ( strmatch(tok,"MacName:")==0 ) {
	    struct macname *names = SFDParseMacNames(sfd,tok);
	    if ( lastaxismap!=NULL )
		lastaxismap->axisnames = names;
	    else if ( lastnamedinstance !=NULL )
		lastnamedinstance->names = names;
	    pushedbacktok = true;
	} else if ( strmatch(tok,"MMCDV:")==0 ) {
	    MMSet *mm = sf->mm;
	    if ( mm!=NULL )
		mm->cdv = SFDParseMMSubroutine(sfd);
	} else if ( strmatch(tok,"MMNDV:")==0 ) {
	    MMSet *mm = sf->mm;
	    if ( mm!=NULL )
		mm->ndv = SFDParseMMSubroutine(sfd);
	} else if ( strmatch(tok,"BeginMMFonts:")==0 ) {
	    int cnt;
	    getint(sfd,&cnt);
	    getint(sfd,&realcnt);
	    ff_progress_change_stages(cnt);
	    ff_progress_change_total(realcnt);
	    MMInferStuff(sf->mm);
    break;
	} else if ( strmatch(tok,"BeginSubFonts:")==0 ) {
	    getint(sfd,&sf->subfontcnt);
	    sf->subfonts = calloc(sf->subfontcnt,sizeof(SplineFont *));
	    getint(sfd,&realcnt);
	    sf->map = EncMap1to1(realcnt);
	    ff_progress_change_stages(2);
	    ff_progress_change_total(realcnt);
    break;
	} else if ( strmatch(tok,"BeginChars:")==0 ) {
	    int charcnt;
	    getint(sfd,&charcnt);
	    if (charcnt<enc->char_cnt) {
		IError("SFD file specifies too few slots for its encoding.\n" );
exit( 1 );
	    }
	    if ( getint(sfd,&realcnt)!=1 || realcnt==-1 )
		realcnt = charcnt;
	    else
		++realcnt;		/* value saved is max glyph, not glyph cnt */
	    ff_progress_change_total(realcnt);
	    sf->glyphcnt = sf->glyphmax = realcnt;
	    sf->glyphs = calloc(realcnt,sizeof(SplineChar *));
	    if ( cidmaster!=NULL ) {
		sf->map = cidmaster->map;
	    } else {
		sf->map = EncMapNew(charcnt,realcnt,enc);
		sf->map->remap = remap;
	    }
	    SFDSizeMap(sf->map,sf->glyphcnt,charcnt);
    break;
#if HANYANG
	} else if ( strmatch(tok,"BeginCompositionRules")==0 ) {
	    sf->rules = SFDReadCompositionRules(sfd);
#endif
	} else {
	    /* If we don't understand it, skip it */
	    geteol(sfd,tok);
	}
    }

    if ( fromdir )
	sf = SFD_FigureDirType(sf,tok,dirname,enc,remap,had_layer_cnt);
    else if ( sf->subfontcnt!=0 ) {
	ff_progress_change_stages(2*sf->subfontcnt);
	for ( i=0; i<sf->subfontcnt; ++i ) {
	    if ( i!=0 )
		ff_progress_next_stage();
	    sf->subfonts[i] = SFD_GetFont(sfd,sf,tok,fromdir,dirname,sfdversion);
	}
    } else if ( sf->mm!=NULL ) {
	MMSet *mm = sf->mm;
	ff_progress_change_stages(2*(mm->instance_count+1));
	for ( i=0; i<mm->instance_count; ++i ) {
	    if ( i!=0 )
		ff_progress_next_stage();
	    mm->instances[i] = SFD_GetFont(sfd,NULL,tok,fromdir,dirname,sfdversion);
	    EncMapFree(mm->instances[i]->map); mm->instances[i]->map=NULL;
	    mm->instances[i]->mm = mm;
	}
	ff_progress_next_stage();
	mm->normal = SFD_GetFont(sfd,NULL,tok,fromdir,dirname,sfdversion);
	mm->normal->mm = mm;
	sf->mm = NULL;
	SplineFontFree(sf);
	sf = mm->normal;
	if ( sf->map->enc!=&custom ) {
	    EncMap *map;
	    MMMatchGlyphs(mm);		/* sfd files from before the encoding change can have mismatched orig pos */
	    map = EncMapFromEncoding(sf,sf->map->enc);
	    EncMapFree(sf->map);
	    sf->map = map;
	}
    } else {
	while ( SFDGetChar(sfd,sf,had_layer_cnt)!=NULL ) {
	    ff_progress_next();
	}
	ff_progress_next_stage();
    }
    haddupenc = false;
    while ( getname(sfd,tok)==1 ) {
	if ( strcmp(tok,"EndSplineFont")==0 || strcmp(tok,"EndSubSplineFont")==0 )
    break;
	else if ( strcmp(tok,"BitmapFont:")==0 )
	    SFDGetBitmapFont(sfd,sf,false,NULL);
	else if ( strmatch(tok,"DupEnc:")==0 ) {
	    int enc, orig;
	    haddupenc = true;
	    if ( getint(sfd,&enc) && getint(sfd,&orig) && sf->map!=NULL ) {
		SFDSetEncMap(sf,orig,enc);
	    }
	}
    }
    if ( sf->cidmaster==NULL )
	SFDFixupRefs(sf);

    if ( !haddupenc )
	SFD_DoAltUnis(sf);
    else
	AltUniFigure(sf,sf->map,true);
    if ( sf->sfd_version<2 )
	SFD_AssignLookups((SplineFont1 *) sf);
    if ( !d.hadtimes )
	SFTimesFromFile(sf,sfd);
    // Make a blank encoding if there are no characters so as to avoid crashes later.
    if (sf->map == NULL) sf->map = EncMapNew(sf->glyphcnt,sf->glyphcnt,&custom);

    SFDFixupUndoRefs(sf);
return( sf );
}