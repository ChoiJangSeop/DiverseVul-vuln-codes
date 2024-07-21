struct cgit_cmd *cgit_get_cmd(struct cgit_context *ctx)
{
	static struct cgit_cmd cmds[] = {
		def_cmd(about, 0, 1),
		def_cmd(blob, 1, 0),
		def_cmd(commit, 1, 1),
		def_cmd(diff, 1, 1),
		def_cmd(log, 1, 1),
		def_cmd(ls_cache, 0, 0),
		def_cmd(patch, 1, 0),
		def_cmd(refs, 1, 1),
		def_cmd(repolist, 0, 0),
		def_cmd(snapshot, 1, 0),
		def_cmd(summary, 1, 1),
		def_cmd(tag, 1, 1),
		def_cmd(tree, 1, 1),
	};
	int i;

	if (ctx->qry.page == NULL) {
		if (ctx->repo)
			ctx->qry.page = "summary";
		else
			ctx->qry.page = "repolist";
	}

	for(i = 0; i < sizeof(cmds)/sizeof(*cmds); i++)
		if (!strcmp(ctx->qry.page, cmds[i].name))
			return &cmds[i];
	return NULL;
}