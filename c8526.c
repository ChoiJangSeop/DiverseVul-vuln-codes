void SFD_AssignLookups(SplineFont1 *sf) {
    PST1 *pst, *pst2;
    int isv;
    KernPair1 *kp, *kp2;
    KernClass1 *kc, *kc2;
    FPST1 *fpst;
    ASM1 *sm;
    AnchorClass1 *ac, *ac2;
    int gid, gid2, cnt, i, k, isgpos;
    SplineFont1 *subsf;
    SplineChar *sc, *sc2;
    OTLookup *otl, **all;
    struct lookup_subtable *sub;

    /* Fix up some gunk from really old versions of the sfd format */
    SFDCleanupAnchorClasses(&sf->sf);
    if ( sf->sf.uni_interp==ui_unset )
	sf->sf.uni_interp = interp_from_encoding(sf->sf.map->enc,ui_none);

    /* Fixup for an old bug */
    if ( sf->sf.pfminfo.os2_winascent < sf->sf.ascent/4 && !sf->sf.pfminfo.winascent_add ) {
	sf->sf.pfminfo.winascent_add = true;
	sf->sf.pfminfo.os2_winascent = 0;
	sf->sf.pfminfo.windescent_add = true;
	sf->sf.pfminfo.os2_windescent = 0;
    }

    /* First handle the PSTs, no complications here */
    k=0;
    do {
	subsf = sf->sf.subfontcnt==0 ? sf : (SplineFont1 *) (sf->sf.subfonts[k]);
	for ( gid=0; gid<subsf->sf.glyphcnt; ++gid ) if ( (sc=subsf->sf.glyphs[gid])!=NULL ) {
	    for ( pst = (PST1 *) (sc->possub); pst!=NULL; pst = (PST1*) (pst->pst.next) ) {
		if ( pst->pst.type == pst_lcaret || pst->pst.subtable!=NULL )
	    continue;		/* Nothing to do, or already done */
		otl = CreateLookup(sf,pst->tag,pst->script_lang_index,pst->flags,pst->pst.type);
		sub = CreateSubtable(otl,sf);
		/* There might be another PST with the same flags on this glyph */
		/* And we must fixup the current pst */
		for ( pst2=pst ; pst2!=NULL; pst2 = (PST1 *) (pst2->pst.next) ) {
		    if ( pst2->tag==pst->tag &&
			    pst2->script_lang_index==pst->script_lang_index &&
			    pst2->flags==pst->flags &&
			    pst2->pst.type==pst->pst.type )
			pst2->pst.subtable = sub;
		}
		for ( gid2=gid+1; gid2<subsf->sf.glyphcnt; ++gid2 ) if ( (sc2=subsf->sf.glyphs[gid2])!=NULL ) {
		    for ( pst2 = (PST1 *) (sc2->possub); pst2!=NULL; pst2 = (PST1 *) (pst2->pst.next) ) {
			if ( pst2->tag==pst->tag &&
				pst2->script_lang_index==pst->script_lang_index &&
				pst2->flags==pst->flags &&
				pst2->pst.type==pst->pst.type )
			    pst2->pst.subtable = sub;
		    }
		}
	    }
	}
	++k;
    } while ( k<sf->sf.subfontcnt );

	/* Now kerns. May need to merge kernclasses to kernpair lookups (different subtables, of course */
    for ( isv=0; isv<2; ++isv ) {
	k=0;
	do {
	    subsf = sf->sf.subfontcnt==0 ? sf : (SplineFont1 *) (sf->sf.subfonts[k]);
	    for ( gid=0; gid<subsf->sf.glyphcnt; ++gid ) if ( (sc=subsf->sf.glyphs[gid])!=NULL ) {
		for ( kp = (KernPair1 *) (isv ? sc->vkerns : sc->kerns); kp!=NULL; kp = (KernPair1 *) (kp->kp.next) ) {
		    if ( kp->kp.subtable!=NULL )
		continue;		/* already done */
		    otl = CreateLookup(sf,isv ? CHR('v','k','r','n') : CHR('k','e','r','n'),
			    kp->sli,kp->flags,pst_pair);
		    sub = CreateSubtable(otl,sf);
		    /* There might be another kp with the same flags on this glyph */
		    /* And we must fixup the current kp */
		    for ( kp2=kp ; kp2!=NULL; kp2 = (KernPair1 *) (kp2->kp.next) ) {
			if ( kp2->sli==kp->sli && kp2->flags==kp->flags )
			    kp2->kp.subtable = sub;
		    }
		    for ( gid2=gid+1; gid2<subsf->sf.glyphcnt; ++gid2 ) if ( (sc2=subsf->sf.glyphs[gid2])!=NULL ) {
			for ( kp2 = (KernPair1 *) (isv ? sc2->vkerns : sc2->kerns); kp2!=NULL; kp2 = (KernPair1 *) (kp2->kp.next) ) {
			    if ( kp2->sli==kp->sli && kp2->flags==kp->flags )
				kp2->kp.subtable = sub;
			}
		    }
		    /* And there might be a kerning class... */
		    for ( kc=(KernClass1 *) (isv ? sf->sf.vkerns : sf->sf.kerns); kc!=NULL;
			    kc = (KernClass1 *) (kc->kc.next) ) {
			if ( kc->sli == kp->sli && kc->flags == kp->flags && kc->kc.subtable==NULL) {
			    sub = CreateSubtable(otl,sf);
			    sub->per_glyph_pst_or_kern = false;
			    sub->kc = &kc->kc;
			    kc->kc.subtable = sub;
			}
		    }
		}
	    }
	    ++k;
	} while ( k<sf->sf.subfontcnt );
	/* Or there might be a kerning class all by its lonesome */
	for ( kc=(KernClass1 *) (isv ? sf->sf.vkerns : sf->sf.kerns); kc!=NULL;
		kc = (KernClass1 *) (kc->kc.next) ) {
	    if ( kc->kc.subtable==NULL) {
		otl = CreateLookup(sf,isv ? CHR('v','k','r','n') : CHR('k','e','r','n'),
			kc->sli,kc->flags,pst_pair);
		for ( kc2=kc; kc2!=NULL; kc2=(KernClass1 *) (kc2->kc.next) ) {
		    if ( kc->sli == kc2->sli && kc->flags == kc2->flags && kc2->kc.subtable==NULL) {
			sub = CreateSubtable(otl,sf);
			sub->per_glyph_pst_or_kern = false;
			sub->kc = &kc2->kc;
			kc2->kc.subtable = sub;
		    }
		}
	    }
	}
    }

    /* Every FPST and ASM lives in its own lookup with one subtable */
    /* But the old format refered to nested lookups by tag, and now we refer */
    /*  to the lookup itself, so fix that up */
    for ( fpst=(FPST1 *) sf->sf.possub; fpst!=NULL; fpst=((FPST1 *) fpst->fpst.next) ) {
	otl = CreateLookup(sf,fpst->tag, fpst->script_lang_index,
		fpst->flags,fpst->fpst.type);
	sub = CreateSubtable(otl,sf);
	sub->per_glyph_pst_or_kern = false;
	sub->fpst = &fpst->fpst;
	fpst->fpst.subtable = sub;
	FPSTReplaceTagsWithLookups(&fpst->fpst,sf);
    }
    for ( sm=(ASM1 *) sf->sf.sm; sm!=NULL; sm=((ASM1 *) sm->sm.next) ) {
	otl = CreateMacLookup(sf,sm);
	sub = CreateSubtable(otl,sf);
	sub->per_glyph_pst_or_kern = false;
	sub->sm = &sm->sm;
	sm->sm.subtable = sub;
	if ( sm->sm.type==asm_context )
	    ASMReplaceTagsWithLookups(&sm->sm,sf);
    }

    /* We retained the old nested feature tags so we could do the above conversion */
    /*  of tag to lookup. Get rid of them now */
    for ( isgpos=0; isgpos<2; ++isgpos ) {
	for ( otl = isgpos ? sf->sf.gpos_lookups : sf->sf.gsub_lookups ;
		otl != NULL; otl=otl->next ) {
	    if ( otl->features!=NULL && otl->features->scripts==NULL ) {
		chunkfree(otl->features,sizeof(FeatureScriptLangList));
		otl->features = NULL;
	    }
	}
    }

    /* Anchor classes are complicated, because I foolishly failed to distinguish */
    /*  between mark to base and mark to ligature classes. So one AC might have */
    /*  both. If so we need to turn it into two ACs, and have separate lookups */
    /*  for each */
    for ( ac=(AnchorClass1 *) (sf->sf.anchor); ac!=NULL; ac=(AnchorClass1 *) ac->ac.next ) {
	ACHasBaseLig(sf,ac);
	if ( ac->has_ligatures && !ac->has_bases )
	    ac->ac.type = act_mklg;
	else if ( ac->has_ligatures && ac->has_bases )
	    ACDisassociateLigatures(sf,ac);
    }
    for ( ac=(AnchorClass1 *) (sf->sf.anchor); ac!=NULL; ac=(AnchorClass1 *) ac->ac.next ) {
	if ( ac->ac.subtable==NULL ) {
	    otl = CreateACLookup(sf,ac);
	    sub = CreateSubtable(otl,sf);
	    for ( ac2=ac; ac2!=NULL; ac2 = (AnchorClass1 *) ac2->ac.next ) {
		if ( ac2->feature_tag == ac->feature_tag &&
			ac2->script_lang_index == ac->script_lang_index &&
			ac2->flags == ac->flags &&
			ac2->ac.type == ac->ac.type &&
			ac2->merge_with == ac->merge_with )
		    ac2->ac.subtable = sub;
	    }
	}
    }

    /* Now I want to order the gsub lookups. I shan't bother with the gpos */
    /*  lookups because I didn't before */
    for ( otl=sf->sf.gsub_lookups, cnt=0; otl!=NULL; otl=otl->next, ++cnt );
    if ( cnt!=0 ) {
	all = malloc(cnt*sizeof(OTLookup *));
	for ( otl=sf->sf.gsub_lookups, cnt=0; otl!=NULL; otl=otl->next, ++cnt ) {
	    all[cnt] = otl;
	    otl->lookup_index = GSubOrder(sf,otl->features);
	}
	qsort(all,cnt,sizeof(OTLookup *),order_lookups);
	sf->sf.gsub_lookups = all[0];
	for ( i=1; i<cnt; ++i )
	    all[i-1]->next = all[i];
	all[cnt-1]->next = NULL;
	free( all );
    }

    for ( isgpos=0; isgpos<2; ++isgpos ) {
	for ( otl = isgpos ? sf->sf.gpos_lookups : sf->sf.gsub_lookups , cnt=0;
		otl!=NULL; otl = otl->next ) {
	    otl->lookup_index = cnt++;
	    NameOTLookup(otl,&sf->sf);
	}
    }
}