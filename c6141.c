int jpc_enc_encpkt(jpc_enc_t *enc, jas_stream_t *out, int compno, int lvlno, int prcno, int lyrno)
{
	jpc_enc_tcmpt_t *comp;
	jpc_enc_rlvl_t *lvl;
	jpc_enc_band_t *band;
	jpc_enc_band_t *endbands;
	jpc_enc_cblk_t *cblk;
	jpc_enc_cblk_t *endcblks;
	jpc_bitstream_t *outb;
	jpc_enc_pass_t *pass;
	jpc_enc_pass_t *startpass;
	jpc_enc_pass_t *lastpass;
	jpc_enc_pass_t *endpass;
	jpc_enc_pass_t *endpasses;
	int i;
	int included;
	int ret;
	jpc_tagtreenode_t *leaf;
	int n;
	int t1;
	int t2;
	int adjust;
	int maxadjust;
	int datalen;
	int numnewpasses;
	int passcount;
	jpc_enc_tile_t *tile;
	jpc_enc_prc_t *prc;
	jpc_enc_cp_t *cp;
	jpc_ms_t *ms;

	JAS_DBGLOG(10, ("encoding packet begin %d %d %d %d\n", compno, lvlno,
	  prcno, lyrno));

	tile = enc->curtile;
	cp = enc->cp;

	if (cp->tcp.csty & JPC_COD_SOP) {
		if (!(ms = jpc_ms_create(JPC_MS_SOP))) {
			return -1;
		}
		ms->parms.sop.seqno = jpc_pi_getind(tile->pi);
		if (jpc_putms(out, enc->cstate, ms)) {
			return -1;
		}
		jpc_ms_destroy(ms);
	}

	if (!(outb = jpc_bitstream_sopen(out, "w+"))) {
		abort();
	}

	if (jpc_bitstream_putbit(outb, 1) == EOF) {
		return -1;
	}
	//JAS_DBGLOG(10, ("\n"));
	JAS_DBGLOG(10, ("present.\n"));

	comp = &tile->tcmpts[compno];
	lvl = &comp->rlvls[lvlno];
	endbands = &lvl->bands[lvl->numbands];
	for (band = lvl->bands; band != endbands; ++band) {
		if (!band->data) {
			continue;
		}
		prc = &band->prcs[prcno];
		if (!prc->cblks) {
			continue;
		}

		endcblks = &prc->cblks[prc->numcblks];
		for (cblk = prc->cblks; cblk != endcblks; ++cblk) {
			if (!lyrno) {
				leaf = jpc_tagtree_getleaf(prc->nlibtree, cblk - prc->cblks);
				jpc_tagtree_setvalue(prc->nlibtree, leaf, cblk->numimsbs);
			}
			pass = cblk->curpass;
			included = (pass && pass->lyrno == lyrno);
			if (included && (!cblk->numencpasses)) {
				assert(pass->lyrno == lyrno);
				leaf = jpc_tagtree_getleaf(prc->incltree,
				  cblk - prc->cblks);
				jpc_tagtree_setvalue(prc->incltree, leaf, pass->lyrno);
			}
		}

		endcblks = &prc->cblks[prc->numcblks];
		for (cblk = prc->cblks; cblk != endcblks; ++cblk) {
			pass = cblk->curpass;
			included = (pass && pass->lyrno == lyrno);
			if (!cblk->numencpasses) {
				leaf = jpc_tagtree_getleaf(prc->incltree,
				  cblk - prc->cblks);
				if (jpc_tagtree_encode(prc->incltree, leaf, lyrno + 1, outb) <
				  0) {
					return -1;
				}
			} else {
				if (jpc_bitstream_putbit(outb, included) == EOF) {
					return -1;
				}
			}
			JAS_DBGLOG(10, ("included=%d ", included));
			if (!included) {
				continue;
			}
			if (!cblk->numencpasses) {
				i = 1;
				leaf = jpc_tagtree_getleaf(prc->nlibtree, cblk - prc->cblks);
				for (;;) {
					if ((ret = jpc_tagtree_encode(prc->nlibtree, leaf, i,
					  outb)) < 0) {
						return -1;
					}
					if (ret) {
						break;
					}
					++i;
				}
				assert(leaf->known_ && i == leaf->value_ + 1);
			}

			endpasses = &cblk->passes[cblk->numpasses];
			startpass = pass;
			endpass = startpass;
			while (endpass != endpasses && endpass->lyrno == lyrno){
				++endpass;
			}
			numnewpasses = endpass - startpass;
			if (jpc_putnumnewpasses(outb, numnewpasses)) {
				return -1;
			}
			JAS_DBGLOG(10, ("numnewpasses=%d ", numnewpasses));

			lastpass = endpass - 1;
			n = startpass->start;
			passcount = 1;
			maxadjust = 0;
			for (pass = startpass; pass != endpass; ++pass) {
				if (pass->term || pass == lastpass) {
					datalen = pass->end - n;
					t1 = jpc_firstone(datalen) + 1;
					t2 = cblk->numlenbits + jpc_floorlog2(passcount);
					adjust = JAS_MAX(t1 - t2, 0);
					maxadjust = JAS_MAX(adjust, maxadjust);
					n += datalen;
					passcount = 1;
				} else {
					++passcount;
				}
			}
			if (jpc_putcommacode(outb, maxadjust)) {
				return -1;
			}
			cblk->numlenbits += maxadjust;

			lastpass = endpass - 1;
			n = startpass->start;
			passcount = 1;
			for (pass = startpass; pass != endpass; ++pass) {
				if (pass->term || pass == lastpass) {
					datalen = pass->end - n;
					assert(jpc_firstone(datalen) < cblk->numlenbits +
					  jpc_floorlog2(passcount));
					if (jpc_bitstream_putbits(outb, cblk->numlenbits +
					  jpc_floorlog2(passcount), datalen) == EOF) {
						return -1;
					}
					n += datalen;
					passcount = 1;
				} else {
					++passcount;
				}
			}
		}
	}

	jpc_bitstream_outalign(outb, 0);
	jpc_bitstream_close(outb);

	if (cp->tcp.csty & JPC_COD_EPH) {
		if (!(ms = jpc_ms_create(JPC_MS_EPH))) {
			return -1;
		}
		if (jpc_putms(out, enc->cstate, ms)) {
			return -1;
		}
		jpc_ms_destroy(ms);
	}

	comp = &tile->tcmpts[compno];
	lvl = &comp->rlvls[lvlno];
	endbands = &lvl->bands[lvl->numbands];
	for (band = lvl->bands; band != endbands; ++band) {
		if (!band->data) {
			continue;
		}
		prc = &band->prcs[prcno];
		if (!prc->cblks) {
			continue;
		}
		endcblks = &prc->cblks[prc->numcblks];
		for (cblk = prc->cblks; cblk != endcblks; ++cblk) {
			pass = cblk->curpass;

			if (!pass) {
				continue;
			}
			if (pass->lyrno != lyrno) {
				assert(pass->lyrno < 0 || pass->lyrno > lyrno);
				continue;
			}

			endpasses = &cblk->passes[cblk->numpasses];
			startpass = pass;
			endpass = startpass;
			while (endpass != endpasses && endpass->lyrno == lyrno){
				++endpass;
			}
			lastpass = endpass - 1;
			numnewpasses = endpass - startpass;

			jas_stream_seek(cblk->stream, startpass->start, SEEK_SET);
			assert(jas_stream_tell(cblk->stream) == startpass->start);
			if (jas_stream_copy(out, cblk->stream, lastpass->end -
			  startpass->start)) {
				return -1;
			}
			cblk->curpass = (endpass != endpasses) ? endpass : 0;
			cblk->numencpasses += numnewpasses;

		}
	}

	JAS_DBGLOG(10, ("encoding packet end\n"));

	return 0;
}