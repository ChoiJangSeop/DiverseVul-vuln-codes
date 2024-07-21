void load_cpu(const char *fname) {
	if (!fname)
		return;

	FILE *fp = fopen(fname, "re");
	if (fp) {
		unsigned tmp;
		int rv = fscanf(fp, "%x", &tmp);
		if (rv)
			cfg.cpus = (uint32_t) tmp;
		fclose(fp);
	}
	else
		fwarning("cannot load cpu affinity mask\n");
}