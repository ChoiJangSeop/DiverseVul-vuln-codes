static void destroy_watch(struct inotify_watch *watch)
{
	struct audit_chunk *chunk = container_of(watch, struct audit_chunk, watch);
	free_chunk(chunk);
}