rsvg_handle_write_impl (RsvgHandle * handle, const guchar * buf, gsize count, GError ** error)
{
    GError *real_error = NULL;
    int result;

    rsvg_return_val_if_fail (handle != NULL, FALSE, error);

    handle->priv->error = &real_error;
    if (handle->priv->ctxt == NULL) {
        handle->priv->ctxt = xmlCreatePushParserCtxt (&rsvgSAXHandlerStruct, handle, NULL, 0,
                                                      rsvg_handle_get_base_uri (handle));

        /* if false, external entities work, but internal ones don't. if true, internal entities
           work, but external ones don't. favor internal entities, in order to not cause a
           regression */
        handle->priv->ctxt->replaceEntities = TRUE;
    }

    result = xmlParseChunk (handle->priv->ctxt, (char *) buf, count, 0);
    if (result != 0) {
        rsvg_set_error (error, handle->priv->ctxt);
        return FALSE;
    }

    handle->priv->error = NULL;

    if (real_error != NULL) {
        g_propagate_error (error, real_error);
        return FALSE;
    }

    return TRUE;
}