finalizer_thread (gpointer unused)
{
	while (!finished) {
		/* Wait to be notified that there's at least one
		 * finaliser to run
		 */

		g_assert (mono_domain_get () == mono_get_root_domain ());

#ifdef MONO_HAS_SEMAPHORES
		MONO_SEM_WAIT (&finalizer_sem);
#else
		/* Use alertable=FALSE since we will be asked to exit using the event too */
		WaitForSingleObjectEx (finalizer_event, INFINITE, FALSE);
#endif

		mono_console_handle_async_ops ();

#ifndef DISABLE_ATTACH
		mono_attach_maybe_start ();
#endif

		if (domains_to_finalize) {
			mono_finalizer_lock ();
			if (domains_to_finalize) {
				DomainFinalizationReq *req = domains_to_finalize->data;
				domains_to_finalize = g_slist_remove (domains_to_finalize, req);
				mono_finalizer_unlock ();

				finalize_domain_objects (req);
			} else {
				mono_finalizer_unlock ();
			}
		}				

		/* If finished == TRUE, mono_gc_cleanup has been called (from mono_runtime_cleanup),
		 * before the domain is unloaded.
		 */
		mono_gc_invoke_finalizers ();

		SetEvent (pending_done_event);
	}

	SetEvent (shutdown_event);
	return 0;
}