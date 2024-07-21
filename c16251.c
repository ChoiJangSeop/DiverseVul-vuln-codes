rsvg_start_xinclude (RsvgHandle * ctx, RsvgPropertyBag * atts)
{
    RsvgSaxHandlerXinclude *handler;
    const char *href, *parse;
    gboolean success = FALSE;

    href = rsvg_property_bag_lookup (atts, "href");
    if (href == NULL)
        goto fallback;

    parse = rsvg_property_bag_lookup (atts, "parse");
    if (parse && !strcmp (parse, "text")) {
        guint8 *data;
        gsize data_len;
        const char *encoding;

        data = _rsvg_handle_acquire_data (ctx, href, NULL, &data_len, NULL);
        if (data == NULL)
            goto fallback;

        encoding = rsvg_property_bag_lookup (atts, "encoding");
        if (encoding && g_ascii_strcasecmp (encoding, "UTF-8") != 0) {
            char *text_data;
            gsize text_data_len;

            text_data = g_convert (data, data_len, "utf-8", encoding, NULL,
                                   &text_data_len, NULL);
            g_free (data);

            data = text_data;
            data_len = text_data_len;
        }

        rsvg_characters_impl (ctx, (const xmlChar *) data, data_len);

        g_free (data);
    } else {
        /* xml */
        GInputStream *stream;
        GError *err = NULL;
        xmlDocPtr xml_doc;
        xmlParserCtxtPtr xml_parser;
        xmlParserInputBufferPtr buffer;
        xmlParserInputPtr input;

        stream = _rsvg_handle_acquire_stream (ctx, href, NULL, NULL);
        if (stream == NULL)
            goto fallback;

        xml_parser = xmlCreatePushParserCtxt (&rsvgSAXHandlerStruct, ctx, NULL, 0, NULL);

        buffer = _rsvg_xml_input_buffer_new_from_stream (stream, NULL /* cancellable */, XML_CHAR_ENCODING_NONE, &err);
        g_object_unref (stream);

        input = xmlNewIOInputStream (xml_parser, buffer /* adopts */, XML_CHAR_ENCODING_NONE);

        if (xmlPushInput (xml_parser, input) < 0) {
            g_clear_error (&err);
            xmlFreeInputStream (input);
            xmlFreeParserCtxt (xml_parser);
            goto fallback;
        }

        (void) xmlParseDocument (xml_parser);

        xml_doc = xml_parser->myDoc;
        xmlFreeParserCtxt (xml_parser);
        if (xml_doc)
            xmlFreeDoc (xml_doc);

        g_clear_error (&err);
    }

    success = TRUE;

  fallback:

    /* needed to handle xi:fallback */
    handler = g_new0 (RsvgSaxHandlerXinclude, 1);

    handler->super.free = rsvg_xinclude_handler_free;
    handler->super.characters = rsvg_xinclude_handler_characters;
    handler->super.start_element = rsvg_xinclude_handler_start;
    handler->super.end_element = rsvg_xinclude_handler_end;
    handler->prev_handler = ctx->priv->handler;
    handler->ctx = ctx;
    handler->success = success;

    ctx->priv->handler = &handler->super;
}