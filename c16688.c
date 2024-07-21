rsvg_drawing_ctx_free (RsvgDrawingCtx * handle)
{
    rsvg_render_free (handle->render);

    rsvg_state_free_all (handle->state);

	/* the drawsub stack's nodes are owned by the ->defs */
	g_slist_free (handle->drawsub_stack);

    g_slist_free (handle->ptrs);
	
    if (handle->base_uri)
        g_free (handle->base_uri);

    if (handle->pango_context != NULL)
        g_object_unref (handle->pango_context);

    g_free (handle);
}