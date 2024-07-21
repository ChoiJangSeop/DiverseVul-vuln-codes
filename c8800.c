ses_handle(struct waited *wp, enum wait_event ev, vtim_real now)
{
	struct sess *sp;
	struct pool *pp;
	struct pool_task *tp;
	const struct transport *xp;

	CHECK_OBJ_NOTNULL(wp, WAITED_MAGIC);
	CAST_OBJ_NOTNULL(sp, wp->priv1, SESS_MAGIC);
	CAST_OBJ_NOTNULL(xp, (const void*)wp->priv2, TRANSPORT_MAGIC);
	AN(wp->priv2);
	assert((void *)sp->ws->f == wp);
	wp->magic = 0;
	wp = NULL;

	WS_Release(sp->ws, 0);

	switch (ev) {
	case WAITER_TIMEOUT:
		SES_Delete(sp, SC_RX_TIMEOUT, now);
		break;
	case WAITER_REMCLOSE:
		SES_Delete(sp, SC_REM_CLOSE, now);
		break;
	case WAITER_ACTION:
		pp = sp->pool;
		CHECK_OBJ_NOTNULL(pp, POOL_MAGIC);
		assert(sizeof *tp <= WS_ReserveSize(sp->ws, sizeof *tp));
		tp = (void*)sp->ws->f;
		tp->func = xp->unwait;
		tp->priv = sp;
		if (Pool_Task(pp, tp, TASK_QUEUE_REQ))
			SES_Delete(sp, SC_OVERLOAD, now);
		break;
	case WAITER_CLOSE:
		WRONG("Should not see WAITER_CLOSE on client side");
		break;
	default:
		WRONG("Wrong event in ses_handle");
	}
}