mono_gc_cleanup (void)
{
#ifdef DEBUG
	g_message ("%s: cleaning up finalizer", __func__);
#endif

	if (!gc_disabled) {
		ResetEvent (shutdown_event);
		finished = TRUE;
		if (mono_thread_internal_current () != gc_thread) {
			mono_gc_finalize_notify ();
			/* Finishing the finalizer thread, so wait a little bit... */
			/* MS seems to wait for about 2 seconds */
			if (WaitForSingleObjectEx (shutdown_event, 2000, FALSE) == WAIT_TIMEOUT) {
				int ret;

				/* Set a flag which the finalizer thread can check */
				suspend_finalizers = TRUE;

				/* Try to abort the thread, in the hope that it is running managed code */
				mono_thread_internal_stop (gc_thread);

				/* Wait for it to stop */
				ret = WaitForSingleObjectEx (gc_thread->handle, 100, TRUE);

				if (ret == WAIT_TIMEOUT) {
					/* 
					 * The finalizer thread refused to die. There is not much we 
					 * can do here, since the runtime is shutting down so the 
					 * state the finalizer thread depends on will vanish.
					 */
					g_warning ("Shutting down finalizer thread timed out.");
				} else {
					/*
					 * FIXME: On unix, when the above wait returns, the thread 
					 * might still be running io-layer code, or pthreads code.
					 */
					Sleep (100);
				}

			}
		}
		gc_thread = NULL;
#ifdef HAVE_BOEHM_GC
		GC_finalizer_notifier = NULL;
#endif
	}

	DeleteCriticalSection (&handle_section);
	DeleteCriticalSection (&allocator_section);
	DeleteCriticalSection (&finalizer_mutex);
}