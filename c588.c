mono_gc_init (void)
{
	InitializeCriticalSection (&handle_section);
	InitializeCriticalSection (&allocator_section);

	InitializeCriticalSection (&finalizer_mutex);

	MONO_GC_REGISTER_ROOT_FIXED (gc_handles [HANDLE_NORMAL].entries);
	MONO_GC_REGISTER_ROOT_FIXED (gc_handles [HANDLE_PINNED].entries);

	mono_gc_base_init ();

	if (mono_gc_is_disabled ()) {
		gc_disabled = TRUE;
		return;
	}
	
	finalizer_event = CreateEvent (NULL, FALSE, FALSE, NULL);
	pending_done_event = CreateEvent (NULL, TRUE, FALSE, NULL);
	shutdown_event = CreateEvent (NULL, TRUE, FALSE, NULL);
	if (finalizer_event == NULL || pending_done_event == NULL || shutdown_event == NULL) {
		g_assert_not_reached ();
	}
#ifdef MONO_HAS_SEMAPHORES
	MONO_SEM_INIT (&finalizer_sem, 0);
#endif

	gc_thread = mono_thread_create_internal (mono_domain_get (), finalizer_thread, NULL, FALSE);
	ves_icall_System_Threading_Thread_SetName_internal (gc_thread, mono_string_new (mono_domain_get (), "Finalizer"));
}