 */
xmlNodePtr
xmlXPathNextFollowing(xmlXPathParserContextPtr ctxt, xmlNodePtr cur) {
    if ((ctxt == NULL) || (ctxt->context == NULL)) return(NULL);
    if (cur != NULL && cur->children != NULL)
        return cur->children ;
    if (cur == NULL) cur = ctxt->context->node;
    if (cur == NULL) return(NULL) ; /* ERROR */
    if (cur->next != NULL) return(cur->next) ;
    do {
        cur = cur->parent;
        if (cur == NULL) break;
        if (cur == (xmlNodePtr) ctxt->context->doc) return(NULL);
        if (cur->next != NULL) return(cur->next);
    } while (cur != NULL);