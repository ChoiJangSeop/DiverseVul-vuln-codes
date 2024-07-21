static void querystring_cb(const char *name, const char *value)
{
	if (!value)
		value = "";

	if (!strcmp(name,"r")) {
		ctx.qry.repo = xstrdup(value);
		ctx.repo = cgit_get_repoinfo(value);
	} else if (!strcmp(name, "p")) {
		ctx.qry.page = xstrdup(value);
	} else if (!strcmp(name, "url")) {
		if (*value == '/')
			value++;
		ctx.qry.url = xstrdup(value);
		cgit_parse_url(value);
	} else if (!strcmp(name, "qt")) {
		ctx.qry.grep = xstrdup(value);
	} else if (!strcmp(name, "q")) {
		ctx.qry.search = xstrdup(value);
	} else if (!strcmp(name, "h")) {
		ctx.qry.head = xstrdup(value);
		ctx.qry.has_symref = 1;
	} else if (!strcmp(name, "id")) {
		ctx.qry.sha1 = xstrdup(value);
		ctx.qry.has_sha1 = 1;
	} else if (!strcmp(name, "id2")) {
		ctx.qry.sha2 = xstrdup(value);
		ctx.qry.has_sha1 = 1;
	} else if (!strcmp(name, "ofs")) {
		ctx.qry.ofs = atoi(value);
	} else if (!strcmp(name, "path")) {
		ctx.qry.path = trim_end(value, '/');
	} else if (!strcmp(name, "name")) {
		ctx.qry.name = xstrdup(value);
	} else if (!strcmp(name, "mimetype")) {
		ctx.qry.mimetype = xstrdup(value);
	} else if (!strcmp(name, "s")) {
		ctx.qry.sort = xstrdup(value);
	} else if (!strcmp(name, "showmsg")) {
		ctx.qry.showmsg = atoi(value);
	} else if (!strcmp(name, "period")) {
		ctx.qry.period = xstrdup(value);
	} else if (!strcmp(name, "dt")) {
		ctx.qry.difftype = atoi(value);
		ctx.qry.has_difftype = 1;
	} else if (!strcmp(name, "ss")) {
		/* No longer generated, but there may be links out there. */
		ctx.qry.difftype = atoi(value) ? DIFF_SSDIFF : DIFF_UNIFIED;
		ctx.qry.has_difftype = 1;
	} else if (!strcmp(name, "all")) {
		ctx.qry.show_all = atoi(value);
	} else if (!strcmp(name, "context")) {
		ctx.qry.context = atoi(value);
	} else if (!strcmp(name, "ignorews")) {
		ctx.qry.ignorews = atoi(value);
	} else if (!strcmp(name, "follow")) {
		ctx.qry.follow = atoi(value);
	}
}