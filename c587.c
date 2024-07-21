mono_runtime_shutdown (void)
{
	mono_domain_foreach (fire_process_exit_event, NULL);
}