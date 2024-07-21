void mono_gc_init (void)
{
	InitializeCriticalSection (&handle_section);
}