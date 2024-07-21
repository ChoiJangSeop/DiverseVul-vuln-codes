void gf_dom_event_remove_all_listeners(GF_DOMEventTarget *event_target)
{
	while (gf_list_count(event_target->listeners)) {
		GF_Node *n = (GF_Node *)gf_list_get(event_target->listeners, 0);
		gf_dom_listener_del(n, event_target);
	}
}