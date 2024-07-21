static gboolean gdk_pixbuf__tga_stop_load(gpointer data, GError **err)
{
	TGAContext *ctx = (TGAContext *) data;
	g_return_val_if_fail(ctx != NULL, FALSE);

	if (ctx->hdr &&
            (ctx->hdr->flags & TGA_ORIGIN_UPPER) == 0 && 
            ctx->run_length_encoded && 
            ctx->pbuf) {
		pixbuf_flip_vertically (ctx->pbuf);
		if (ctx->ufunc)
			(*ctx->ufunc) (ctx->pbuf, 0, 0,
				       ctx->pbuf->width, ctx->pbuf->height,
			       	       ctx->udata);
	}
	g_free (ctx->hdr);
	if (ctx->cmap) {
	  g_free (ctx->cmap->cols);
	  g_free (ctx->cmap);
	}
	if (ctx->pbuf)
		g_object_unref (ctx->pbuf);
	if (ctx->in && ctx->in->size)
		ctx->in = io_buffer_free_segment (ctx->in, ctx->in->size, err);
	if (!ctx->in) {
		g_free (ctx);
		return FALSE;
	}
	io_buffer_free (ctx->in);
	g_free (ctx);
	return TRUE;
}