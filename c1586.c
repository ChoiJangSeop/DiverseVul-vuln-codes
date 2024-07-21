xsltParseTemplateContent(xsltStylesheetPtr style, xmlNodePtr templ) {
    xmlNodePtr cur, delete;
    /*
     * This content comes from the stylesheet
     * For stylesheets, the set of whitespace-preserving
     * element names consists of just xsl:text.
     */
    cur = templ->children;
    delete = NULL;
    while (cur != NULL) {
	if (delete != NULL) {
#ifdef WITH_XSLT_DEBUG_BLANKS
	    xsltGenericDebug(xsltGenericDebugContext,
	     "xsltParseTemplateContent: removing text\n");
#endif
	    xmlUnlinkNode(delete);
	    xmlFreeNode(delete);
	    delete = NULL;
	}
	if (IS_XSLT_ELEM(cur)) {
	    if (IS_XSLT_NAME(cur, "text")) {
		/*
		* TODO: Processing of xsl:text should be moved to
		*   xsltPrecomputeStylesheet(), since otherwise this
		*   will be performed for every multiply included
		*   stylesheet; i.e. this here is not skipped with
		*   the use of the style->nopreproc flag.
		*/
		if (cur->children != NULL) {
		    xmlChar *prop;
		    xmlNodePtr text = cur->children, next;
		    int noesc = 0;
			
		    prop = xmlGetNsProp(cur,
			(const xmlChar *)"disable-output-escaping",
			NULL);
		    if (prop != NULL) {
#ifdef WITH_XSLT_DEBUG_PARSING
			xsltGenericDebug(xsltGenericDebugContext,
			     "Disable escaping: %s\n", text->content);
#endif
			if (xmlStrEqual(prop, (const xmlChar *)"yes")) {
			    noesc = 1;
			} else if (!xmlStrEqual(prop,
						(const xmlChar *)"no")){
			    xsltTransformError(NULL, style, cur,
	     "xsl:text: disable-output-escaping allows only yes or no\n");
			    style->warnings++;

			}
			xmlFree(prop);
		    }

		    while (text != NULL) {
			if (text->type == XML_COMMENT_NODE) {
			    text = text->next;
			    continue;
			}
			if ((text->type != XML_TEXT_NODE) &&
			     (text->type != XML_CDATA_SECTION_NODE)) {
			    xsltTransformError(NULL, style, cur,
		 "xsltParseTemplateContent: xslt:text content problem\n");
			    style->errors++;
			    break;
			}
			if ((noesc) && (text->type != XML_CDATA_SECTION_NODE))
			    text->name = xmlStringTextNoenc;
			text = text->next;
		    }

		    /*
		     * replace xsl:text by the list of childs
		     */
		    if (text == NULL) {
			text = cur->children;
			while (text != NULL) {
			    if ((style->internalized) &&
			        (text->content != NULL) &&
			        (!xmlDictOwns(style->dict, text->content))) {

				/*
				 * internalize the text string
				 */
				if (text->doc->dict != NULL) {
				    const xmlChar *tmp;
				    
				    tmp = xmlDictLookup(text->doc->dict,
				                        text->content, -1);
				    if (tmp != text->content) {
				        xmlNodeSetContent(text, NULL);
					text->content = (xmlChar *) tmp;
				    }
				}
			    }

			    next = text->next;
			    xmlUnlinkNode(text);
			    xmlAddPrevSibling(cur, text);
			    text = next;
			}
		    }
		}
		delete = cur;
		goto skip_children;
	    }
	}
	else if ((cur->ns != NULL) && (style->nsDefs != NULL) &&
	    (xsltCheckExtPrefix(style, cur->ns->prefix)))
	{
	    /*
	     * okay this is an extension element compile it too
	     */
	    xsltStylePreCompute(style, cur);
	} else {
	    /*
	     * This is an element which will be output as part of the
	     * template exectution, precompile AVT if found.
	     */
	    if ((cur->ns == NULL) && (style->defaultAlias != NULL) &&
	    		(cur->type == XML_ELEMENT_NODE)) {
		cur->ns = xmlSearchNsByHref(cur->doc, cur,
			style->defaultAlias);
	    }
	    if (cur->properties != NULL) {
	        xmlAttrPtr attr = cur->properties;

		while (attr != NULL) {
		    xsltCompileAttr(style, attr);
		    attr = attr->next;
		}
	    }
	}
	/*
	 * Skip to next node
	 */
	if (cur->children != NULL) {
	    if (cur->children->type != XML_ENTITY_DECL) {
		cur = cur->children;
		continue;
	    }
	}
skip_children:
	if (cur->next != NULL) {
	    cur = cur->next;
	    continue;
	}
	
	do {
	    cur = cur->parent;
	    if (cur == NULL)
		break;
	    if (cur == templ) {
		cur = NULL;
		break;
	    }
	    if (cur->next != NULL) {
		cur = cur->next;
		break;
	    }
	} while (cur != NULL);
    }
    if (delete != NULL) {
#ifdef WITH_XSLT_DEBUG_PARSING
	xsltGenericDebug(xsltGenericDebugContext,
	 "xsltParseTemplateContent: removing text\n");
#endif
	xmlUnlinkNode(delete);
	xmlFreeNode(delete);
	delete = NULL;
    }

    /*
     * Skip the first params
     */
    cur = templ->children;
    while (cur != NULL) {
	if ((IS_XSLT_ELEM(cur)) && (!(IS_XSLT_NAME(cur, "param"))))
	    break;
	cur = cur->next;
    }

    /*
     * Browse the remainder of the template
     */
    while (cur != NULL) {
	if ((IS_XSLT_ELEM(cur)) && (IS_XSLT_NAME(cur, "param"))) {
	    xmlNodePtr param = cur;

	    xsltTransformError(NULL, style, cur,
		"xsltParseTemplateContent: ignoring misplaced param element\n");
	    if (style != NULL) style->warnings++;
            cur = cur->next;
	    xmlUnlinkNode(param);
	    xmlFreeNode(param);
	} else
	    break;
    }
}