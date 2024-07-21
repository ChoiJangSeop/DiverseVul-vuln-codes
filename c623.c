xmlXPathNodeSetMergeAndClear(xmlNodeSetPtr set1, xmlNodeSetPtr set2,
			     int hasNullEntries)
{
    if ((set1 == NULL) && (hasNullEntries == 0)) {
	/*
	* Note that doing a memcpy of the list, namespace nodes are
	* just assigned to set1, since set2 is cleared anyway.
	*/
	set1 = xmlXPathNodeSetCreateSize(set2->nodeNr);
	if (set1 == NULL)
	    return(NULL);
	if (set2->nodeNr != 0) {
	    memcpy(set1->nodeTab, set2->nodeTab,
		set2->nodeNr * sizeof(xmlNodePtr));
	    set1->nodeNr = set2->nodeNr;
	}
    } else {
	int i, j, initNbSet1;
	xmlNodePtr n1, n2;

	if (set1 == NULL)
            set1 = xmlXPathNodeSetCreate(NULL);
        if (set1 == NULL)
            return (NULL);

	initNbSet1 = set1->nodeNr;
	for (i = 0;i < set2->nodeNr;i++) {
	    n2 = set2->nodeTab[i];
	    /*
	    * Skip NULLed entries.
	    */
	    if (n2 == NULL)
		continue;
	    /*
	    * Skip duplicates.
	    */
	    for (j = 0; j < initNbSet1; j++) {
		n1 = set1->nodeTab[j];
		if (n1 == n2) {
		    goto skip_node;
		} else if ((n1->type == XML_NAMESPACE_DECL) &&
		    (n2->type == XML_NAMESPACE_DECL))
		{
		    if ((((xmlNsPtr) n1)->next == ((xmlNsPtr) n2)->next) &&
			(xmlStrEqual(((xmlNsPtr) n1)->prefix,
			((xmlNsPtr) n2)->prefix)))
		    {
			/*
			* Free the namespace node.
			*/
			set2->nodeTab[i] = NULL;
			xmlXPathNodeSetFreeNs((xmlNsPtr) n2);
			goto skip_node;
		    }
		}
	    }
	    /*
	    * grow the nodeTab if needed
	    */
	    if (set1->nodeMax == 0) {
		set1->nodeTab = (xmlNodePtr *) xmlMalloc(
		    XML_NODESET_DEFAULT * sizeof(xmlNodePtr));
		if (set1->nodeTab == NULL) {
		    xmlXPathErrMemory(NULL, "merging nodeset\n");
		    return(NULL);
		}
		memset(set1->nodeTab, 0,
		    XML_NODESET_DEFAULT * (size_t) sizeof(xmlNodePtr));
		set1->nodeMax = XML_NODESET_DEFAULT;
	    } else if (set1->nodeNr >= set1->nodeMax) {
		xmlNodePtr *temp;

		set1->nodeMax *= 2;
		temp = (xmlNodePtr *) xmlRealloc(
		    set1->nodeTab, set1->nodeMax * sizeof(xmlNodePtr));
		if (temp == NULL) {
		    xmlXPathErrMemory(NULL, "merging nodeset\n");
		    return(NULL);
		}
		set1->nodeTab = temp;
	    }
	    if (n2->type == XML_NAMESPACE_DECL) {
		xmlNsPtr ns = (xmlNsPtr) n2;

		set1->nodeTab[set1->nodeNr++] =
		    xmlXPathNodeSetDupNs((xmlNodePtr) ns->next, ns);
	    } else
		set1->nodeTab[set1->nodeNr++] = n2;
skip_node:
	    {}
	}
    }
    set2->nodeNr = 0;
    return(set1);
}