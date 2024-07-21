void cgit_print_http_headers(void)
{
	if (ctx.env.no_http && !strcmp(ctx.env.no_http, "1"))
		return;

	if (ctx.page.status)
		htmlf("Status: %d %s\n", ctx.page.status, ctx.page.statusmsg);
	if (ctx.page.mimetype && ctx.page.charset)
		htmlf("Content-Type: %s; charset=%s\n", ctx.page.mimetype,
		      ctx.page.charset);
	else if (ctx.page.mimetype)
		htmlf("Content-Type: %s\n", ctx.page.mimetype);
	if (ctx.page.size)
		htmlf("Content-Length: %zd\n", ctx.page.size);
	if (ctx.page.filename)
		htmlf("Content-Disposition: inline; filename=\"%s\"\n",
		      ctx.page.filename);
	if (!ctx.env.authenticated)
		html("Cache-Control: no-cache, no-store\n");
	htmlf("Last-Modified: %s\n", http_date(ctx.page.modified));
	htmlf("Expires: %s\n", http_date(ctx.page.expires));
	if (ctx.page.etag)
		htmlf("ETag: \"%s\"\n", ctx.page.etag);
	html("\n");
	if (ctx.env.request_method && !strcmp(ctx.env.request_method, "HEAD"))
		exit(0);
}