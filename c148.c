static int jp2_colr_getdata(jp2_box_t *box, jas_stream_t *in)
{
	jp2_colr_t *colr = &box->data.colr;
	colr->csid = 0;
	colr->iccp = 0;
	colr->iccplen = 0;

	if (jp2_getuint8(in, &colr->method) || jp2_getuint8(in, &colr->pri) ||
	  jp2_getuint8(in, &colr->approx)) {
		return -1;
	}
	switch (colr->method) {
	case JP2_COLR_ENUM:
		if (jp2_getuint32(in, &colr->csid)) {
			return -1;
		}
		break;
	case JP2_COLR_ICC:
		colr->iccplen = box->datalen - 3;
		if (!(colr->iccp = jas_malloc(colr->iccplen * sizeof(uint_fast8_t)))) {
			return -1;
		}
		if (jas_stream_read(in, colr->iccp, colr->iccplen) != colr->iccplen) {
			return -1;
		}
		break;
	}
	return 0;
}