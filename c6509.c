void cgit_clone_objects(void)
{
	if (!ctx.qry.path) {
		cgit_print_error_page(400, "Bad request", "Bad request");
		return;
	}

	if (!strcmp(ctx.qry.path, "info/packs")) {
		print_pack_info();
		return;
	}

	send_file(git_path("objects/%s", ctx.qry.path));
}