void URI_FUNC(ResetUri)(URI_TYPE(Uri) * uri) {
	memset(uri, 0, sizeof(URI_TYPE(Uri)));
}