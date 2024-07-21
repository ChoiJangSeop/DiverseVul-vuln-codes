managesieve_parser_read_string(struct managesieve_parser *parser,
				   const unsigned char *data, size_t data_size)
{
	size_t i;

	/* QUOTED-CHAR        = SAFE-UTF8-CHAR / "\" QUOTED-SPECIALS
	 * quoted             = <"> *QUOTED-CHAR <">
	 *                    ;; limited to 1024 octets between the <">s
	 */

	/* read until we've found non-escaped ", CR or LF */
	for (i = parser->cur_pos; i < data_size; i++) {
		if (data[i] == '"') {

			if ( !uni_utf8_data_is_valid(data+1, i-1) ) {
				parser->error = "Invalid UTF-8 character in quoted-string.";
				return FALSE;
			}

			managesieve_parser_save_arg(parser, data, i);
			i++; /* skip the trailing '"' too */
			break;
		}

		if (data[i] == '\\') {
			if (i+1 == data_size) {
				/* known data ends with '\' - leave it to
				   next time as well if it happens to be \" */
				break;
			}

			/* save the first escaped char */
			if (parser->str_first_escape < 0)
				parser->str_first_escape = i;

			/* skip the escaped char */
			i++;

			if ( !IS_QUOTED_SPECIAL(data[i]) ) {
				parser->error =
					"Escaped quoted-string character is not a QUOTED-SPECIAL.";
				return FALSE;
			}

			continue;
		}

		if ( (data[i] & 0x80) == 0 && !IS_SAFE_CHAR(data[i]) ) {
			parser->error = "String contains invalid character.";
			return FALSE;
		}
	}

	parser->cur_pos = i;
	return ( parser->cur_type == ARG_PARSE_NONE );
}