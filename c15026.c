void destroy_session(sasl_session_t *p)
{
	mowgli_node_t *n, *tn;
	myuser_t *mu;

	if (p->flags & ASASL_NEED_LOG && p->username != NULL)
	{
		mu = myuser_find_by_nick(p->username);
		if (mu != NULL && !(ircd->flags & IRCD_SASL_USE_PUID))
			sasl_logcommand(p, mu, CMDLOG_LOGIN, "LOGIN (session timed out)");
	}

	MOWGLI_ITER_FOREACH_SAFE(n, tn, sessions.head)
	{
		if(n->data == p)
		{
			mowgli_node_delete(n, &sessions);
			mowgli_node_free(n);
		}
	}

	free(p->uid);
	free(p->buf);
	p->buf = p->p = NULL;
	if(p->mechptr)
		p->mechptr->mech_finish(p); /* Free up any mechanism data */
	p->mechptr = NULL; /* We're not freeing the mechanism, just "dereferencing" it */
	free(p->username);
	free(p->certfp);
	free(p->authzid);
	free(p->host);
	free(p->ip);

	free(p);
}