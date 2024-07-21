valuePop(xmlXPathParserContextPtr ctxt)
{
    xmlXPathObjectPtr ret;

    if ((ctxt == NULL) || (ctxt->valueNr <= 0))
        return (NULL);
    ctxt->valueNr--;
    if (ctxt->valueNr > 0)
        ctxt->value = ctxt->valueTab[ctxt->valueNr - 1];
    else
        ctxt->value = NULL;
    ret = ctxt->valueTab[ctxt->valueNr];
    ctxt->valueTab[ctxt->valueNr] = NULL;
    return (ret);
}