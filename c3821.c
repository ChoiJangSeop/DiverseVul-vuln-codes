xmlSchemaComplexTypeErr(xmlSchemaAbstractCtxtPtr actxt,
		        xmlParserErrors error,
		        xmlNodePtr node,
			xmlSchemaTypePtr type ATTRIBUTE_UNUSED,
			const char *message,
			int nbval,
			int nbneg,
			xmlChar **values)
{
    xmlChar *str = NULL, *msg = NULL;
    xmlChar *localName, *nsName;
    const xmlChar *cur, *end;
    int i;

    xmlSchemaFormatNodeForError(&msg, actxt, node);
    msg = xmlStrcat(msg, (const xmlChar *) message);
    msg = xmlStrcat(msg, BAD_CAST ".");
    /*
    * Note that is does not make sense to report that we have a
    * wildcard here, since the wildcard might be unfolded into
    * multiple transitions.
    */
    if (nbval + nbneg > 0) {
	if (nbval + nbneg > 1) {
	    str = xmlStrdup(BAD_CAST " Expected is one of ( ");
	} else
	    str = xmlStrdup(BAD_CAST " Expected is ( ");
	nsName = NULL;

	for (i = 0; i < nbval + nbneg; i++) {
	    cur = values[i];
	    if (cur == NULL)
	        continue;
	    if ((cur[0] == 'n') && (cur[1] == 'o') && (cur[2] == 't') &&
	        (cur[3] == ' ')) {
		cur += 4;
		str = xmlStrcat(str, BAD_CAST "##other");
	    }
	    /*
	    * Get the local name.
	    */
	    localName = NULL;

	    end = cur;
	    if (*end == '*') {
		localName = xmlStrdup(BAD_CAST "*");
		end++;
	    } else {
		while ((*end != 0) && (*end != '|'))
		    end++;
		localName = xmlStrncat(localName, BAD_CAST cur, end - cur);
	    }
	    if (*end != 0) {
		end++;
		/*
		* Skip "*|*" if they come with negated expressions, since
		* they represent the same negated wildcard.
		*/
		if ((nbneg == 0) || (*end != '*') || (*localName != '*')) {
		    /*
		    * Get the namespace name.
		    */
		    cur = end;
		    if (*end == '*') {
			nsName = xmlStrdup(BAD_CAST "{*}");
		    } else {
			while (*end != 0)
			    end++;

			if (i >= nbval)
			    nsName = xmlStrdup(BAD_CAST "{##other:");
			else
			    nsName = xmlStrdup(BAD_CAST "{");

			nsName = xmlStrncat(nsName, BAD_CAST cur, end - cur);
			nsName = xmlStrcat(nsName, BAD_CAST "}");
		    }
		    str = xmlStrcat(str, BAD_CAST nsName);
		    FREE_AND_NULL(nsName)
		} else {
		    FREE_AND_NULL(localName);
		    continue;
		}
	    }
	    str = xmlStrcat(str, BAD_CAST localName);
	    FREE_AND_NULL(localName);

	    if (i < nbval + nbneg -1)
		str = xmlStrcat(str, BAD_CAST ", ");
	}
	str = xmlStrcat(str, BAD_CAST " ).\n");
	msg = xmlStrcat(msg, BAD_CAST str);
	FREE_AND_NULL(str)
    } else
      msg = xmlStrcat(msg, BAD_CAST "\n");
    xmlSchemaErr(actxt, error, node, (const char *) msg, NULL, NULL);
    xmlFree(msg);
}