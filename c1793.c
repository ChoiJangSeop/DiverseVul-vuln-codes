results_differ(const char *testname, const char *resultsfile, const char *default_expectfile)
{
	char		expectfile[MAXPGPATH];
	char		diff[MAXPGPATH];
	char		cmd[MAXPGPATH * 3];
	char		best_expect_file[MAXPGPATH];
	FILE	   *difffile;
	int			best_line_count;
	int			i;
	int			l;
	const char *platform_expectfile;

	/*
	 * We can pass either the resultsfile or the expectfile, they should have
	 * the same type (filename.type) anyway.
	 */
	platform_expectfile = get_expectfile(testname, resultsfile);

	strcpy(expectfile, default_expectfile);
	if (platform_expectfile)
	{
		/*
		 * Replace everything afer the last slash in expectfile with what the
		 * platform_expectfile contains.
		 */
		char	   *p = strrchr(expectfile, '/');

		if (p)
			strcpy(++p, platform_expectfile);
	}

	/* Name to use for temporary diff file */
	snprintf(diff, sizeof(diff), "%s.diff", resultsfile);

	/* OK, run the diff */
	snprintf(cmd, sizeof(cmd),
			 SYSTEMQUOTE "diff %s \"%s\" \"%s\" > \"%s\"" SYSTEMQUOTE,
			 basic_diff_opts, expectfile, resultsfile, diff);

	/* Is the diff file empty? */
	if (run_diff(cmd, diff) == 0)
	{
		unlink(diff);
		return false;
	}

	/* There may be secondary comparison files that match better */
	best_line_count = file_line_count(diff);
	strcpy(best_expect_file, expectfile);

	for (i = 0; i <= 9; i++)
	{
		char	   *alt_expectfile;

		alt_expectfile = get_alternative_expectfile(expectfile, i);
		if (!file_exists(alt_expectfile))
			continue;

		snprintf(cmd, sizeof(cmd),
				 SYSTEMQUOTE "diff %s \"%s\" \"%s\" > \"%s\"" SYSTEMQUOTE,
				 basic_diff_opts, alt_expectfile, resultsfile, diff);

		if (run_diff(cmd, diff) == 0)
		{
			unlink(diff);
			return false;
		}

		l = file_line_count(diff);
		if (l < best_line_count)
		{
			/* This diff was a better match than the last one */
			best_line_count = l;
			strcpy(best_expect_file, alt_expectfile);
		}
		free(alt_expectfile);
	}

	/*
	 * fall back on the canonical results file if we haven't tried it yet and
	 * haven't found a complete match yet.
	 */

	if (platform_expectfile)
	{
		snprintf(cmd, sizeof(cmd),
				 SYSTEMQUOTE "diff %s \"%s\" \"%s\" > \"%s\"" SYSTEMQUOTE,
				 basic_diff_opts, default_expectfile, resultsfile, diff);

		if (run_diff(cmd, diff) == 0)
		{
			/* No diff = no changes = good */
			unlink(diff);
			return false;
		}

		l = file_line_count(diff);
		if (l < best_line_count)
		{
			/* This diff was a better match than the last one */
			best_line_count = l;
			strcpy(best_expect_file, default_expectfile);
		}
	}

	/*
	 * Use the best comparison file to generate the "pretty" diff, which we
	 * append to the diffs summary file.
	 */
	snprintf(cmd, sizeof(cmd),
			 SYSTEMQUOTE "diff %s \"%s\" \"%s\" >> \"%s\"" SYSTEMQUOTE,
			 pretty_diff_opts, best_expect_file, resultsfile, difffilename);
	run_diff(cmd, difffilename);

	/* And append a separator */
	difffile = fopen(difffilename, "a");
	if (difffile)
	{
		fprintf(difffile,
				"\n======================================================================\n\n");
		fclose(difffile);
	}

	unlink(diff);
	return true;
}