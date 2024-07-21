size_t srvstr_pull_req_talloc(TALLOC_CTX *ctx, struct smb_request *req,
			      char **dest, const uint8_t *src, int flags)
{
	ssize_t bufrem = smbreq_bufrem(req, src);

	if (bufrem < 0) {
		return 0;
	}

	return pull_string_talloc(ctx, req->inbuf, req->flags2, dest, src,
				  bufrem, flags);
}