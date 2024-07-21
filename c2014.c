static SC_HTMLState sc_html_read_line(SC_HTMLParser *parser)
{
	gchar buf[SC_HTMLBUFSIZE];
	gchar buf2[SC_HTMLBUFSIZE];
	gint index;
	gint n;

	if (parser->fp == NULL)
		return SC_HTML_EOF;

	n = fread(buf, 1, sizeof(buf), parser->fp);
	if (n == 0) {
		parser->state = SC_HTML_EOF;
		return SC_HTML_EOF;
	} else
		buf[n] = '\0';

	if (conv_convert(parser->conv, buf2, sizeof(buf2), buf) < 0) {
		index = parser->bufp - parser->buf->str;

		conv_utf8todisp(buf2, sizeof(buf2), buf);
		g_string_append(parser->buf, buf2);

		parser->bufp = parser->buf->str + index;

		return SC_HTML_CONV_FAILED;
	}

	index = parser->bufp - parser->buf->str;

	g_string_append(parser->buf, buf2);

	parser->bufp = parser->buf->str + index;

	return SC_HTML_NORMAL;
}