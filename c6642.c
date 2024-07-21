static int pppol2tp_session_create(struct net *net, u32 tunnel_id, u32 session_id, u32 peer_session_id, struct l2tp_session_cfg *cfg)
{
	int error;
	struct l2tp_tunnel *tunnel;
	struct l2tp_session *session;
	struct pppol2tp_session *ps;

	tunnel = l2tp_tunnel_find(net, tunnel_id);

	/* Error if we can't find the tunnel */
	error = -ENOENT;
	if (tunnel == NULL)
		goto out;

	/* Error if tunnel socket is not prepped */
	if (tunnel->sock == NULL)
		goto out;

	/* Default MTU values. */
	if (cfg->mtu == 0)
		cfg->mtu = 1500 - PPPOL2TP_HEADER_OVERHEAD;
	if (cfg->mru == 0)
		cfg->mru = cfg->mtu;

	/* Allocate and initialize a new session context. */
	session = l2tp_session_create(sizeof(struct pppol2tp_session),
				      tunnel, session_id,
				      peer_session_id, cfg);
	if (IS_ERR(session)) {
		error = PTR_ERR(session);
		goto out;
	}

	ps = l2tp_session_priv(session);
	ps->tunnel_sock = tunnel->sock;

	l2tp_info(session, L2TP_MSG_CONTROL, "%s: created\n",
		  session->name);

	error = 0;

out:
	return error;
}