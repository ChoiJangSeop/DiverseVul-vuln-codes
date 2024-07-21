xmlRemoveID(xmlDocPtr doc, xmlAttrPtr attr) {
    xmlIDTablePtr table;
    xmlIDPtr id;
    xmlChar *ID;

    if (doc == NULL) return(-1);
    if (attr == NULL) return(-1);

    table = (xmlIDTablePtr) doc->ids;
    if (table == NULL)
        return(-1);

    ID = xmlNodeListGetString(doc, attr->children, 1);
    if (ID == NULL)
        return(-1);

    id = xmlHashLookup(table, ID);
    if (id == NULL || id->attr != attr) {
        xmlFree(ID);
        return(-1);
    }

    xmlHashRemoveEntry(table, ID, xmlFreeIDTableEntry);
    xmlFree(ID);
    attr->atype = 0;
    return(0);
}