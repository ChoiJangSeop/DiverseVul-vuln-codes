jpc_enc_tile_t *jpc_enc_tile_create(jpc_enc_cp_t *cp, jas_image_t *image, int tileno)
{
	jpc_enc_tile_t *tile;
	uint_fast32_t htileno;
	uint_fast32_t vtileno;
	uint_fast16_t lyrno;
	uint_fast16_t cmptno;
	jpc_enc_tcmpt_t *tcmpt;

	if (!(tile = jas_malloc(sizeof(jpc_enc_tile_t)))) {
		goto error;
	}

	/* Initialize a few members used in error recovery. */
	tile->tcmpts = 0;
	tile->lyrsizes = 0;
	tile->numtcmpts = cp->numcmpts;
	tile->pi = 0;

	tile->tileno = tileno;
	htileno = tileno % cp->numhtiles;
	vtileno = tileno / cp->numhtiles;

	/* Calculate the coordinates of the top-left and bottom-right
	  corners of the tile. */
	tile->tlx = JAS_MAX(cp->tilegrdoffx + htileno * cp->tilewidth,
	  cp->imgareatlx);
	tile->tly = JAS_MAX(cp->tilegrdoffy + vtileno * cp->tileheight,
	  cp->imgareatly);
	tile->brx = JAS_MIN(cp->tilegrdoffx + (htileno + 1) * cp->tilewidth,
	  cp->refgrdwidth);
	tile->bry = JAS_MIN(cp->tilegrdoffy + (vtileno + 1) * cp->tileheight,
	  cp->refgrdheight);

	/* Initialize some tile coding parameters. */
	tile->intmode = cp->tcp.intmode;
	tile->csty = cp->tcp.csty;
	tile->prg = cp->tcp.prg;
	tile->mctid = cp->tcp.mctid;

	tile->numlyrs = cp->tcp.numlyrs;
	if (!(tile->lyrsizes = jas_malloc(tile->numlyrs *
	  sizeof(uint_fast32_t)))) {
		goto error;
	}
	for (lyrno = 0; lyrno < tile->numlyrs; ++lyrno) {
		tile->lyrsizes[lyrno] = 0;
	}

	/* Allocate an array for the per-tile-component information. */
	if (!(tile->tcmpts = jas_malloc(cp->numcmpts * sizeof(jpc_enc_tcmpt_t)))) {
		goto error;
	}
	/* Initialize a few members critical for error recovery. */
	for (cmptno = 0, tcmpt = tile->tcmpts; cmptno < cp->numcmpts;
	  ++cmptno, ++tcmpt) {
		tcmpt->rlvls = 0;
		tcmpt->tsfb = 0;
		tcmpt->data = 0;
	}
	/* Initialize the per-tile-component information. */
	for (cmptno = 0, tcmpt = tile->tcmpts; cmptno < cp->numcmpts;
	  ++cmptno, ++tcmpt) {
		if (!tcmpt_create(tcmpt, cp, image, tile)) {
			goto error;
		}
	}

	/* Initialize the synthesis weights for the MCT. */
	switch (tile->mctid) {
	case JPC_MCT_RCT:
		tile->tcmpts[0].synweight = jpc_dbltofix(sqrt(3.0));
		tile->tcmpts[1].synweight = jpc_dbltofix(sqrt(0.6875));
		tile->tcmpts[2].synweight = jpc_dbltofix(sqrt(0.6875));
		break;
	case JPC_MCT_ICT:
		tile->tcmpts[0].synweight = jpc_dbltofix(sqrt(3.0000));
		tile->tcmpts[1].synweight = jpc_dbltofix(sqrt(3.2584));
		tile->tcmpts[2].synweight = jpc_dbltofix(sqrt(2.4755));
		break;
	default:
	case JPC_MCT_NONE:
		for (cmptno = 0, tcmpt = tile->tcmpts; cmptno < cp->numcmpts;
		  ++cmptno, ++tcmpt) {
			tcmpt->synweight = JPC_FIX_ONE;
		}
		break;
	}

	if (!(tile->pi = jpc_enc_pi_create(cp, tile))) {
		goto error;
	}

	return tile;

error:

	if (tile) {
		jpc_enc_tile_destroy(tile);
	}
	return 0;
}