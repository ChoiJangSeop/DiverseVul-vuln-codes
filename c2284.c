static void jas_icclut16_destroy(jas_iccattrval_t *attrval)
{
	jas_icclut16_t *lut16 = &attrval->data.lut16;
	if (lut16->clut)
		jas_free(lut16->clut);
	if (lut16->intabs)
		jas_free(lut16->intabs);
	if (lut16->intabsbuf)
		jas_free(lut16->intabsbuf);
	if (lut16->outtabs)
		jas_free(lut16->outtabs);
	if (lut16->outtabsbuf)
		jas_free(lut16->outtabsbuf);
}