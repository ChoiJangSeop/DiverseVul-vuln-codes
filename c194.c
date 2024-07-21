jpc_pi_t *jpc_dec_pi_create(jpc_dec_t *dec, jpc_dec_tile_t *tile)
{
	jpc_pi_t *pi;
	int compno;
	jpc_picomp_t *picomp;
	jpc_pirlvl_t *pirlvl;
	jpc_dec_tcomp_t *tcomp;
	int rlvlno;
	jpc_dec_rlvl_t *rlvl;
	int prcno;
	int *prclyrno;
	jpc_dec_cmpt_t *cmpt;

	if (!(pi = jpc_pi_create0())) {
		return 0;
	}
	pi->numcomps = dec->numcomps;
	if (!(pi->picomps = jas_malloc(pi->numcomps * sizeof(jpc_picomp_t)))) {
		jpc_pi_destroy(pi);
		return 0;
	}
	for (compno = 0, picomp = pi->picomps; compno < pi->numcomps; ++compno,
	  ++picomp) {
		picomp->pirlvls = 0;
	}

	for (compno = 0, tcomp = tile->tcomps, picomp = pi->picomps;
	  compno < pi->numcomps; ++compno, ++tcomp, ++picomp) {
		picomp->numrlvls = tcomp->numrlvls;
		if (!(picomp->pirlvls = jas_malloc(picomp->numrlvls *
		  sizeof(jpc_pirlvl_t)))) {
			jpc_pi_destroy(pi);
			return 0;
		}
		for (rlvlno = 0, pirlvl = picomp->pirlvls; rlvlno <
		  picomp->numrlvls; ++rlvlno, ++pirlvl) {
			pirlvl->prclyrnos = 0;
		}
		for (rlvlno = 0, pirlvl = picomp->pirlvls, rlvl = tcomp->rlvls;
		  rlvlno < picomp->numrlvls; ++rlvlno, ++pirlvl, ++rlvl) {
/* XXX sizeof(long) should be sizeof different type */
			pirlvl->numprcs = rlvl->numprcs;
			if (!(pirlvl->prclyrnos = jas_malloc(pirlvl->numprcs *
			  sizeof(long)))) {
				jpc_pi_destroy(pi);
				return 0;
			}
		}
	}

	pi->maxrlvls = 0;
	for (compno = 0, tcomp = tile->tcomps, picomp = pi->picomps, cmpt =
	  dec->cmpts; compno < pi->numcomps; ++compno, ++tcomp, ++picomp,
	  ++cmpt) {
		picomp->hsamp = cmpt->hstep;
		picomp->vsamp = cmpt->vstep;
		for (rlvlno = 0, pirlvl = picomp->pirlvls, rlvl = tcomp->rlvls;
		  rlvlno < picomp->numrlvls; ++rlvlno, ++pirlvl, ++rlvl) {
			pirlvl->prcwidthexpn = rlvl->prcwidthexpn;
			pirlvl->prcheightexpn = rlvl->prcheightexpn;
			for (prcno = 0, prclyrno = pirlvl->prclyrnos;
			  prcno < pirlvl->numprcs; ++prcno, ++prclyrno) {
				*prclyrno = 0;
			}
			pirlvl->numhprcs = rlvl->numhprcs;
		}
		if (pi->maxrlvls < tcomp->numrlvls) {
			pi->maxrlvls = tcomp->numrlvls;
		}
	}

	pi->numlyrs = tile->cp->numlyrs;
	pi->xstart = tile->xstart;
	pi->ystart = tile->ystart;
	pi->xend = tile->xend;
	pi->yend = tile->yend;

	pi->picomp = 0;
	pi->pirlvl = 0;
	pi->x = 0;
	pi->y = 0;
	pi->compno = 0;
	pi->rlvlno = 0;
	pi->prcno = 0;
	pi->lyrno = 0;
	pi->xstep = 0;
	pi->ystep = 0;

	pi->pchgno = -1;

	pi->defaultpchg.prgord = tile->cp->prgord;
	pi->defaultpchg.compnostart = 0;
	pi->defaultpchg.compnoend = pi->numcomps;
	pi->defaultpchg.rlvlnostart = 0;
	pi->defaultpchg.rlvlnoend = pi->maxrlvls;
	pi->defaultpchg.lyrnoend = pi->numlyrs;
	pi->pchg = 0;

	pi->valid = 0;

	return pi;
}