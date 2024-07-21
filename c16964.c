startElementHandler(void *userData, const char *name, const char **atts)
{
    int pos;
    userdata_t *ud = (userdata_t *) userData;
    Agraph_t *g = NULL;

    if (strcmp(name, "graphml") == 0) {
	/* do nothing */
    } else if (strcmp(name, "graph") == 0) {
	const char *edgeMode = "";
	const char *id;
	Agdesc_t dir;
	char buf[NAMEBUF];	/* holds % + number */

	Current_class = TAG_GRAPH;
	if (ud->closedElementType == TAG_GRAPH) {
	    fprintf(stderr,
		    "Warning: Node contains more than one graph.\n");
	}
	pos = get_xml_attr("id", atts);
	if (pos > 0) {
	    id = atts[pos];
	}
	else
	    id = ud->gname;
	pos = get_xml_attr("edgedefault", atts);
	if (pos > 0) {
	    edgeMode = atts[pos];
	}

	if (GSP == 0) {
	    if (strcmp(edgeMode, "directed") == 0) {
		dir = Agdirected;
	    } else if (strcmp(edgeMode, "undirected") == 0) {
		dir = Agundirected;
	    } else {
		if (Verbose) {
		    fprintf(stderr,
			"Warning: graph has no edgedefault attribute - assume directed\n");
		}
		dir = Agdirected;
	    }
	    g = agopen((char *) id, dir, &AgDefaultDisc);
	    push_subg(g);
	} else {
	    Agraph_t *subg;
	    if (isAnonGraph((char *) id)) {
		static int anon_id = 1;
		sprintf(buf, "%%%d", anon_id++);
		id = buf;
	    }
	    subg = agsubg(G, (char *) id, 1);
	    push_subg(subg);
	}

	pushString(&ud->elements, id);
    } else if (strcmp(name, "node") == 0) {
	Current_class = TAG_NODE;
	pos = get_xml_attr("id", atts);
	if (pos > 0) {
	    const char *attrname;
	    attrname = atts[pos];

	    bind_node(attrname);

	    pushString(&ud->elements, attrname);
	}

    } else if (strcmp(name, "edge") == 0) {
	const char *head = "", *tail = "";
	char *tname;
	Agnode_t *t;

	Current_class = TAG_EDGE;
	pos = get_xml_attr("source", atts);
	if (pos > 0)
	    tail = atts[pos];
	pos = get_xml_attr("target", atts);
	if (pos > 0)
	    head = atts[pos];

	tname = mapLookup(ud->nameMap, (char *) tail);
	if (tname)
	    tail = tname;

	tname = mapLookup(ud->nameMap, (char *) head);
	if (tname)
	    head = tname;

	bind_edge(tail, head);

	t = AGTAIL(E);
	tname = agnameof(t);

	if (strcmp(tname, tail) == 0) {
	    ud->edgeinverted = FALSE;
	} else if (strcmp(tname, head) == 0) {
	    ud->edgeinverted = TRUE;
	}

	pos = get_xml_attr("id", atts);
	if (pos > 0) {
	    setEdgeAttr(E, GRAPHML_ID, (char *) atts[pos], ud);
	}
    } else {
	/* must be some extension */
	fprintf(stderr,
		"Unknown node %s - ignoring.\n",
		name);
    }
}