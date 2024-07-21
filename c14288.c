static void wait_a_bit(void *ctx)
{
	struct timeval when;
	REQUEST *request = ctx;
	fr_event_callback_t callback = NULL;

	rad_assert(request->magic == REQUEST_MAGIC);

#ifdef WITH_COA
	/*
	 *	The CoA request is a new (internally generated)
	 *	request, created in a child thread.  We therefore need
	 *	some way to tie its events back into the main event
	 *	handler.
	 */
	if (request->coa && !request->coa->proxy_reply &&
	    request->coa->next_callback) {
		request->coa->when = request->coa->next_when;
		INSERT_EVENT(request->coa->next_callback, request->coa);
		request->coa->next_callback = NULL;
		request->coa->parent = NULL;
		request->coa = NULL;
	}
#endif

	switch (request->child_state) {
	case REQUEST_QUEUED:
	case REQUEST_RUNNING:
		when = request->received;
		when.tv_sec += request->root->max_request_time;

		/*
		 *	Normally called from the event loop with the
		 *	proper event loop time.  Otherwise, called from
		 *	post proxy fail handler, which sets "now", and
		 *	this call won't re-set it, because we're not
		 *	in the event loop.
		 */
		fr_event_now(el, &now);

		/*
		 *	Request still has more time.  Continue
		 *	waiting.
		 */
		if (timercmp(&now, &when, <) ||
		    ((request->listener->type == RAD_LISTEN_DETAIL) &&
		     (request->child_state == REQUEST_QUEUED))) {
			if (request->delay < (USEC / 10)) {
				request->delay = USEC / 10;
			}
			request->delay += request->delay >> 1;

#ifdef WITH_DETAIL
			/*
			 *	Cap wait at some sane value for detail
			 *	files.
			 */
			if ((request->listener->type == RAD_LISTEN_DETAIL) &&
			    (request->delay > (request->root->max_request_time * USEC))) {
				request->delay = request->root->max_request_time * USEC;
			}
#endif

			request->when = now;
			tv_add(&request->when, request->delay);
			callback = wait_a_bit;
			break;
		}

#if defined(HAVE_PTHREAD_H)
		/*
		 *	A child thread MAY still be running on the
		 *	request.  Ask the thread to stop working on
		 *	the request.
		 */
		if (have_children &&
		    (pthread_equal(request->child_pid, NO_SUCH_CHILD_PID) == 0)) {
			request->master_state = REQUEST_STOP_PROCESSING;

			radlog(L_ERR, "WARNING: Unresponsive child for request %u, in module %s component %s",
			       request->number,
			       request->module ? request->module : "<server core>",
			       request->component ? request->component : "<server core>");
			
			request->delay = USEC / 4;
			tv_add(&request->when, request->delay);
			callback = wait_for_child_to_die;
			break;
		}
#endif

		/*
		 *	Else no child thread is processing the
		 *	request.  We probably should have just marked
		 *	the request as 'done' elsewhere, like in the
		 *	post-proxy-fail handler.  But doing that would
		 *	involve checking for max_request_time in
		 *	multiple places, so this may be simplest.
		 */
		request->child_state = REQUEST_DONE;
		/* FALL-THROUGH */

		/*
		 *	Mark the request as no longer running,
		 *	and clean it up.
		 */
	case REQUEST_DONE:
#ifdef HAVE_PTHREAD_H
		request->child_pid = NO_SUCH_CHILD_PID;
#endif

#ifdef WITH_COA
		/*
		 *	This is a CoA request.  It's been divorced
		 *	from everything else, so we clean it up now.
		 */
		if (!request->in_request_hash &&
		    request->proxy &&
		    (request->packet->code != request->proxy->code) &&
		    ((request->proxy->code == PW_COA_REQUEST) ||
		     (request->proxy->code == PW_DISCONNECT_REQUEST))) {
			/*
			 *	FIXME: Do CoA MIBs
			 */
			ev_request_free(&request);
			return;
		}
#endif
		request_stats_final(request);
		cleanup_delay(request);
		return;

	case REQUEST_REJECT_DELAY:
	case REQUEST_CLEANUP_DELAY:
#ifdef HAVE_PTHREAD_H
		request->child_pid = NO_SUCH_CHILD_PID;
#endif
		request_stats_final(request);

	case REQUEST_PROXIED:
		rad_assert(request->next_callback != NULL);
		rad_assert(request->next_callback != wait_a_bit);

		request->when = request->next_when;
		callback = request->next_callback;
		request->next_callback = NULL;
		break;

	default:
		rad_panic("Internal sanity check failure");
		return;
	}

	/*
	 *	Something major went wrong.  Discard the request, and
	 *	keep running.
	 *
	 *	FIXME: No idea why this happens or how to fix it...
	 *	It seems to happen *only* when requests are proxied,
	 *	and where the home server doesn't respond.  So it looks
	 *	like a race condition above, but it happens in debug
	 *	mode, with no threads...
	 */
	if (!callback) {
		RDEBUG("WARNING: Internal sanity check failed in event handler for request %u: Discarding the request!", request->number);
		ev_request_free(&request);
		return;
	}

	INSERT_EVENT(callback, request);
}