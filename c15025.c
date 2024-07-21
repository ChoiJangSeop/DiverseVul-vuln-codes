sasl_session_t *find_session(const char *uid)
{
	sasl_session_t *p;
	mowgli_node_t *n;

	if (uid == NULL)
		return NULL;

	MOWGLI_ITER_FOREACH(n, sessions.head)
	{
		p = n->data;
		if(p->uid != NULL && !strcmp(p->uid, uid))
			return p;
	}

	return NULL;
}