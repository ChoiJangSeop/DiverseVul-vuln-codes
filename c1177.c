xmlCtxtReset(xmlParserCtxtPtr ctxt)
{
    xmlParserInputPtr input;
    xmlDictPtr dict;

    if (ctxt == NULL)
        return;

    dict = ctxt->dict;

    while ((input = inputPop(ctxt)) != NULL) { /* Non consuming */
        xmlFreeInputStream(input);
    }
    ctxt->inputNr = 0;
    ctxt->input = NULL;

    ctxt->spaceNr = 0;
    if (ctxt->spaceTab != NULL) {
	ctxt->spaceTab[0] = -1;
	ctxt->space = &ctxt->spaceTab[0];
    } else {
        ctxt->space = NULL;
    }


    ctxt->nodeNr = 0;
    ctxt->node = NULL;

    ctxt->nameNr = 0;
    ctxt->name = NULL;

    DICT_FREE(ctxt->version);
    ctxt->version = NULL;
    DICT_FREE(ctxt->encoding);
    ctxt->encoding = NULL;
    DICT_FREE(ctxt->directory);
    ctxt->directory = NULL;
    DICT_FREE(ctxt->extSubURI);
    ctxt->extSubURI = NULL;
    DICT_FREE(ctxt->extSubSystem);
    ctxt->extSubSystem = NULL;
    if (ctxt->myDoc != NULL)
        xmlFreeDoc(ctxt->myDoc);
    ctxt->myDoc = NULL;

    ctxt->standalone = -1;
    ctxt->hasExternalSubset = 0;
    ctxt->hasPErefs = 0;
    ctxt->html = 0;
    ctxt->external = 0;
    ctxt->instate = XML_PARSER_START;
    ctxt->token = 0;

    ctxt->wellFormed = 1;
    ctxt->nsWellFormed = 1;
    ctxt->disableSAX = 0;
    ctxt->valid = 1;
#if 0
    ctxt->vctxt.userData = ctxt;
    ctxt->vctxt.error = xmlParserValidityError;
    ctxt->vctxt.warning = xmlParserValidityWarning;
#endif
    ctxt->record_info = 0;
    ctxt->nbChars = 0;
    ctxt->checkIndex = 0;
    ctxt->inSubset = 0;
    ctxt->errNo = XML_ERR_OK;
    ctxt->depth = 0;
    ctxt->charset = XML_CHAR_ENCODING_UTF8;
    ctxt->catalogs = NULL;
    ctxt->nbentities = 0;
    ctxt->sizeentities = 0;
    xmlInitNodeInfoSeq(&ctxt->node_seq);

    if (ctxt->attsDefault != NULL) {
        xmlHashFree(ctxt->attsDefault, (xmlHashDeallocator) xmlFree);
        ctxt->attsDefault = NULL;
    }
    if (ctxt->attsSpecial != NULL) {
        xmlHashFree(ctxt->attsSpecial, NULL);
        ctxt->attsSpecial = NULL;
    }

#ifdef LIBXML_CATALOG_ENABLED
    if (ctxt->catalogs != NULL)
	xmlCatalogFreeLocal(ctxt->catalogs);
#endif
    if (ctxt->lastError.code != XML_ERR_OK)
        xmlResetError(&ctxt->lastError);
}