static void ucma_move_events(struct ucma_context *ctx, struct ucma_file *file)
{
	struct ucma_event *uevent, *tmp;

	list_for_each_entry_safe(uevent, tmp, &ctx->file->event_list, list)
		if (uevent->ctx == ctx)
			list_move_tail(&uevent->list, &file->event_list);
}