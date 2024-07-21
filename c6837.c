format_cb_session_group_list(struct format_tree *ft, struct format_entry *fe)
{
	struct session		*s = ft->s;
	struct session_group	*sg;
	struct session		*loop;
	struct evbuffer		*buffer;
	int			 size;

	if (s == NULL)
		return;
	sg = session_group_contains(s);
	if (sg == NULL)
		return;

	buffer = evbuffer_new();
	TAILQ_FOREACH(loop, &sg->sessions, gentry) {
		if (EVBUFFER_LENGTH(buffer) > 0)
			evbuffer_add(buffer, ",", 1);
		evbuffer_add_printf(buffer, "%s", loop->name);
	}
	if ((size = EVBUFFER_LENGTH(buffer)) != 0)
		xasprintf(&fe->value, "%.*s", size, EVBUFFER_DATA(buffer));
	evbuffer_free(buffer);
}