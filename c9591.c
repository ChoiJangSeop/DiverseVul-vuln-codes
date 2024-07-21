struct tevent_req *winbindd_lookupsids_send(TALLOC_CTX *mem_ctx,
					    struct tevent_context *ev,
					   struct winbindd_cli_state *cli,
					   struct winbindd_request *request)
{
	struct tevent_req *req, *subreq;
	struct winbindd_lookupsids_state *state;

	req = tevent_req_create(mem_ctx, &state,
				struct winbindd_lookupsids_state);
	if (req == NULL) {
		return NULL;
	}

	DEBUG(3, ("lookupsids\n"));

	if (request->extra_len == 0) {
		tevent_req_done(req);
		return tevent_req_post(req, ev);
	}
	if (request->extra_data.data[request->extra_len-1] != '\0') {
		DEBUG(10, ("Got invalid sids list\n"));
		tevent_req_nterror(req, NT_STATUS_INVALID_PARAMETER);
		return tevent_req_post(req, ev);
	}
	if (!parse_sidlist(state, request->extra_data.data,
			   &state->sids, &state->num_sids)) {
		DEBUG(10, ("parse_sidlist failed\n"));
		tevent_req_nterror(req, NT_STATUS_INVALID_PARAMETER);
		return tevent_req_post(req, ev);
	}
	subreq = wb_lookupsids_send(state, ev, state->sids, state->num_sids);
	if (tevent_req_nomem(subreq, req)) {
		return tevent_req_post(req, ev);
	}
	tevent_req_set_callback(subreq, winbindd_lookupsids_done, req);
	return req;
}