void load_cgroup(const char *fname) {
	if (!fname)
		return;

	FILE *fp = fopen(fname, "re");
	if (fp) {
		char buf[MAXBUF];
		if (fgets(buf, MAXBUF, fp)) {
			cfg.cgroup = strdup(buf);
			if (!cfg.cgroup)
				errExit("strdup");
		}
		else
			goto errout;

		fclose(fp);
		return;
	}
errout:
	fwarning("cannot load control group\n");
	if (fp)
		fclose(fp);
}