static char *dex_resolve_library(const char *library) {
	if (!library || library[0] != 'L') {
		return NULL;
	}
	char *demangled = strdup(library + 1);
	rz_str_replace_ch(demangled, '/', '.', 1);
	demangled[strlen(demangled) - 1] = 0;
	return demangled;
}