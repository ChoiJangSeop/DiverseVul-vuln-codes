extract_argv (EvDocument *document, gint page)
{
	ComicsDocument *comics_document = COMICS_DOCUMENT (document);
	char **argv;
	char *command_line, *quoted_archive, *quoted_filename;
	GError *err = NULL;

        if (page >= comics_document->page_names->len)
                return NULL;

	if (comics_document->regex_arg) {
		quoted_archive = g_shell_quote (comics_document->archive);
		quoted_filename =
			comics_regex_quote (comics_document->page_names->pdata[page]);
	} else {
		quoted_archive = g_shell_quote (comics_document->archive);
		quoted_filename = g_shell_quote (comics_document->page_names->pdata[page]);
	}

	command_line = g_strdup_printf ("%s %s %s",
					comics_document->extract_command,
					quoted_archive,
					quoted_filename);
	g_free (quoted_archive);
	g_free (quoted_filename);

	g_shell_parse_argv (command_line, NULL, &argv, &err);
	g_free (command_line);

	if (err) {
		g_warning (_("Error %s"), err->message);
		g_error_free (err);
		return NULL;
	}

	return argv;
}