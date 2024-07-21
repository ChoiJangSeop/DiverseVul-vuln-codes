static guint parse_rle_data_pseudocolor(TGAContext *ctx)
{
	guint rle_num, raw_num;
	guchar *s, tag;
	guint n;

	g_return_val_if_fail(ctx->in->size > 0, 0);
	s = ctx->in->data;

	for (n = 0; n < ctx->in->size; ) {
		tag = *s;
		s++, n++;
		if (tag & 0x80) {
			if (n == ctx->in->size) {
				return --n;
			} else {
				rle_num = (tag & 0x7f) + 1;
				write_rle_data(ctx, &ctx->cmap->cols[*s], &rle_num);
				s++, n++;
				if (ctx->pbuf_bytes_done == ctx->pbuf_bytes) {
					ctx->done = TRUE;
					return n;
				}
			}
		} else {
			raw_num = tag + 1;
			if (n + raw_num >= ctx->in->size) {
				return --n;
			} else {
				for (; raw_num; raw_num--) {
					*ctx->pptr++ =
						ctx->cmap->cols[*s].r;
					*ctx->pptr++ =
						ctx->cmap->cols[*s].g;
					*ctx->pptr++ =
						ctx->cmap->cols[*s].b;
					if (ctx->pbuf->n_channels == 4)
						*ctx->pptr++ = ctx->cmap->cols[*s].a;
					s++, n++;
					ctx->pbuf_bytes_done += ctx->pbuf->n_channels;
					if (ctx->pbuf_bytes_done == ctx->pbuf_bytes) {
						ctx->done = TRUE;
						return n;
					}
				}
			}
		}
	}

	if (ctx->pbuf_bytes_done == ctx->pbuf_bytes) 
		ctx->done = TRUE;
	
	return n;
}