xsltParseTemplateContent(xsltStylesheetPtr style, xmlNodePtr templ) {
    if ((style == NULL) || (templ == NULL))
	return;

    /*
    * Detection of handled content of extension instructions.
    */
    if (XSLT_CCTXT(style)->inode->category == XSLT_ELEMENT_CATEGORY_EXTENSION) {
	XSLT_CCTXT(style)->inode->extContentHandled = 1;
    }

    if (templ->children != NULL) {	
	xmlNodePtr child = templ->children;
	/*
	* Process xsl:param elements, which can only occur as the
	* immediate children of xsl:template (well, and of any
	* user-defined extension instruction if needed).
	*/	
	do {
	    if ((child->type == XML_ELEMENT_NODE) &&
		IS_XSLT_ELEM_FAST(child) &&
		IS_XSLT_NAME(child, "param"))
	    {
		XSLT_CCTXT(style)->inode->curChildType = XSLT_FUNC_PARAM;
		xsltParseAnyXSLTElem(XSLT_CCTXT(style), child);
	    } else
		break;
	    child = child->next;
	} while (child != NULL);
	/*
	* Parse the content and register the pattern.
	*/
	xsltParseSequenceConstructor(XSLT_CCTXT(style), child);
    }
}