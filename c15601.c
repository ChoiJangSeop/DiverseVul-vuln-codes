void gf_sg_reset(GF_SceneGraph *sg)
{
	GF_SceneGraph *par;
	GF_List *gc;
#ifndef GPAC_DISABLE_SVG
	u32 type;
#endif
	u32 count;
	NodeIDedItem *reg_node;
	if (!sg) return;

	GF_LOG(GF_LOG_DEBUG, GF_LOG_SCENE, ("[SceneGraph] resetting scene graph\n"));
#if 0
	/*inline graph, remove any of this graph nodes from the parent graph*/
	if (!sg->pOwningProto && sg->parent_scene) {
		GF_SceneGraph *par = sg->parent_scene;
		while (par->parent_scene) par = par->parent_scene;
		if (par->RootNode) SG_GraphRemoved(par->RootNode, sg);
	}
#endif

	gc = gf_list_new();
#ifdef GPAC_HAS_QJS
	/*scripts are the first source of cylic references in the graph. In order to clean properly
	force a remove of all script nodes, this will release all references to nodes in JS*/
	while (gf_list_count(sg->scripts)) {
		GF_Node *n = gf_list_get(sg->scripts, 0);
		gf_list_rem(sg->scripts, 0);
		/*prevent destroy*/
		gf_node_register(n, NULL);
		/*remove from all parents*/
		gf_node_replace(n, NULL, 0);
		/*FORCE destroy in case the script refers to itself*/
		n->sgprivate->num_instances=1;
		gf_node_unregister(n, NULL);
		/*remember this script was forced to be destroyed*/
		gf_list_add(gc, n);
	}
#endif

#ifndef GPAC_DISABLE_SVG

	gf_mx_p(sg->dom_evt_mx);
	/*remove listeners attached to the doc*/
	gf_dom_event_remove_all_listeners(sg->dom_evt);
	/*flush any pending add_listener*/
	gf_dom_listener_reset_deferred(sg);
	gf_mx_v(sg->dom_evt_mx);
#endif

#ifndef GPAC_DISABLE_VRML
	while (gf_list_count(sg->routes_to_activate)) {
		gf_list_rem(sg->routes_to_activate, 0);
	}

	/*destroy all routes*/
	while (gf_list_count(sg->Routes)) {
		GF_Route *r = (GF_Route*)gf_list_get(sg->Routes, 0);
		/*this will unregister the route from the graph, so don't delete the chain entry*/
		gf_sg_route_del(r);

	}
#endif


	/*reset all exported symbols */
	while (gf_list_count(sg->exported_nodes)) {
		GF_Node *n = gf_list_get(sg->exported_nodes, 0);
		gf_list_rem(sg->exported_nodes, 0);
		gf_node_replace(n, NULL, 0);
	}
	/*reassign the list of exported nodes to our garbage collected nodes*/
	gf_list_del(sg->exported_nodes);
	sg->exported_nodes = gc;

	/*reset the main tree*/
	if (sg->RootNode) gf_node_unregister(sg->RootNode, NULL);
	sg->RootNode = NULL;

#ifndef GPAC_DISABLE_VRML
	if (!sg->pOwningProto && gf_list_count(sg->protos) && sg->GetExternProtoLib)
		sg->GetExternProtoLib(sg->userpriv, NULL);
#endif

	/*WATCHOUT: we may have cyclic dependencies due to
	1- a node referencing itself (forbidden in VRML)
	2- nodes referred to in commands of conditionals children of this node (MPEG-4 is mute about that)
	we recursively preocess from last declared DEF node to first one
	*/
restart:
	reg_node = sg->id_node;
	while (reg_node) {
#if 0
		Bool ignore = 0;
#endif
		GF_ParentList *nlist;

		GF_Node *node = reg_node->node;
		if (!node
#ifndef GPAC_DISABLE_VRML
		        || (node==sg->global_qp)
#endif
		   ) {
			reg_node = reg_node->next;
			continue;
		}

		/*first replace all instances in parents by NULL WITHOUT UNREGISTERING (to avoid destroying the node).
		This will take care of nodes referencing themselves*/
		nlist = node->sgprivate->parents;
#ifndef GPAC_DISABLE_SVG
		type = (node->sgprivate->tag>GF_NODE_RANGE_LAST_VRML) ? 1 : 0;
#endif
		while (nlist) {
			GF_ParentList *next = nlist->next;
#if 0
			/*parent is a DEF'ed node, try to clean-up properly?*/
			if ((nlist->node!=node) && SG_SearchForNode(sg, nlist->node) != NULL) {
				ignore = 1;
				break;
			}
#endif

#ifndef GPAC_DISABLE_SVG
			if (type) {
				ReplaceIRINode(nlist->node, node, NULL);
			} else
#endif
				ReplaceDEFNode(nlist->node, reg_node->node, NULL, 0);

			/*direct cyclic reference to ourselves, make sure we update the parentList to the next entry before freeing it
			since the next parent node could be reg_node again (reg_node->reg_node)*/
			if (nlist->node==node) {
				node->sgprivate->parents = next;
			}

			gf_free(nlist);
			nlist = next;
#if 0
			if (ignore) {
				node->sgprivate->parents = nlist;
				continue;
			}
#endif
		}

		node->sgprivate->parents = NULL;

		//sg->node_registry[i-1] = NULL;
		count = get_num_id_nodes(sg);
		node->sgprivate->num_instances = 1;
		/*remember this node was forced to be destroyed*/
		gf_list_add(sg->exported_nodes, node);
		gf_node_unregister(node, NULL);
		if (count != get_num_id_nodes(sg)) goto restart;
		reg_node = reg_node->next;
	}

	/*reset the forced destroy ndoes*/
	gf_list_reset(sg->exported_nodes);

#ifndef GPAC_DISABLE_VRML
	/*destroy all proto*/
	while (gf_list_count(sg->protos)) {
		GF_Proto *p = (GF_Proto *)gf_list_get(sg->protos, 0);
		/*this will unregister the proto from the graph, so don't delete the chain entry*/
		gf_sg_proto_del(p);
	}
	/*destroy all unregistered proto*/
	while (gf_list_count(sg->unregistered_protos)) {
		GF_Proto *p = (GF_Proto *)gf_list_get(sg->unregistered_protos, 0);
		/*this will unregister the proto from the graph, so don't delete the chain entry*/
		gf_sg_proto_del(p);
	}

	/*last destroy all routes*/
	gf_sg_destroy_routes(sg);
	sg->simulation_tick = 0;

#endif /*GPAC_DISABLE_VRML*/

#ifndef GPAC_DISABLE_SVG
//	assert(gf_list_count(sg->xlink_hrefs) == 0);
#endif

	while (gf_list_count(sg->ns)) {
		GF_XMLNS *ns = gf_list_get(sg->ns, 0);
		gf_list_rem(sg->ns, 0);
		if (ns->name) gf_free(ns->name);
		if (ns->qname) gf_free(ns->qname);
		gf_free(ns);
	}
	gf_list_del(sg->ns);
	sg->ns = 0;

	par = sg;
	while (par->parent_scene) par = par->parent_scene;

#ifndef GPAC_DISABLE_SVG
	if (par != sg) {
		u32 i;
		count = gf_list_count(par->smil_timed_elements);
		for (i=0; i<count; i++) {
			SMIL_Timing_RTI *rti = gf_list_get(par->smil_timed_elements, i);
			if (rti->timed_elt->sgprivate->scenegraph == sg) {
				gf_list_rem(par->smil_timed_elements, i);
				i--;
				count--;
			}
		}
	}
#endif

#ifdef GF_SELF_REPLACE_ENABLE
	sg->graph_has_been_reset = 1;
#endif
	GF_LOG(GF_LOG_DEBUG, GF_LOG_SCENE, ("[SceneGraph] Scene graph has been reset\n"));
}