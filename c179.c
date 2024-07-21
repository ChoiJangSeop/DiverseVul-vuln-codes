int jpc_enc_enccblk(jpc_enc_t *enc, jas_stream_t *out, jpc_enc_tcmpt_t *tcmpt, jpc_enc_band_t *band, jpc_enc_cblk_t *cblk)
{
	jpc_enc_pass_t *pass;
	jpc_enc_pass_t *endpasses;
	int bitpos;
	int n;
	int adjust;
	int ret;
	int passtype;
	int t;
	jpc_bitstream_t *bout;
	jpc_enc_pass_t *termpass;
	jpc_enc_rlvl_t *rlvl;
	int vcausal;
	int segsym;
	int termmode;
	int c;

	bout = 0;
	rlvl = band->rlvl;

	cblk->stream = jas_stream_memopen(0, 0);
	assert(cblk->stream);
	cblk->mqenc = jpc_mqenc_create(JPC_NUMCTXS, cblk->stream);
	assert(cblk->mqenc);
	jpc_mqenc_setctxs(cblk->mqenc, JPC_NUMCTXS, jpc_mqctxs);

	cblk->numpasses = (cblk->numbps > 0) ? (3 * cblk->numbps - 2) : 0;
	if (cblk->numpasses > 0) {
		cblk->passes = jas_malloc(cblk->numpasses * sizeof(jpc_enc_pass_t));
		assert(cblk->passes);
	} else {
		cblk->passes = 0;
	}
	endpasses = (cblk->passes) ? &cblk->passes[cblk->numpasses] : 0;
	for (pass = cblk->passes; pass != endpasses; ++pass) {
		pass->start = 0;
		pass->end = 0;
		pass->term = JPC_ISTERMINATED(pass - cblk->passes, 0, cblk->numpasses, (tcmpt->cblksty & JPC_COX_TERMALL) != 0, (tcmpt->cblksty & JPC_COX_LAZY) != 0);
		pass->type = JPC_SEGTYPE(pass - cblk->passes, 0, (tcmpt->cblksty & JPC_COX_LAZY) != 0);
		pass->lyrno = -1;
if (pass == endpasses - 1) {
assert(pass->term == 1);
	pass->term = 1;
}
	}

	cblk->flags = jas_matrix_create(jas_matrix_numrows(cblk->data) + 2,
	  jas_matrix_numcols(cblk->data) + 2);
	assert(cblk->flags);


	bitpos = cblk->numbps - 1;
	pass = cblk->passes;
	n = cblk->numpasses;
	while (--n >= 0) {

		if (pass->type == JPC_SEG_MQ) {
			/* NOP */
		} else {
			assert(pass->type == JPC_SEG_RAW);
			if (!bout) {
				bout = jpc_bitstream_sopen(cblk->stream, "w");
				assert(bout);
			}
		}

#if 1
		passtype = (pass - cblk->passes + 2) % 3;
#else
		passtype = JPC_PASSTYPE(pass - cblk->passes + 2);
#endif
		pass->start = jas_stream_tell(cblk->stream);
#if 0
assert(jas_stream_tell(cblk->stream) == jas_stream_getrwcount(cblk->stream));
#endif
		assert(bitpos >= 0);
		vcausal = (tcmpt->cblksty & JPC_COX_VSC) != 0;
		segsym = (tcmpt->cblksty & JPC_COX_SEGSYM) != 0;
		if (pass->term) {
			termmode = ((tcmpt->cblksty & JPC_COX_PTERM) ?
			  JPC_MQENC_PTERM : JPC_MQENC_DEFTERM) + 1;
		} else {
			termmode = 0;
		}
		switch (passtype) {
		case JPC_SIGPASS:
			ret = (pass->type == JPC_SEG_MQ) ? jpc_encsigpass(cblk->mqenc,
			  bitpos, band->orient, vcausal, cblk->flags,
			  cblk->data, termmode, &pass->nmsedec) :
			  jpc_encrawsigpass(bout, bitpos, vcausal, cblk->flags,
			  cblk->data, termmode, &pass->nmsedec);
			break;
		case JPC_REFPASS:
			ret = (pass->type == JPC_SEG_MQ) ? jpc_encrefpass(cblk->mqenc,
			  bitpos, vcausal, cblk->flags, cblk->data, termmode,
			  &pass->nmsedec) : jpc_encrawrefpass(bout, bitpos,
			  vcausal, cblk->flags, cblk->data, termmode,
			  &pass->nmsedec);
			break;
		case JPC_CLNPASS:
			assert(pass->type == JPC_SEG_MQ);
			ret = jpc_encclnpass(cblk->mqenc, bitpos, band->orient,
			  vcausal, segsym, cblk->flags, cblk->data, termmode,
			  &pass->nmsedec);
			break;
		default:
			assert(0);
			break;
		}

		if (pass->type == JPC_SEG_MQ) {
			if (pass->term) {
				jpc_mqenc_init(cblk->mqenc);
			}
			jpc_mqenc_getstate(cblk->mqenc, &pass->mqencstate);
			pass->end = jas_stream_tell(cblk->stream);
			if (tcmpt->cblksty & JPC_COX_RESET) {
				jpc_mqenc_setctxs(cblk->mqenc, JPC_NUMCTXS, jpc_mqctxs);
			}
		} else {
			if (pass->term) {
				if (jpc_bitstream_pending(bout)) {
					jpc_bitstream_outalign(bout, 0x2a);
				}
				jpc_bitstream_close(bout);
				bout = 0;
				pass->end = jas_stream_tell(cblk->stream);
			} else {
				pass->end = jas_stream_tell(cblk->stream) +
				  jpc_bitstream_pending(bout);
/* NOTE - This will not work.  need to adjust by # of pending output bytes */
			}
		}
#if 0
/* XXX - This assertion fails sometimes when various coding modes are used.
This seems to be harmless, but why does it happen at all? */
assert(jas_stream_tell(cblk->stream) == jas_stream_getrwcount(cblk->stream));
#endif

		pass->wmsedec = jpc_fixtodbl(band->rlvl->tcmpt->synweight) *
		  jpc_fixtodbl(band->rlvl->tcmpt->synweight) *
		  jpc_fixtodbl(band->synweight) *
		  jpc_fixtodbl(band->synweight) *
		  jpc_fixtodbl(band->absstepsize) * jpc_fixtodbl(band->absstepsize) *
		  ((double) (1 << bitpos)) * ((double)(1 << bitpos)) *
		  jpc_fixtodbl(pass->nmsedec);
		pass->cumwmsedec = pass->wmsedec;
		if (pass != cblk->passes) {
			pass->cumwmsedec += pass[-1].cumwmsedec;
		}
		if (passtype == JPC_CLNPASS) {
			--bitpos;
		}
		++pass;
	}

#if 0
dump_passes(cblk->passes, cblk->numpasses, cblk);
#endif

	n = 0;
	endpasses = (cblk->passes) ? &cblk->passes[cblk->numpasses] : 0;
	for (pass = cblk->passes; pass != endpasses; ++pass) {
		if (pass->start < n) {
			pass->start = n;
		}
		if (pass->end < n) {
			pass->end = n;
		}
		if (!pass->term) {
			termpass = pass;
			while (termpass - pass < cblk->numpasses &&
			  !termpass->term) {
				++termpass;
			}
			if (pass->type == JPC_SEG_MQ) {
				t = (pass->mqencstate.lastbyte == 0xff) ? 1 : 0;
				if (pass->mqencstate.ctreg >= 5) {
					adjust = 4 + t;
				} else {
					adjust = 5 + t;
				}
				pass->end += adjust;
			}
			if (pass->end > termpass->end) {
				pass->end = termpass->end;
			}
			if ((c = getthebyte(cblk->stream, pass->end - 1)) == EOF) {
				abort();
			}
			if (c == 0xff) {
				++pass->end;
			}
			n = JAS_MAX(n, pass->end);
		} else {
			n = JAS_MAX(n, pass->end);
		}
	}

#if 0
dump_passes(cblk->passes, cblk->numpasses, cblk);
#endif

	if (bout) {
		jpc_bitstream_close(bout);
	}

	return 0;
}