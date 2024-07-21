xmlParserEntityCheck(xmlParserCtxtPtr ctxt, size_t size,
                     xmlEntityPtr ent, size_t replacement)
{
    size_t consumed = 0;

    if ((ctxt == NULL) || (ctxt->options & XML_PARSE_HUGE))
        return (0);
    if (ctxt->lastError.code == XML_ERR_ENTITY_LOOP)
        return (1);
    if (replacement != 0) {
	if (replacement < XML_MAX_TEXT_LENGTH)
	    return(0);

        /*
	 * If the volume of entity copy reaches 10 times the
	 * amount of parsed data and over the large text threshold
	 * then that's very likely to be an abuse.
	 */
        if (ctxt->input != NULL) {
	    consumed = ctxt->input->consumed +
	               (ctxt->input->cur - ctxt->input->base);
	}
        consumed += ctxt->sizeentities;

        if (replacement < XML_PARSER_NON_LINEAR * consumed)
	    return(0);
    } else if (size != 0) {
        /*
         * Do the check based on the replacement size of the entity
         */
        if (size < XML_PARSER_BIG_ENTITY)
	    return(0);

        /*
         * A limit on the amount of text data reasonably used
         */
        if (ctxt->input != NULL) {
            consumed = ctxt->input->consumed +
                (ctxt->input->cur - ctxt->input->base);
        }
        consumed += ctxt->sizeentities;

        if ((size < XML_PARSER_NON_LINEAR * consumed) &&
	    (ctxt->nbentities * 3 < XML_PARSER_NON_LINEAR * consumed))
            return (0);
    } else if (ent != NULL) {
        /*
         * use the number of parsed entities in the replacement
         */
        size = ent->checked / 2;

        /*
         * The amount of data parsed counting entities size only once
         */
        if (ctxt->input != NULL) {
            consumed = ctxt->input->consumed +
                (ctxt->input->cur - ctxt->input->base);
        }
        consumed += ctxt->sizeentities;

        /*
         * Check the density of entities for the amount of data
	 * knowing an entity reference will take at least 3 bytes
         */
        if (size * 3 < consumed * XML_PARSER_NON_LINEAR)
            return (0);
    } else {
        /*
         * strange we got no data for checking just return
         */
        return (0);
    }
    xmlFatalErr(ctxt, XML_ERR_ENTITY_LOOP, NULL);
    return (1);
}