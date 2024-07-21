rsvg_handle_read_stream_sync (RsvgHandle   *handle,
                              GInputStream *stream,
                              GCancellable *cancellable,
                              GError      **error)
{
    RsvgHandlePrivate *priv;
    xmlParserInputBufferPtr buffer;
    xmlParserInputPtr input;
    int result;
    xmlDocPtr doc;
    GError *err = NULL;
    gboolean res = FALSE;

    g_return_val_if_fail (RSVG_IS_HANDLE (handle), FALSE);
    g_return_val_if_fail (G_IS_INPUT_STREAM (stream), FALSE);
    g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), FALSE);
    g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

    priv = handle->priv;

    priv->error = &err;
    priv->cancellable = cancellable ? g_object_ref (cancellable) : NULL;
    if (priv->ctxt == NULL) {
        priv->ctxt = xmlCreatePushParserCtxt (&rsvgSAXHandlerStruct, handle, NULL, 0,
                                              rsvg_handle_get_base_uri (handle));

        /* if false, external entities work, but internal ones don't. if true, internal entities
           work, but external ones don't. favor internal entities, in order to not cause a
           regression */
        /* FIXMEchpe: FIX THIS! */
        priv->ctxt->replaceEntities = TRUE;
    }

    buffer = _rsvg_xml_input_buffer_new_from_stream (stream, cancellable, XML_CHAR_ENCODING_NONE, &err);
    input = xmlNewIOInputStream (priv->ctxt, buffer, XML_CHAR_ENCODING_NONE);

    if (xmlPushInput (priv->ctxt, input) < 0) {
        rsvg_set_error (error, priv->ctxt);
        xmlFreeInputStream (input);
        goto out;
    }

    result = xmlParseDocument (priv->ctxt);
    if (result != 0) {
        if (err)
            g_propagate_error (error, err);
        else
            rsvg_set_error (error, handle->priv->ctxt);

        goto out;
    }

    if (err != NULL) {
        g_propagate_error (error, err);
        goto out;
    }

    doc = priv->ctxt->myDoc;
    xmlFreeParserCtxt (priv->ctxt);
    priv->ctxt = NULL;

    xmlFreeDoc (doc);

    rsvg_defs_resolve_all (priv->defs);
    priv->finished = TRUE;

    res = TRUE;

  out:

    priv->error = NULL;
    g_clear_object (&priv->cancellable);

    return res;
}