xmlXPathNodeSetMerge(xmlNodeSetPtr val1, xmlNodeSetPtr val2) {
    int i, j, initNr, skip;
    xmlNodePtr n1, n2;

    if (val2 == NULL) return(val1);
    if (val1 == NULL) {
	val1 = xmlXPathNodeSetCreate(NULL);
    if (val1 == NULL)
        return (NULL);
#if 0
	/*
	* TODO: The optimization won't work in every case, since
	*  those nasty namespace nodes need to be added with
	*  xmlXPathNodeSetDupNs() to the set; thus a pure
	*  memcpy is not possible.
	*  If there was a flag on the nodesetval, indicating that
	*  some temporary nodes are in, that would be helpfull.
	*/
	/*
	* Optimization: Create an equally sized node-set
	* and memcpy the content.
	*/
	val1 = xmlXPathNodeSetCreateSize(val2->nodeNr);
	if (val1 == NULL)
	    return(NULL);
	if (val2->nodeNr != 0) {
	    if (val2->nodeNr == 1)
		*(val1->nodeTab) = *(val2->nodeTab);
	    else {
		memcpy(val1->nodeTab, val2->nodeTab,
		    val2->nodeNr * sizeof(xmlNodePtr));
	    }
	    val1->nodeNr = val2->nodeNr;
	}
	return(val1);
#endif
    }

    /* @@ with_ns to check whether namespace nodes should be looked at @@ */
    initNr = val1->nodeNr;

    for (i = 0;i < val2->nodeNr;i++) {
	n2 = val2->nodeTab[i];
	/*
	 * check against duplicates
	 */
	skip = 0;
	for (j = 0; j < initNr; j++) {
	    n1 = val1->nodeTab[j];
	    if (n1 == n2) {
		skip = 1;
		break;
	    } else if ((n1->type == XML_NAMESPACE_DECL) &&
		       (n2->type == XML_NAMESPACE_DECL)) {
		if ((((xmlNsPtr) n1)->next == ((xmlNsPtr) n2)->next) &&
		    (xmlStrEqual(((xmlNsPtr) n1)->prefix,
			((xmlNsPtr) n2)->prefix)))
		{
		    skip = 1;
		    break;
		}
	    }
	}
	if (skip)
	    continue;

	/*
	 * grow the nodeTab if needed
	 */
	if (val1->nodeMax == 0) {
	    val1->nodeTab = (xmlNodePtr *) xmlMalloc(XML_NODESET_DEFAULT *
						    sizeof(xmlNodePtr));
	    if (val1->nodeTab == NULL) {
	        xmlXPathErrMemory(NULL, "merging nodeset\n");
		return(NULL);
	    }
	    memset(val1->nodeTab, 0 ,
		   XML_NODESET_DEFAULT * (size_t) sizeof(xmlNodePtr));
	    val1->nodeMax = XML_NODESET_DEFAULT;
	} else if (val1->nodeNr == val1->nodeMax) {
	    xmlNodePtr *temp;

	    val1->nodeMax *= 2;
	    temp = (xmlNodePtr *) xmlRealloc(val1->nodeTab, val1->nodeMax *
					     sizeof(xmlNodePtr));
	    if (temp == NULL) {
	        xmlXPathErrMemory(NULL, "merging nodeset\n");
		return(NULL);
	    }
	    val1->nodeTab = temp;
	}
	if (n2->type == XML_NAMESPACE_DECL) {
	    xmlNsPtr ns = (xmlNsPtr) n2;

	    val1->nodeTab[val1->nodeNr++] =
		xmlXPathNodeSetDupNs((xmlNodePtr) ns->next, ns);
	} else
	    val1->nodeTab[val1->nodeNr++] = n2;
    }

    return(val1);
}