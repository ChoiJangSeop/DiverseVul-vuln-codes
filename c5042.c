static int ahash_op_unaligned(struct ahash_request *req,
			      int (*op)(struct ahash_request *))
{
	int err;

	err = ahash_save_req(req, ahash_op_unaligned_done);
	if (err)
		return err;

	err = op(req);
	ahash_op_unaligned_finish(req, err);

	return err;
}