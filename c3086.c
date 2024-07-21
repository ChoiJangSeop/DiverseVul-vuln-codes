static gboolean fill_in_context(TGAContext *ctx, GError **err)
{
	gboolean alpha;
	guint w, h;

	g_return_val_if_fail(ctx != NULL, FALSE);

	ctx->run_length_encoded =
		((ctx->hdr->type == TGA_TYPE_RLE_PSEUDOCOLOR)
		 || (ctx->hdr->type == TGA_TYPE_RLE_TRUECOLOR)
		 || (ctx->hdr->type == TGA_TYPE_RLE_GRAYSCALE));

	if (ctx->hdr->has_cmap)
		ctx->cmap_size = ((ctx->hdr->cmap_bpp + 7) >> 3) *
			LE16(ctx->hdr->cmap_n_colors);

	alpha = ((ctx->hdr->bpp == 16) || 
		 (ctx->hdr->bpp == 32) ||
		 (ctx->hdr->has_cmap && (ctx->hdr->cmap_bpp == 32)));

	w = LE16(ctx->hdr->width);
	h = LE16(ctx->hdr->height);

	if (ctx->sfunc) {
		gint wi = w;
		gint hi = h;
		
		(*ctx->sfunc) (&wi, &hi, ctx->udata);
		
		if (wi == 0 || hi == 0) 
			return FALSE;
	}

	ctx->pbuf = get_contiguous_pixbuf (w, h, alpha);

	if (!ctx->pbuf) {
		g_set_error_literal(err, GDK_PIXBUF_ERROR, GDK_PIXBUF_ERROR_INSUFFICIENT_MEMORY,
                                    _("Cannot allocate new pixbuf"));
		return FALSE;
	}

	ctx->pbuf_bytes = ctx->pbuf->rowstride * ctx->pbuf->height;
	if ((ctx->hdr->flags & TGA_ORIGIN_UPPER) || ctx->run_length_encoded)
		ctx->pptr = ctx->pbuf->pixels;
	else
		ctx->pptr = ctx->pbuf->pixels + (ctx->pbuf->height - 1)*ctx->pbuf->rowstride;

	if (ctx->hdr->type == TGA_TYPE_PSEUDOCOLOR)
	  ctx->rowstride = ctx->pbuf->width;
	else if (ctx->hdr->type == TGA_TYPE_GRAYSCALE)
	  ctx->rowstride = (alpha ? ctx->pbuf->width * 2 : ctx->pbuf->width);
	else if (ctx->hdr->type == TGA_TYPE_TRUECOLOR)
		ctx->rowstride = ctx->pbuf->rowstride;

	ctx->completed_lines = 0;
	return TRUE;
}