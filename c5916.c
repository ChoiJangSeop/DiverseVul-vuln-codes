char *uwsgi_expand_path(char *dir, int dir_len, char *ptr) {
	char src[PATH_MAX + 1];
	memcpy(src, dir, dir_len);
	src[dir_len] = 0;
	char *dst = ptr;
	if (!dst)
		dst = uwsgi_malloc(PATH_MAX + 1);
	if (!realpath(src, dst)) {
		uwsgi_error_realpath(src);
		if (!ptr)
			free(dst);
		return NULL;
	}
	return dst;
}