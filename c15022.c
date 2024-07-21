static void sasl_newuser(hook_user_nick_t *data)
{
	user_t *u = data->u;
	sasl_mechanism_t *mptr;
	sasl_session_t *p;
	myuser_t *mu;

	/* If the user has been killed, don't do anything. */
	if (!u)
		return;

	p = find_session(u->uid);

	/* Not concerned unless it's a SASL login. */
	if(p == NULL)
		return;

	/* We will log it ourselves, if needed */
	p->flags &= ~ASASL_NEED_LOG;

	/* Find the account */
	mu = p->authzid ? myuser_find_by_nick(p->authzid) : NULL;
	if (mu == NULL)
	{
		notice(saslsvs->nick, u->nick, "Account %s dropped, login cancelled",
		       p->authzid ? p->authzid : "??");
		destroy_session(p);
		/* We'll remove their ircd login in handle_burstlogin() */
		return;
	}

	mptr = p->mechptr;

	destroy_session(p);

	myuser_login(saslsvs, u, mu, false);

	logcommand_user(saslsvs, u, CMDLOG_LOGIN, "LOGIN (%s)", mptr->name);
}