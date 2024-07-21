int load_env(char *envstr, FILE * f) {
	long filepos;
	int fileline;
	enum env_state state;
	char quotechar, *c, *str, *val;

	filepos = ftell(f);
	fileline = LineNumber;
	skip_comments(f);
	if (EOF == get_string(envstr, MAX_ENVSTR, f, "\n"))
		return (ERR);

	Debug(DPARS, ("load_env, read <%s>\n", envstr));

	str = envstr;
	state = NAMEI;
	quotechar = '\0';
	c = envstr;
	while (state != ERROR && *c) {
		switch (state) {
		case NAMEI:
		case VALUEI:
			if (*c == '\'' || *c == '"')
				quotechar = *c++;
			state++;
			/* FALLTHROUGH */
		case NAME:
		case VALUE:
			if (quotechar) {
				if (*c == quotechar) {
					state++;
					c++;
					break;
				}
				if (state == NAME && *c == '=') {
					state = ERROR;
					break;
				}
			}
			else {
				if (state == NAME) {
					if (isspace((unsigned char) *c)) {
						c++;
						state++;
						break;
					}
					if (*c == '=') {
						state++;
						break;
					}
				}
			}
			*str++ = *c++;
			break;

		case EQ1:
			if (*c == '=') {
				state++;
				quotechar = '\0';
				*str++ = *c;
				val = str;
			}
			else {
				if (!isspace((unsigned char) *c))
					state = ERROR;
			}
			c++;
			break;

		case EQ2:
		case FINI:
			if (isspace((unsigned char) *c))
				c++;
			else
				state++;
			break;

		default:
			abort();
		}
	}
	if (state != FINI && state != EQ2 && !(state == VALUE && !quotechar)) {
		Debug(DPARS, ("load_env, not an env var, state = %d\n", state));
		if (fseek(f, filepos, 0)) {
			return ERR;
		}
		Set_LineNum(fileline);
		return (FALSE);
	}
	*str = '\0';
	if (state == VALUE) {
		/* End of unquoted value: trim trailing whitespace */
		while (str > val && isspace((unsigned char)str[-1]))
			*(--str) = '\0';
	}
	return TRUE;
}