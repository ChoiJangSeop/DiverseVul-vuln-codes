SES_Wait(struct sess *sp, const struct transport *xp)
{
	struct pool *pp;
	struct waited *wp;

	CHECK_OBJ_NOTNULL(sp, SESS_MAGIC);
	CHECK_OBJ_NOTNULL(xp, TRANSPORT_MAGIC);
	pp = sp->pool;
	CHECK_OBJ_NOTNULL(pp, POOL_MAGIC);
	assert(sp->fd > 0);
	/*
	 * XXX: waiter_epoll prevents us from zeroing the struct because
	 * XXX: it keeps state across calls.
	 */
	VTCP_nonblocking(sp->fd);

	/*
	 * put struct waited on the workspace
	 */
	if (WS_ReserveSize(sp->ws, sizeof(struct waited))
	    < sizeof(struct waited)) {
		SES_Delete(sp, SC_OVERLOAD, NAN);
		return;
	}
	wp = (void*)sp->ws->f;
	INIT_OBJ(wp, WAITED_MAGIC);
	wp->fd = sp->fd;
	wp->priv1 = sp;
	wp->priv2 = (uintptr_t)xp;
	wp->idle = sp->t_idle;
	wp->func = ses_handle;
	wp->tmo = SESS_TMO(sp, timeout_idle);
	if (Wait_Enter(pp->waiter, wp))
		SES_Delete(sp, SC_PIPE_OVERFLOW, NAN);
}