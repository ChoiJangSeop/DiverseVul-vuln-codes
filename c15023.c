sasl_session_t *make_session(const char *uid, server_t *server)
{
	sasl_session_t *p = find_session(uid);
	mowgli_node_t *n;

	if(p)
		return p;

	p = malloc(sizeof(sasl_session_t));
	memset(p, 0, sizeof(sasl_session_t));
	p->uid = strdup(uid);
	p->server = server;
	n = mowgli_node_create();
	mowgli_node_add(p, n, &sessions);

	return p;
}