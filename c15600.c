void gf_node_free(GF_Node *node)
{
	if (!node) return;

	if (node->sgprivate->UserCallback) node->sgprivate->UserCallback(node, NULL, 1);

	if (node->sgprivate->scenegraph && node->sgprivate->scenegraph->NodeCallback)
		node->sgprivate->scenegraph->NodeCallback(node->sgprivate->scenegraph->userpriv, GF_SG_CALLBACK_NODE_DESTROY, node, NULL);

	if (node->sgprivate->interact) {
		if (node->sgprivate->interact->routes) {
			gf_list_del(node->sgprivate->interact->routes);
		}
#ifndef GPAC_DISABLE_SVG
		if (node->sgprivate->interact->dom_evt) {
			gf_dom_event_remove_all_listeners(node->sgprivate->interact->dom_evt);
			gf_dom_event_target_del(node->sgprivate->interact->dom_evt);
		}
		if (node->sgprivate->interact->animations) {
			gf_list_del(node->sgprivate->interact->animations);
		}
#endif
#ifdef GPAC_HAS_QJS
		if (node->sgprivate->interact->js_binding) {
			if (node->sgprivate->scenegraph && node->sgprivate->scenegraph->on_node_modified)
				node->sgprivate->scenegraph->on_node_modified(node->sgprivate->scenegraph, node, NULL, NULL);
			gf_list_del(node->sgprivate->interact->js_binding->fields);
			gf_free(node->sgprivate->interact->js_binding);
		}
#endif
		gf_free(node->sgprivate->interact);
	}
	assert(! node->sgprivate->parents);
	gf_free(node->sgprivate);
	gf_free(node);
}