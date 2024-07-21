xsltAddKey(xsltStylesheetPtr style, const xmlChar *name,
	   const xmlChar *nameURI, const xmlChar *match,
	   const xmlChar *use, xmlNodePtr inst) {
    xsltKeyDefPtr key;
    xmlChar *pattern = NULL;
    int current, end, start, i = 0;

    if ((style == NULL) || (name == NULL) || (match == NULL) || (use == NULL))
	return(-1);

#ifdef WITH_XSLT_DEBUG_KEYS
    xsltGenericDebug(xsltGenericDebugContext,
	"Add key %s, match %s, use %s\n", name, match, use);
#endif

    key = xsltNewKeyDef(name, nameURI);
    key->match = xmlStrdup(match);
    key->use = xmlStrdup(use);
    key->inst = inst;
    key->nsList = xmlGetNsList(inst->doc, inst);
    if (key->nsList != NULL) {
        while (key->nsList[i] != NULL)
	    i++;
    }
    key->nsNr = i;

    /*
     * Split the | and register it as as many keys
     */
    current = end = 0;
    while (match[current] != 0) {
	start = current;
	while (IS_BLANK_CH(match[current]))
	    current++;
	end = current;
	while ((match[end] != 0) && (match[end] != '|')) {
	    if (match[end] == '[') {
	        end = skipPredicate(match, end);
		if (end <= 0) {
		    xsltTransformError(NULL, style, inst,
		                       "key pattern is malformed: %s",
				       key->match);
		    if (style != NULL) style->errors++;
		    goto error;
		}
	    } else
		end++;
	}
	if (current == end) {
	    xsltTransformError(NULL, style, inst,
			       "key pattern is empty\n");
	    if (style != NULL) style->errors++;
	    goto error;
	}
	if (match[start] != '/') {
	    pattern = xmlStrcat(pattern, (xmlChar *)"//");
	    if (pattern == NULL) {
		if (style != NULL) style->errors++;
		goto error;
	    }
	}
	pattern = xmlStrncat(pattern, &match[start], end - start);
	if (pattern == NULL) {
	    if (style != NULL) style->errors++;
	    goto error;
	}

	if (match[end] == '|') {
	    pattern = xmlStrcat(pattern, (xmlChar *)"|");
	    end++;
	}
	current = end;
    }
#ifdef WITH_XSLT_DEBUG_KEYS
    xsltGenericDebug(xsltGenericDebugContext,
	"   resulting pattern %s\n", pattern);
#endif
    /*
    * XSLT-1: "It is an error for the value of either the use
    *  attribute or the match attribute to contain a
    *  VariableReference."
    * TODO: We should report a variable-reference at compile-time.
    *   Maybe a search for "$", if it occurs outside of quotation
    *   marks, could be sufficient.
    */
#ifdef XML_XPATH_NOVAR
    key->comp = xsltXPathCompileFlags(style, pattern, XML_XPATH_NOVAR);
#else
    key->comp = xsltXPathCompile(style, pattern);
#endif
    if (key->comp == NULL) {
	xsltTransformError(NULL, style, inst,
		"xsl:key : XPath pattern compilation failed '%s'\n",
		         pattern);
	if (style != NULL) style->errors++;
    }
#ifdef XML_XPATH_NOVAR
    key->usecomp = xsltXPathCompileFlags(style, use, XML_XPATH_NOVAR);
#else
    key->usecomp = xsltXPathCompile(style, use);
#endif
    if (key->usecomp == NULL) {
	xsltTransformError(NULL, style, inst,
		"xsl:key : XPath pattern compilation failed '%s'\n",
		         use);
	if (style != NULL) style->errors++;
    }

    /*
     * Sometimes the stylesheet writer use the order to ease the
     * resolution of keys when they are dependant, keep the provided
     * order so add the new one at the end.
     */
    if (style->keys == NULL) {
	style->keys = key;
    } else {
        xsltKeyDefPtr prev = style->keys;

	while (prev->next != NULL)
	    prev = prev->next;

	prev->next = key;
    }
    key->next = NULL;

error:
    if (pattern != NULL)
	xmlFree(pattern);
    return(0);
}