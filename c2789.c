void pcntl_signal_dispatch()
{
	zval *param, **handle, *retval;
	struct php_pcntl_pending_signal *queue, *next;
	sigset_t mask;
	sigset_t old_mask;
	TSRMLS_FETCH();
		
	/* Mask all signals */
	sigfillset(&mask);
	sigprocmask(SIG_BLOCK, &mask, &old_mask);

	/* Bail if the queue is empty or if we are already playing the queue*/
	if (! PCNTL_G(head) || PCNTL_G(processing_signal_queue)) {
		sigprocmask(SIG_SETMASK, &old_mask, NULL);
		return;
	}

	/* Prevent reentrant handler calls */
	PCNTL_G(processing_signal_queue) = 1;

	queue = PCNTL_G(head);
	PCNTL_G(head) = NULL; /* simple stores are atomic */
	
	/* Allocate */

	while (queue) {
		if (zend_hash_index_find(&PCNTL_G(php_signal_table), queue->signo, (void **) &handle)==SUCCESS) {
			MAKE_STD_ZVAL(retval);
			MAKE_STD_ZVAL(param);
			ZVAL_NULL(retval);
			ZVAL_LONG(param, queue->signo);

			/* Call php signal handler - Note that we do not report errors, and we ignore the return value */
			/* FIXME: this is probably broken when multiple signals are handled in this while loop (retval) */
			call_user_function(EG(function_table), NULL, *handle, retval, 1, &param TSRMLS_CC);
			zval_ptr_dtor(&param);
			zval_ptr_dtor(&retval);
		}

		next = queue->next;
		queue->next = PCNTL_G(spares);
		PCNTL_G(spares) = queue;
		queue = next;
	}

	/* Re-enable queue */
	PCNTL_G(processing_signal_queue) = 0;
	
	/* return signal mask to previous state */
	sigprocmask(SIG_SETMASK, &old_mask, NULL);
}