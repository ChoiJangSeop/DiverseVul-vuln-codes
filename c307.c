static void async_main_request_sent(void *private_data, BOOL success)
{
	struct winbindd_async_request *state =
		talloc_get_type_abort(private_data, struct winbindd_async_request);

	if (!success) {
		DEBUG(5, ("Could not send async request\n"));

		state->response->length = sizeof(struct winbindd_response);
		state->response->result = WINBINDD_ERROR;
		state->continuation(state->private_data, False);
		return;
	}

	if (state->request->extra_len == 0) {
		async_request_sent(private_data, True);
		return;
	}

	setup_async_write(&state->child->event, state->request->extra_data.data,
			  state->request->extra_len,
			  async_request_sent, state);
}