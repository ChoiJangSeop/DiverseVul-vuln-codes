static bool consume_encoded_name_new(RBuffer *b, ut64 bound, ut32 *len_out, char **str_out) {
	ut32 len;
	char *orig = NULL;
	if (!consume_str_new (b, bound, &len, &orig)) {
		return false;
	}

	// room for even every character getting encoded
	size_t maxsize = (len * 4) + 2;
	char *sout = malloc (maxsize);
	if (!sout) {
		free (orig);
		return false;
	}

	size_t i, oi = 0;
	for (i = 0; i < len && oi + 4 < maxsize; i++) {
		if (WASM_IS_OK (orig, i, len)) {
			sout[oi++] = orig[i];
		} else {
			oi += snprintf (sout + oi, maxsize - oi, "_%02x_", orig[i]);
		}
	}
	sout[oi++] = '\0';
	free (orig);

	char *tmp = realloc (sout, oi);
	if (!tmp) {
		free (sout);
		free (tmp);
		return false;
	}
	*str_out = tmp;
	if (len_out) {
		*len_out = len;
	}
	return true;
}