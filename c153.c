int jpc_atoaf(char *s, int *numvalues, double **values)
{
	static char delim[] = ", \t\n";
	char buf[4096];
	int n;
	double *vs;
	char *cp;

	strncpy(buf, s, sizeof(buf));
	buf[sizeof(buf) - 1] = '\0';
	n = 0;
	if ((cp = strtok(buf, delim))) {
		++n;
		while ((cp = strtok(0, delim))) {
			if (cp != '\0') {
				++n;
			}
		}
	}

	if (n) {
		if (!(vs = jas_malloc(n * sizeof(double)))) {
			return -1;
		}

		strncpy(buf, s, sizeof(buf));
		buf[sizeof(buf) - 1] = '\0';
		n = 0;
		if ((cp = strtok(buf, delim))) {
			vs[n] = atof(cp);
			++n;
			while ((cp = strtok(0, delim))) {
				if (cp != '\0') {
					vs[n] = atof(cp);
					++n;
				}
			}
		}
	} else {
		vs = 0;
	}

	*numvalues = n;
	*values = vs;

	return 0;
}