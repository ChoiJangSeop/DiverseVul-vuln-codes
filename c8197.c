pdf_filter_BDC(fz_context *ctx, pdf_processor *proc, const char *tag, pdf_obj *raw, pdf_obj *cooked)
{
	/* Create a tag, and push it onto pending_tags. If it gets
	 * flushed to the stream, it'll be moved from there onto
	 * current_tags. */
	pdf_filter_processor *p = (pdf_filter_processor*)proc;
	tag_record *bdc = fz_malloc_struct(ctx, tag_record);
	pdf_obj *mcid;

	fz_try(ctx)
	{
		bdc->bdc = 1;
		bdc->tag = fz_strdup(ctx, tag);
		bdc->raw = pdf_keep_obj(ctx, raw);
		bdc->cooked = pdf_keep_obj(ctx, raw);
	}
	fz_catch(ctx)
	{
		fz_free(ctx, bdc->tag);
		pdf_drop_obj(ctx, bdc->raw);
		pdf_drop_obj(ctx, bdc->cooked);
		fz_free(ctx, bdc);
		fz_rethrow(ctx);
	}
	bdc->prev = p->pending_tags;
	p->pending_tags = bdc;

	/* Look to see if this has an mcid object */
	mcid = pdf_dict_get(ctx, cooked, PDF_NAME(MCID));
	if (!pdf_is_number(ctx, mcid))
		return;
	bdc->mcid_num = pdf_to_int(ctx, mcid);
	bdc->mcid_obj = pdf_keep_obj(ctx, pdf_array_get(ctx, p->structarray, bdc->mcid_num));
	bdc->alt.utf8 = pdf_new_utf8_from_pdf_string_obj(ctx, pdf_dict_get(ctx, bdc->mcid_obj, PDF_NAME(Alt)));
	bdc->actualtext.utf8 = pdf_new_utf8_from_pdf_string_obj(ctx, pdf_dict_get(ctx, bdc->mcid_obj, PDF_NAME(ActualText)));
}