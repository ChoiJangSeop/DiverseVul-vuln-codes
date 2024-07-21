static void parse_data_for_row_pseudocolor(TGAContext *ctx)
{
	guchar *s = ctx->in->data;
	guint upper_bound = ctx->pbuf->width;
	guchar *p = ctx->pptr;

	for (; upper_bound; upper_bound--, s++) {
		*p++ = ctx->cmap->cols[*s].r;
		*p++ = ctx->cmap->cols[*s].g;
		*p++ = ctx->cmap->cols[*s].b;
		if (ctx->hdr->cmap_bpp == 32)
			*p++ = ctx->cmap->cols[*s].a;
	}
}