static void format_expand_code(const char **format, GString *out, int *flags)
{
	int set;

	if (flags == NULL) {
		/* flags are being ignored - skip the code */
		while (**format != ']')
			(*format)++;
		return;
	}

	set = TRUE;
	(*format)++;
	while (**format != ']' && **format != '\0') {
		if (**format == '+')
			set = TRUE;
		else if (**format == '-')
			set = FALSE;
		else switch (**format) {
		case 's':
		case 'S':
			*flags |= !set ? PRINT_FLAG_UNSET_LINE_START :
				**format == 's' ? PRINT_FLAG_SET_LINE_START :
				PRINT_FLAG_SET_LINE_START_IRSSI;
			break;
		case 't':
			*flags |= set ? PRINT_FLAG_SET_TIMESTAMP :
				PRINT_FLAG_UNSET_TIMESTAMP;
			break;
		case 'T':
			*flags |= set ? PRINT_FLAG_SET_SERVERTAG :
				PRINT_FLAG_UNSET_SERVERTAG;
			break;
		}

		(*format)++;
	}
}