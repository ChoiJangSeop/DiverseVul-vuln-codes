
private int
save_cont(struct magic_set *ms, struct cont *c)
{
	size_t len;
	*c = ms->c;
	len = c->len * sizeof(*c->li);
	ms->c.li = CAST(struct level_info *, malloc(len));
	if (ms->c.li == NULL) {
		ms->c = *c;
		return -1;
	}
	memcpy(ms->c.li, c->li, len);