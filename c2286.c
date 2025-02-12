static void jas_icclut8_destroy(jas_iccattrval_t *attrval)
{
	jas_icclut8_t *lut8 = &attrval->data.lut8;
	if (lut8->clut)
		jas_free(lut8->clut);
	if (lut8->intabs)
		jas_free(lut8->intabs);
	if (lut8->intabsbuf)
		jas_free(lut8->intabsbuf);
	if (lut8->outtabs)
		jas_free(lut8->outtabs);
	if (lut8->outtabsbuf)
		jas_free(lut8->outtabsbuf);
}