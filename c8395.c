static int parse_one_feature(const char *feature, int from_stream)
{
	const char *arg;

	if (skip_prefix(feature, "date-format=", &arg)) {
		option_date_format(arg);
	} else if (skip_prefix(feature, "import-marks=", &arg)) {
		option_import_marks(arg, from_stream, 0);
	} else if (skip_prefix(feature, "import-marks-if-exists=", &arg)) {
		option_import_marks(arg, from_stream, 1);
	} else if (skip_prefix(feature, "export-marks=", &arg)) {
		option_export_marks(arg);
	} else if (!strcmp(feature, "get-mark")) {
		; /* Don't die - this feature is supported */
	} else if (!strcmp(feature, "cat-blob")) {
		; /* Don't die - this feature is supported */
	} else if (!strcmp(feature, "relative-marks")) {
		relative_marks_paths = 1;
	} else if (!strcmp(feature, "no-relative-marks")) {
		relative_marks_paths = 0;
	} else if (!strcmp(feature, "done")) {
		require_explicit_termination = 1;
	} else if (!strcmp(feature, "force")) {
		force_update = 1;
	} else if (!strcmp(feature, "notes") || !strcmp(feature, "ls")) {
		; /* do nothing; we have the feature */
	} else {
		return 0;
	}

	return 1;
}