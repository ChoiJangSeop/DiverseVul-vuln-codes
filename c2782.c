static void pcntl_signal_handler(int signo)
{
	struct php_pcntl_pending_signal *psig;
	TSRMLS_FETCH();
	
	psig = PCNTL_G(spares);
	if (!psig) {
		/* oops, too many signals for us to track, so we'll forget about this one */
		return;
	}
	PCNTL_G(spares) = psig->next;

	psig->signo = signo;
	psig->next = NULL;

	/* the head check is important, as the tick handler cannot atomically clear both
	 * the head and tail */
	if (PCNTL_G(head) && PCNTL_G(tail)) {
		PCNTL_G(tail)->next = psig;
	} else {
		PCNTL_G(head) = psig;
	}
	PCNTL_G(tail) = psig;
}