void audit_put_chunk(struct audit_chunk *chunk)
{
	put_inotify_watch(&chunk->watch);
}