size_t srvstr_get_path_req(TALLOC_CTX *mem_ctx, struct smb_request *req,
				 char **pp_dest, const char *src, int flags,
				 NTSTATUS *err)
{
	ssize_t bufrem = smbreq_bufrem(req, src);

	if (bufrem < 0) {
		*err = NT_STATUS_INVALID_PARAMETER;
		return 0;
	}

	if (req->posix_pathnames) {
		return srvstr_get_path_internal(mem_ctx,
				(const char *)req->inbuf,
				req->flags2,
				pp_dest,
				src,
				bufrem,
				flags,
				true,
				err);
	} else {
		return srvstr_get_path_internal(mem_ctx,
				(const char *)req->inbuf,
				req->flags2,
				pp_dest,
				src,
				bufrem,
				flags,
				false,
				err);
	}
}