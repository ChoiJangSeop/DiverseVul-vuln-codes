static int read_pack_info_file(const char *infofile)
{
	FILE *fp;
	char line[1000];
	int old_cnt = 0;

	fp = fopen(infofile, "r");
	if (!fp)
		return 1; /* nonexistent is not an error. */

	while (fgets(line, sizeof(line), fp)) {
		int len = strlen(line);
		if (len && line[len-1] == '\n')
			line[--len] = 0;

		if (!len)
			continue;

		switch (line[0]) {
		case 'P': /* P name */
			if (parse_pack_def(line, old_cnt++))
				goto out_stale;
			break;
		case 'D': /* we used to emit D but that was misguided. */
			goto out_stale;
			break;
		case 'T': /* we used to emit T but nobody uses it. */
			goto out_stale;
			break;
		default:
			error("unrecognized: %s", line);
			break;
		}
	}
	fclose(fp);
	return 0;
 out_stale:
	fclose(fp);
	return 1;
}