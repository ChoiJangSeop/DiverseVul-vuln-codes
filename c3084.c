static void write_rle_data(TGAContext *ctx, TGAColor *color, guint *rle_count)
{
	for (; *rle_count; (*rle_count)--) {
		g_memmove(ctx->pptr, (guchar *) color, ctx->pbuf->n_channels);
		ctx->pptr += ctx->pbuf->n_channels;
		ctx->pbuf_bytes_done += ctx->pbuf->n_channels;
		if (ctx->pbuf_bytes_done == ctx->pbuf_bytes)
			return;
	}
}