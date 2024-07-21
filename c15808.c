static int set_next_request(void)
{
	current_req = list_first_entry_or_null(&floppy_reqs, struct request,
					       queuelist);
	if (current_req) {
		current_req->error_count = 0;
		list_del_init(&current_req->queuelist);
	}
	return current_req != NULL;
}