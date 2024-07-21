xmlXPathNodeSetMergeAndClearNoDupls(xmlNodeSetPtr set1, xmlNodeSetPtr set2,
				    int hasNullEntries)
{
    if (set2 == NULL)
	return(set1);
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
	int i;
	xmlNodePtr n2;

	if (set1 == NULL)
	    set1 = xmlXPathNodeSetCreate(NULL);
        if (set1 == NULL)
            return (NULL);

	for (i = 0;i < set2->nodeNr;i++) {
	    n2 = set2->nodeTab[i];
	    /*
	    * Skip NULLed entries.
	    */
	    if (n2 == NULL)
		continue;
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
	    set1->nodeTab[set1->nodeNr++] = n2;
	}
    }
    set2->nodeNr = 0;
    return(set1);
}