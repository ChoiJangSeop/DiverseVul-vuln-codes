static void endElementHandler(void *userData, const char *name)
{
    userdata_t *ud = (userdata_t *) userData;

    if (strcmp(name, "graph") == 0) {
	pop_subg();
	popString(&ud->elements);
	ud->closedElementType = TAG_GRAPH;
    } else if (strcmp(name, "node") == 0) {
	char *ele_name = topString(ud->elements);
	if (ud->closedElementType == TAG_GRAPH) {
	    Agnode_t *node = agnode(root, ele_name, 0);
	    agdelete(root, node);
	}
	popString(&ud->elements);
	Current_class = TAG_GRAPH;
	N = 0;
	ud->closedElementType = TAG_NODE;
    } else if (strcmp(name, "edge") == 0) {
	Current_class = TAG_GRAPH;
	E = 0;
	ud->closedElementType = TAG_EDGE;
	ud->edgeinverted = FALSE;
    } else if (strcmp(name, "attr") == 0) {
	char *name;
	char *value;
	char buf[SMALLBUF] = GRAPHML_COMP;
	char *dynbuf = 0;

	ud->closedElementType = TAG_NONE;
	if (ud->compositeReadState) {
	    int len = sizeof(GRAPHML_COMP) + agxblen(&ud->xml_attr_name);
	    if (len <= SMALLBUF) {
		name = buf;
	    } else {
		name = dynbuf = N_NEW(len, char);
		strcpy(name, GRAPHML_COMP);
	    }
	    strcpy(name + sizeof(GRAPHML_COMP) - 1,
		   agxbuse(&ud->xml_attr_name));
	    value = agxbuse(&ud->composite_buffer);
	    agxbclear(&ud->xml_attr_value);
	    ud->compositeReadState = FALSE;
	} else {
	    name = agxbuse(&ud->xml_attr_name);
	    value = agxbuse(&ud->xml_attr_value);
	}

	switch (ud->globalAttrType) {
	case TAG_NONE:
	    setAttr(name, value, ud);
	    break;
	case TAG_NODE:
	    setGlobalNodeAttr(G, name, value, ud);
	    break;
	case TAG_EDGE:
	    setGlobalEdgeAttr(G, name, value, ud);
	    break;
	case TAG_GRAPH:
	    setGraphAttr(G, name, value, ud);
	    break;
	}
	if (dynbuf)
	    free(dynbuf);
	ud->globalAttrType = TAG_NONE;
    } 
}