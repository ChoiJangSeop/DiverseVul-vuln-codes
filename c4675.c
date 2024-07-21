int format_expand_styles(GString *out, const char **format, int *flags)
{
	int retval = 1;

	char *p, fmt;

	/* storage for numerical parsing code for %x/X formats. */
	int tmp;
	unsigned int tmp2;

	fmt = **format;
	switch (fmt) {
	case '{':
	case '}':
	case '%':
		/* escaped char */
		g_string_append_c(out, fmt);
		break;
	case 'U':
		/* Underline on/off */
		g_string_append_c(out, 4);
		g_string_append_c(out, FORMAT_STYLE_UNDERLINE);
		break;
	case '9':
	case '_':
		/* bold on/off */
		g_string_append_c(out, 4);
		g_string_append_c(out, FORMAT_STYLE_BOLD);
		break;
	case '8':
		/* reverse */
		g_string_append_c(out, 4);
		g_string_append_c(out, FORMAT_STYLE_REVERSE);
		break;
	case 'I':
		/* italic */
		g_string_append_c(out, 4);
		g_string_append_c(out, FORMAT_STYLE_ITALIC);
		break;
	case ':':
		/* Newline */
		g_string_append_c(out, '\n');
		break;
	case '|':
		/* Indent here */
		g_string_append_c(out, 4);
		g_string_append_c(out, FORMAT_STYLE_INDENT);
		break;
	case 'F':
		/* blink */
		g_string_append_c(out, 4);
		g_string_append_c(out, FORMAT_STYLE_BLINK);
		break;
	case 'n':
	case 'N':
		/* default color */
		g_string_append_c(out, 4);
		g_string_append_c(out, FORMAT_STYLE_DEFAULTS);
		break;
	case '>':
		/* clear to end of line */
		g_string_append_c(out, 4);
		g_string_append_c(out, FORMAT_STYLE_CLRTOEOL);
		break;
	case '#':
		g_string_append_c(out, 4);
		g_string_append_c(out, FORMAT_STYLE_MONOSPACE);
		break;
	case '[':
		/* code */
		format_expand_code(format, out, flags);
		break;
	case 'x':
	case 'X':
		if ((*format)[1] < '0' || (*format)[1] > '7')
			break;

		tmp = 16 + ((*format)[1]-'0'-1)*36;
		if (tmp > 231) {
			if (!isalpha((*format)[2]))
				break;

			tmp += (*format)[2] >= 'a' ? (*format)[2] - 'a' : (*format)[2] - 'A';

			if (tmp > 255)
				break;
		}
		else if (tmp > 0) {
			if (!isalnum((*format)[2]))
				break;

			if ((*format)[2] >= 'a')
				tmp += 10 + (*format)[2] - 'a';
			else if ((*format)[2] >= 'A')
				tmp += 10 + (*format)[2] - 'A';
			else
				tmp += (*format)[2] - '0';
		}
		else {
			if (!isxdigit((*format)[2]))
				break;

			tmp = g_ascii_xdigit_value((*format)[2]);
		}

		retval += 2;

		format_ext_color(out, fmt == 'x', tmp);
		break;
	case 'z':
	case 'Z':
		tmp2 = 0;
		for (tmp = 1; tmp < 7; ++tmp) {
			if (!isxdigit((*format)[tmp])) {
				tmp2 = UINT_MAX;
				break;
			}
			tmp2 <<= 4;
			tmp2 |= g_ascii_xdigit_value((*format)[tmp]);
		}

		if (tmp2 == UINT_MAX)
			break;

		retval += 6;

		format_24bit_color(out, fmt == 'z', tmp2);
		break;
	default:
		/* check if it's a background color */
		p = strchr(format_backs, fmt);
		if (p != NULL) {
			g_string_append_c(out, 4);
			g_string_append_c(out, FORMAT_COLOR_NOCHANGE);
			g_string_append_c(out, (char) ((int) (p-format_backs)+'0'));
			break;
		}

		/* check if it's a foreground color */
		if (fmt == 'p') fmt = 'm';
		p = strchr(format_fores, fmt);
		if (p != NULL) {
			g_string_append_c(out, 4);
			g_string_append_c(out, (char) ((int) (p-format_fores)+'0'));
			g_string_append_c(out, FORMAT_COLOR_NOCHANGE);
			break;
		}

		/* check if it's a bold foreground color */
		if (fmt == 'P') fmt = 'M';
		p = strchr(format_boldfores, fmt);
		if (p != NULL) {
			g_string_append_c(out, 4);
			g_string_append_c(out, (char) (8+(int) (p-format_boldfores)+'0'));
			g_string_append_c(out, FORMAT_COLOR_NOCHANGE);
			break;
		}

		return FALSE;
	}

	return retval;
}