static bool rb_per_cpu_empty(struct ring_buffer_per_cpu *cpu_buffer)
{
	struct buffer_page *reader = cpu_buffer->reader_page;
	struct buffer_page *head = rb_set_head_page(cpu_buffer);
	struct buffer_page *commit = cpu_buffer->commit_page;

	/* In case of error, head will be NULL */
	if (unlikely(!head))
		return true;

	return reader->read == rb_page_commit(reader) &&
		(commit == reader ||
		 (commit == head &&
		  head->read == rb_page_commit(commit)));
}